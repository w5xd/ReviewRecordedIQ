/* Copyright (c) 2022, Wayne Wright, W5XD. All rights reserved. */
#include "SimpleSdrImpl.h"
#include <AudioSink.h>
#include <RiffReader.h>
#include <FIRFilter.h>
#include <PrecomputeSinCos.h>
#include <deque>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <fstream>
#include <cstring>

namespace NARROW_CW_FILTER {
    const unsigned SAMPLEFILTER_TAP_NUM = 101;
    extern FilterCoeficient_t filter_taps[];
}
namespace WIDE_CW_FILTER {
    const unsigned SAMPLEFILTER_TAP_NUM = 101;
    extern FilterCoeficient_t filter_taps[];
}
namespace NARROW_SSB_FILTER {
    const unsigned SAMPLEFILTER_TAP_NUM = 101;
    extern FilterCoeficient_t filter_taps[];
}
namespace WIDE_SSB_FILTER {
    const unsigned SAMPLEFILTER_TAP_NUM = 101;
    extern FilterCoeficient_t filter_taps[];
}

namespace XDSdr {
    namespace impl {

        const unsigned IQ_AND_OUTPUT_FRAMES_PER_SECOND = 12000;
        const unsigned PRECOMPUTE_SIN_COS_DENSITY = 2; // double the frequency of the table

        class SimpleSDRImpl
        {
        public:
            SimpleSDRImpl(const std::string &fileName, void *sink) 
                : m_bandwidth(SimpleSDR::UNINITIALIZED)
                , m_RxFrequencyKHz(0)
                , m_BfoOffsetKHz(0)
                , m_stop(false)
                , m_pause(true) // paused at the beginning
                , m_riffReader(m_inputWave)
                , m_bandPassFilters(2)
                , m_MixIindex(0)
                , m_MixQindex(0)
                , m_mixFrequency(IQ_AND_OUTPUT_FRAMES_PER_SECOND) // invalid
                , m_WeaverIindex(0)
                , m_WeaverQindex(0)
                , m_WeaverFreq(IQ_AND_OUTPUT_FRAMES_PER_SECOND) // invalid
                , m_QScale(1)
                , m_WeaverQScale(1)
                , m_gain(1)
                , m_maxObserved(0)
                , m_currentFrameNumber(0)
            {
                m_audioSink = std::shared_ptr<XD::AudioSink>(reinterpret_cast<XD::AudioSink*>(sink),
                    [](XD::AudioSink* p) { p->ReleaseSink(); });

                m_inputWave.open(fileName.c_str(), std::ifstream::binary);
                if (!m_inputWave.is_open())
                    throw std::runtime_error("Failed to open input file");

                m_riffReader.ParseHeader();

                if (m_riffReader.get_numChannels() != 2)
                    throw std::runtime_error("Input file must be stereo");

                if (m_riffReader.get_format() != 3 || m_riffReader.get_bitsPerSample() != 32)
                    throw std::runtime_error("Input file must be 32 bit float format");

                SetBandwidth(SimpleSDR::WIDE_SSB);
                SetRxFrequencyCenterHz(0);
                SetRxFrequencyBfoOffsetHz(0);

                m_thread = std::thread(std::bind(&SimpleSDRImpl::thread, this));
            }

            ~SimpleSDRImpl()
            {      Close();      }

            void Close()
            {
                {
                    lock_t l(m_mutex);
                    m_stop = true;
                    m_pause = false;
                    m_cond.notify_all();
                }
                if (m_thread.joinable())
                    m_thread.join();
                m_audioSink.reset();
            }
 
            void Play()
            {
                lock_t l(m_mutex);
                m_pause = false;
                m_cond.notify_all();
            }

            void Pause()
            {
                lock_t l(m_mutex);
                m_pause = true;
                m_cond.notify_all();
            }

            float GetPlayLengthSeconds()
            {
                auto blockAlign = m_riffReader.get_blockAlign();
                if (blockAlign != 0)
                    return (static_cast<float>(m_riffReader.get_dataChunkSize()) / blockAlign) / IQ_AND_OUTPUT_FRAMES_PER_SECOND;
                return 0;
            }

            float GetIfBoundaryAbsHz()
            {
                return m_riffReader.get_sampleRate() / 2.0f;
            }

            float GetPlayPositionSeconds()
            {
                lock_t l(m_mutex);
                return m_currentFrameNumber / static_cast<float>(IQ_AND_OUTPUT_FRAMES_PER_SECOND);
            }

            void SetPlayPositionSeconds(float v)
            {
                lock_t l(m_mutex);
                m_queue.push_back([this, v]()
                    {
                        unsigned frameNumber = static_cast<unsigned>(v * IQ_AND_OUTPUT_FRAMES_PER_SECOND);
                        m_riffReader.SeekToFrameNumber(frameNumber);
                    });
                m_cond.notify_all();
            }

            float GetRxFrequencyCenterHz()
            {
                return m_RxFrequencyKHz;
            }

            void SetRxFrequencyCenterHz(float v)
            {
                if (IQ_AND_OUTPUT_FRAMES_PER_SECOND/2 <= static_cast<unsigned>(fabs(v)))
                    return;
                lock_t l(m_mutex);
                m_RxFrequencyKHz = v;
                auto newMix = quantizeMixFrequencyTo10Hz(static_cast<int>(v), m_mixFrequency);
                if (newMix != m_mixFrequency)
                {
                    m_queue.push_back([this, newMix]() 
                    {
                        double prevMixI = 0;
                        double prevMixQ = 1;
                        if (!m_MixCoef.empty())
                        {
                            // prep for glitch minimization
                            // Save the phase of the previous MIX signal
                            prevMixI = m_MixCoef[m_MixIindex];
                            prevMixQ = m_MixCoef[m_MixQindex] * m_QScale;
                        }
                        // glitch minimization is an optimization in the sense
                        // it keeps the output wave continuous when you change m_MixFreq.
                        m_MixIindex = 0;
                        m_MixQindex = 0;
                        m_MixCoef.clear();
                        m_QScale = newMix < 0 ? -1.f : 1.f;
                        // populate the sine table.
                        // use the same table for cosine but start in different position.
                        bool closestOneNeg = PrecomputeSinCos::ComputeSinCos(
                            IQ_AND_OUTPUT_FRAMES_PER_SECOND, newMix >= 0 ? newMix : -newMix, m_MixCoef, m_MixQindex, PRECOMPUTE_SIN_COS_DENSITY);
                        if (closestOneNeg)
                            m_QScale *= -1.f;// flip mixQ summation if we're using upside-down cosine
                        auto deGlitch = PrecomputeSinCos::MinimizeSinCosGlitch(prevMixI, prevMixQ, m_MixQindex, m_QScale, m_MixCoef);
                        m_MixIindex += deGlitch; m_MixQindex += deGlitch;
                        if (m_MixIindex >= m_MixCoef.size())
                            m_MixIindex -= m_MixCoef.size();
                        if (m_MixQindex >= m_MixCoef.size())
                            m_MixQindex -= m_MixCoef.size();
                        m_mixFrequency = newMix;
                    });
                    m_cond.notify_all();
                }
            }

            float GetRxFrequencyBfoOffsetHz()
            {
                return m_BfoOffsetKHz;
            }

            void SetRxFrequencyBfoOffsetHz(float v)
            {
                if (IQ_AND_OUTPUT_FRAMES_PER_SECOND / 2 <= static_cast<unsigned>(fabs(v)))
                    return;
                lock_t l(m_mutex);
                m_BfoOffsetKHz = v;
                auto newMix = quantizeMixFrequencyTo10Hz(static_cast<int>(v), m_WeaverFreq);
                if (newMix != m_WeaverFreq)
                {
                    m_queue.push_back([this, newMix]()
                        {
                            double prevMixI = 0;
                            double prevMixQ = 1;
                            if (!m_WeaverMix.empty())
                            {
                                // prep for glitch minimization
                                // Save the phase of the previous MIX signal
                                 prevMixI = m_WeaverMix[m_WeaverIindex];
                                 prevMixQ = m_WeaverMix[m_WeaverQindex] * m_WeaverQScale;
                                }
                            // glitch minimization is an optimization in the sense
                            // it keeps the output wave continuous when you change m_MixFreq.
                            m_WeaverIindex = 0;
                            m_WeaverQindex = 0;
                            m_WeaverMix.clear();
                            m_WeaverQScale = newMix < 0 ? -1.f : 1.f;
                            // populate the sine table.
                            // use the same table for cosine but start in different position.
                            bool closestOneNeg = PrecomputeSinCos::ComputeSinCos(
                                IQ_AND_OUTPUT_FRAMES_PER_SECOND, newMix >= 0 ? newMix : -newMix, m_WeaverMix, m_WeaverQindex, PRECOMPUTE_SIN_COS_DENSITY);
                            if (closestOneNeg)
                                m_WeaverQScale *= -1.f;// flip mixQ summation if we're using upside-down cosine
                            auto deGlitch = PrecomputeSinCos::MinimizeSinCosGlitch(prevMixI, prevMixQ, m_WeaverQindex, m_WeaverQScale, m_WeaverMix);
                            m_WeaverIindex += deGlitch; m_WeaverQindex += deGlitch;
                            if (m_WeaverIindex >= m_WeaverMix.size())
                                m_WeaverIindex -= m_WeaverMix.size();
                            if (m_WeaverQindex >= m_WeaverMix.size())
                                m_WeaverQindex -= m_WeaverMix.size();
                            m_WeaverFreq = newMix;
                        });
                    m_cond.notify_all();
                }
            }

            SimpleSDR::SdrDecodeBandwidth GetBandwidth()
            {   return m_bandwidth;  }

            void SetBandwidth(SimpleSDR::SdrDecodeBandwidth v)
            {
                lock_t l(m_mutex);
                if (v != m_bandwidth)
                {
                    m_bandwidth = v;
                    m_queue.push_back([this, v]() {
                        unsigned len; const FilterCoeficient_t*coef;
                        switch (v)
                        {
                        case SimpleSDR::NARROW_CW:
                            len = NARROW_CW_FILTER::SAMPLEFILTER_TAP_NUM;
                            coef = NARROW_CW_FILTER::filter_taps;
                            break;
                        case SimpleSDR::WIDE_CW:
                            len = WIDE_CW_FILTER::SAMPLEFILTER_TAP_NUM;
                            coef = WIDE_CW_FILTER::filter_taps;
                            break;
                        case SimpleSDR::NARROW_SSB:
                            len = NARROW_SSB_FILTER::SAMPLEFILTER_TAP_NUM;
                            coef = NARROW_SSB_FILTER::filter_taps;
                            break;
                        case SimpleSDR::WIDE_SSB:
                            len = WIDE_SSB_FILTER::SAMPLEFILTER_TAP_NUM;
                            coef = WIDE_SSB_FILTER::filter_taps;
                            break;
                        default:
                            return;
                        }
                        for (auto &f : m_bandPassFilters)
                            f.setFilterDefinition(len, coef);
                        });
                    m_cond.notify_all();
                }
            }

            std::string FromSliceIQ()
            {
                std::string ret;
                {
                    lock_t l(m_mutex);
                    ret = m_fromSliceIQ;
                }
                return ret;
            }

        private:
            typedef std::unique_lock<std::mutex> lock_t;
            void thread()
            {
                RiffReader::RiffChunkFcn_t riff = [this](const char*buf, unsigned chunkSize, std::ifstream& infile)
                {
                    if (strncmp(buf, "0SDR", 4) == 0)
                    {
                        std::vector<char> buf(chunkSize); 
                        infile.read(&buf[0], chunkSize);
                        lock_t l(m_mutex);
                        for (auto &c : buf)
                            if (isprint(c))
                                m_fromSliceIQ += c;
                    }
                };
                RiffReader::AtEndFcn_t atEnd = [this]() {
                    lock_t l(m_mutex);
                    while (!m_stop && m_queue.empty())
                        m_cond.wait(l);
                    dispatchQueueItems(l);
                    return m_stop;
                };
                m_riffReader.ProcessChunks(std::bind(&SimpleSDRImpl::chunk, this, std::placeholders::_1, std::placeholders::_2), riff, atEnd);
            }

            typedef std::function<void()> OnChunkThreadFcn_t;
            std::deque<OnChunkThreadFcn_t> m_queue;

            bool dispatchQueueItems(lock_t &l)
            {   
                if (!m_queue.empty())
                {
                    auto fcn = m_queue.front();
                    m_queue.pop_front();
                    l.unlock();
                    fcn();
                    l.lock();
                    return true;
                }
                return false;
            }

            bool chunk(unsigned char *p, unsigned numFrames)
            {
                static const unsigned MAX_FRAMES_TO_PROCESS = 120; // 10 msec
                while (numFrames > 0)
                {
                    lock_t l(m_mutex);
                    while (m_pause && m_queue.empty())
                        m_cond.wait(l);
                    if (m_stop)
                        return false;
                    if (dispatchQueueItems(l))
                        continue;
                    m_currentFrameNumber = m_riffReader.CurrentFrameNumber();
                    l.unlock();

                    unsigned framesToProcess = std::min(MAX_FRAMES_TO_PROCESS, numFrames);
                    process(reinterpret_cast<float*>(p), framesToProcess);
                    numFrames -= framesToProcess;
                    p += framesToProcess * m_riffReader.get_blockAlign();
                }

                return true;
            }

            void process(float *p, unsigned numFrames)
            {
                auto result = ApplyMIX(p, numFrames);
                bool foundMax(false);
                for (auto s : result)
                {
                    auto fs = fabs(s);
                    if (fs > m_maxObserved)
                    {
                        m_maxObserved = fs;
                        foundMax = true;
                    }
                }

                if (foundMax)
                {
                    double peakGain = 0.5 / m_maxObserved;
                    m_gain = sqrt(peakGain * m_gain);
                }
                std::vector<short> buf(result.size());
                for (unsigned i = 0; i < result.size(); i++)
                    buf[i] = static_cast<short>(0x7FFF * m_gain * result[i]);
                m_audioSink->AddMonoSoundFrames(&buf[0], numFrames);
            }

            std::vector<float> ApplyMIX(float* p, unsigned numFrames /*I/Q frames*/)
            {
                std::vector<float> ret;
                for (int i = 0; i < static_cast<int>(numFrames); i += 1)
                {
                    float inI = *p++;
                    float inQ = *p++;

                    unsigned sze = static_cast<unsigned>(m_MixCoef.size());
                    double mixI = m_MixCoef[m_MixIindex];
                    m_MixIindex += PRECOMPUTE_SIN_COS_DENSITY;
                    if (m_MixIindex >= sze)
                        m_MixIindex = 0;
                    double mixQ = m_MixCoef[m_MixQindex];
                    m_MixQindex += PRECOMPUTE_SIN_COS_DENSITY;
                    if (m_MixQindex >= sze)
                        m_MixQindex = 0;
                    mixQ *= m_QScale; // flip for negative mixer frequency

                    // The mix is a complex multiply
                    double nextI = inI * mixI - inQ * mixQ;
                    double nextQ = inQ * mixI + inI * mixQ;

                    // low pass the mixed I separate from the mixed Q
                    m_bandPassFilters[0].applySample(nextI);
                    m_bandPassFilters[1].applySample(nextQ);

                    // Now mix again. This time by the m_WeaverFreq
                    // http://www.csun.edu/~skatz/katzpage/sdr_project/sdr/ssb_rcv_signals.pdf
                    sze = static_cast<unsigned>(m_WeaverMix.size());
                    double weaverI = m_WeaverMix[m_WeaverIindex];
                    m_WeaverIindex += PRECOMPUTE_SIN_COS_DENSITY;
                    if (m_WeaverIindex >= sze)
                        m_WeaverIindex = 0;
                    double weaverQ = m_WeaverMix[m_WeaverQindex];
                    m_WeaverQindex += PRECOMPUTE_SIN_COS_DENSITY;
                    if (m_WeaverQindex >= sze)
                        m_WeaverQindex = 0;

                    // ...The sum of the I+Q detects Weaver
                    double v = m_bandPassFilters[0].value() * weaverI;
                    v += m_bandPassFilters[1].value() * weaverQ * m_WeaverQScale;

                    ret.push_back(static_cast<float>(v));
                }
                return ret;
            }

            int quantizeMixFrequencyTo10Hz(int mix, int prevMix)
            {
                static const int RESOLUTION_HZ = 10;
                static const int MAX_MIX = static_cast<int>(IQ_AND_OUTPUT_FRAMES_PER_SECOND / 2) - 1;
                int mixF = mix;
                bool isNeg = mix < 0;
                if (isNeg)
                    mixF = -mix;
                mixF += RESOLUTION_HZ / 2;
                mixF /= RESOLUTION_HZ;
                mixF *= RESOLUTION_HZ;
                if (mixF > MAX_MIX )
                    mixF = MAX_MIX;
                return isNeg ? -mixF : mixF;
            }

            std::vector<CFIRFilter> m_bandPassFilters;
            std::vector<double> m_MixCoef;
            unsigned m_MixIindex;
            unsigned m_MixQindex;
            int m_mixFrequency; // can be negative

            std::vector<double> m_WeaverMix;
            unsigned m_WeaverIindex;
            unsigned m_WeaverQindex;
            int m_WeaverFreq; // can be negative
            double m_QScale;
            double m_WeaverQScale;

            double m_gain;
            float m_maxObserved;

            std::string m_fromSliceIQ;

            bool m_stop;
            bool m_pause;
            RiffReader m_riffReader;
            unsigned m_currentFrameNumber;
            std::ifstream m_inputWave;
            float m_RxFrequencyKHz;
            float m_BfoOffsetKHz;
            SimpleSDR::SdrDecodeBandwidth m_bandwidth;
            std::shared_ptr<XD::AudioSink> m_audioSink;
            std::condition_variable m_cond;
            std::mutex m_mutex;
            std::thread m_thread;
        };

        SimpleSDR::SimpleSDR(const std::string& fileName, void *sink) 
            : m_impl(std::make_shared<SimpleSDRImpl>(fileName, sink))
        {}
        void SimpleSDR::Close() { return m_impl->Close(); }
        void SimpleSDR::Play() { return m_impl->Play(); }
        void SimpleSDR::Pause() { return m_impl->Pause(); }
        float SimpleSDR::GetPlayLengthSeconds() { return m_impl->GetPlayLengthSeconds(); }
        float SimpleSDR::GetIfBoundaryAbsHz() { return m_impl->GetIfBoundaryAbsHz(); }
        float SimpleSDR::GetPlayPositionSeconds() { return m_impl->GetPlayPositionSeconds(); }
        void SimpleSDR::SetPlayPositionSeconds(float v) { return m_impl->SetPlayPositionSeconds(v); }
        float SimpleSDR::GetRxFrequencyCenterHz() { return m_impl->GetRxFrequencyCenterHz(); }
        void SimpleSDR::SetRxFrequencyCenterHz(float v) { return m_impl->SetRxFrequencyCenterHz(v); }
        float SimpleSDR::GetRxFrequencyBfoOffsetHz() { return m_impl->GetRxFrequencyBfoOffsetHz(); }
        void SimpleSDR::SetRxFrequencyBfoOffsetHz(float v) { return m_impl->SetRxFrequencyBfoOffsetHz(v); }
        SimpleSDR::SdrDecodeBandwidth SimpleSDR::GetBandwidth() { return m_impl->GetBandwidth(); }
        void SimpleSDR::SetBandwidth(SimpleSDR::SdrDecodeBandwidth v) { return m_impl->SetBandwidth(v); }
        std::string SimpleSDR::FromSliceIQ() { return m_impl->FromSliceIQ();}
    }
}

/*************************************************************************************************
** Filters ***************************************************************************************
* These are all designed with octave: http://www.octave.org
*************************************************************************************************/

namespace NARROW_CW_FILTER {
/*  Octave:
    f = 125 / 12000
    b = fir1(100, f)
    freqz(b)
    fd = fopen("d:/temp/250of6000.txt", "w")
    fprintf(fd, "%10g,\n", b)
    fclose(fd)
*/
    FilterCoeficient_t filter_taps[SAMPLEFILTER_TAP_NUM] = { 1.33275e-05,
5.24769e-05,
9.6034e-05,
0.000146771,
0.000207588,
0.000281477,
0.000371482,
0.000480656,
0.000612018,
0.00076851,
0.000952954,
0.00116801,
0.00141611,
0.00169948,
0.00202002,
0.00237931,
0.00277859,
0.00321871,
0.00370009,
0.00422273,
0.00478618,
0.00538952,
0.00603137,
0.00670988,
0.00742273,
0.00816715,
0.00893995,
 0.0097375,
 0.0105558,
 0.0113905,
 0.0122368,
 0.0130899,
 0.0139446,
 0.0147953,
 0.0156367,
 0.0164631,
  0.017269,
 0.0180485,
 0.0187964,
 0.0195071,
 0.0201755,
 0.0207966,
 0.0213659,
 0.0218789,
 0.0223318,
 0.0227212,
 0.0230439,
 0.0232976,
 0.0234803,
 0.0235904,
 0.0236272,
 0.0235904,
 0.0234803,
 0.0232976,
 0.0230439,
 0.0227212,
 0.0223318,
 0.0218789,
 0.0213659,
 0.0207966,
 0.0201755,
 0.0195071,
 0.0187964,
 0.0180485,
  0.017269,
 0.0164631,
 0.0156367,
 0.0147953,
 0.0139446,
 0.0130899,
 0.0122368,
 0.0113905,
 0.0105558,
 0.0097375,
0.00893995,
0.00816715,
0.00742273,
0.00670988,
0.00603137,
0.00538952,
0.00478618,
0.00422273,
0.00370009,
0.00321871,
0.00277859,
0.00237931,
0.00202002,
0.00169948,
0.00141611,
0.00116801,
0.000952954,
0.00076851,
0.000612018,
0.000480656,
0.000371482,
0.000281477,
0.000207588,
0.000146771,
9.6034e-05,
5.24769e-05,
1.33275e-05,
    };
}
namespace WIDE_CW_FILTER {
    /*  Octave:
        f = 250/12000
        b = fir1(100,f)
        freqz(b)
        fd=fopen("d:/temp/CW500.txt", "w")
        fprintf(fd, "%10g,\n", b)
        fclose(fd)    
        */
    FilterCoeficient_t filter_taps[SAMPLEFILTER_TAP_NUM] ={
1.33275e-05,
5.24769e-05,
9.6034e-05,
0.000146771,
0.000207588,
0.000281477,
0.000371482,
0.000480656,
0.000612018,
0.00076851,
0.000952954,
0.00116801,
0.00141611,
0.00169948,
0.00202002,
0.00237931,
0.00277859,
0.00321871,
0.00370009,
0.00422273,
0.00478618,
0.00538952,
0.00603137,
0.00670988,
0.00742273,
0.00816715,
0.00893995,
 0.0097375,
 0.0105558,
 0.0113905,
 0.0122368,
 0.0130899,
 0.0139446,
 0.0147953,
 0.0156367,
 0.0164631,
  0.017269,
 0.0180485,
 0.0187964,
 0.0195071,
 0.0201755,
 0.0207966,
 0.0213659,
 0.0218789,
 0.0223318,
 0.0227212,
 0.0230439,
 0.0232976,
 0.0234803,
 0.0235904,
 0.0236272,
 0.0235904,
 0.0234803,
 0.0232976,
 0.0230439,
 0.0227212,
 0.0223318,
 0.0218789,
 0.0213659,
 0.0207966,
 0.0201755,
 0.0195071,
 0.0187964,
 0.0180485,
  0.017269,
 0.0164631,
 0.0156367,
 0.0147953,
 0.0139446,
 0.0130899,
 0.0122368,
 0.0113905,
 0.0105558,
 0.0097375,
0.00893995,
0.00816715,
0.00742273,
0.00670988,
0.00603137,
0.00538952,
0.00478618,
0.00422273,
0.00370009,
0.00321871,
0.00277859,
0.00237931,
0.00202002,
0.00169948,
0.00141611,
0.00116801,
0.000952954,
0.00076851,
0.000612018,
0.000480656,
0.000371482,
0.000281477,
0.000207588,
0.000146771,
9.6034e-05,
5.24769e-05,
1.33275e-05,
    };
}
namespace NARROW_SSB_FILTER {
    /*  Octave:
        f = 1020/12000
        b = fir1(100,f)
        fd=fopen("d:/temp/SSB2040.txt", "w")
        fprintf(fd, "%10g,\n", b)
        fclose(fd)
        */
    FilterCoeficient_t filter_taps[SAMPLEFILTER_TAP_NUM] = 
    {
0.000299054,
0.000188022,
5.73147e-05,
-9.45361e-05,
-0.000267475,
-0.000458224,
-0.000658816,
-0.000855647,
-0.00102931,
-0.0011554,
-0.00120632,
-0.00115407,
-0.000973822,
-0.00064789,
-0.000169779,
0.000452272,
0.00119269,
0.00200744,
0.00283464,
0.00359726,
0.00420787,
 0.0045753,
0.00461276,
0.00424705,
0.00342781,
0.00213626,
0.000392428,
-0.00173999,
-0.00415175,
-0.00668977,
-0.00916253,
-0.0113493,
-0.0130124,
-0.0139128,
-0.0138265,
 -0.012562,
-0.00997698,
-0.00599262,
-0.000604795,
0.00610914,
 0.0139888,
 0.0227944,
 0.0322168,
 0.0418914,
 0.0514174,
 0.0603798,
 0.0683734,
 0.0750261,
  0.080022,
 0.0831202,
 0.0841701,
 0.0831202,
  0.080022,
 0.0750261,
 0.0683734,
 0.0603798,
 0.0514174,
 0.0418914,
 0.0322168,
 0.0227944,
 0.0139888,
0.00610914,
-0.000604795,
-0.00599262,
-0.00997698,
 -0.012562,
-0.0138265,
-0.0139128,
-0.0130124,
-0.0113493,
-0.00916253,
-0.00668977,
-0.00415175,
-0.00173999,
0.000392428,
0.00213626,
0.00342781,
0.00424705,
0.00461276,
 0.0045753,
0.00420787,
0.00359726,
0.00283464,
0.00200744,
0.00119269,
0.000452272,
-0.000169779,
-0.00064789,
-0.000973822,
-0.00115407,
-0.00120632,
-0.0011554,
-0.00102931,
-0.000855647,
-0.000658816,
-0.000458224,
-0.000267475,
-9.45361e-05,
5.73147e-05,
0.000188022,
0.000299054,
    };
}
namespace WIDE_SSB_FILTER {
    /*  Octave:
        f = 1200/12000
        b = fir1(100,f)
        freqz(b)
        fd = fopen("d:/temp/2400of6000.txt", "w")
        fprintf(fd, "%10g,\n", b)
        fclose(fd)
    */
    FilterCoeficient_t filter_taps[SAMPLEFILTER_TAP_NUM] = { 
-7.74788e-05,
-0.000367152,
-0.00054399,
-0.000532634,
-0.000304421,
9.94694e-05,
0.000559382,
0.000897583,
0.00093385,
0.000564832,
-0.00016314,
-0.00102717,
-0.00167654,
-0.00175765,
-0.00107753,
0.000262436,
0.00183457,
0.00299675,
0.00313153,
0.00193186,
-0.000387748,
-0.00306799,
-0.00502119,
-0.00524086,
-0.00326123,
0.000526855,
0.00487117,
0.00802679,
0.00840902,
0.00530257,
-0.00066613,
-0.00753681,
-0.0125862,
-0.0133231,
-0.00857107,
0.000791895,
 0.0117942,
 0.0201706,
 0.0218272,
  0.014515,
-0.000891776,
-0.0200518,
  -0.03601,
-0.0411124,
-0.0294477,
0.000955936,
 0.0469428,
  0.100533,
   0.15074,
  0.186421,
  0.199326,
  0.186421,
   0.15074,
  0.100533,
 0.0469428,
0.000955936,
-0.0294477,
-0.0411124,
  -0.03601,
-0.0200518,
-0.000891776,
  0.014515,
 0.0218272,
 0.0201706,
 0.0117942,
0.000791895,
-0.00857107,
-0.0133231,
-0.0125862,
-0.00753681,
-0.00066613,
0.00530257,
0.00840902,
0.00802679,
0.00487117,
0.000526855,
-0.00326123,
-0.00524086,
-0.00502119,
-0.00306799,
-0.000387748,
0.00193186,
0.00313153,
0.00299675,
0.00183457,
0.000262436,
-0.00107753,
-0.00175765,
-0.00167654,
-0.00102717,
-0.00016314,
0.000564832,
0.00093385,
0.000897583,
0.000559382,
9.94694e-05,
-0.000304421,
-0.000532634,
-0.00054399,
-0.000367152,
-7.74788e-05,
    };
}
