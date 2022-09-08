/* Copyright (c) 2022, Wayne Wright, W5XD. All rights reserved. */

/* SliceIQ
** Command line program that takes a 192KHz rate stereo file as input
** and outputs a 12KHz rate stereo file as output.
**
** SliceIQ <InputFile.wav> <OutputFile.wav>
**
** The input is described by these optional command line arguments:
** --inputCenterKHz=nnnnn
** --inputIsFlipped   The I channel, by definition, is ahead of Q by 90 degrees, but this flips it.
** --inputStartTime=YYYY/MM/DD-HH:MM:SS
**
** The output subset is defined by these command line arguments
** --outputCenterKHz=nnnnn
** --outputIntervalSeconds=nnnn
** --outputStartOffsetSeconds=nnnnn
** --outputStartTime=YYYY/MM/DD-HH:MM:SS
**      Those last two are redundant with each other. If both are specified, outputStartOffsetSeconds is used
*/
#include <string>
#include <cstring>
#include <cmath>
#include <memory>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <algorithm>

#include <PrecomputeSinCos.h>
#include <FIRFilter.h>
#include <RiffReader.h>

namespace {
    const char InputCenterArg[] = "--inputCenterKHz=";
    const char InputStartArg[] = "--inputStartTime=";
    const char InputIsFlippedArg[] = "--inputIsFlipped";
    const char OutputCenterKHzArg[] = "--outputCenterKHz=";
    const char OutputStartSecondsArg[] = "--outputStartOffsetSeconds=";
    const char OutputStartTimeArg[] = "--outputStartTime=";
    const char OutputIntervalSecondsArg[] = "--outputIntervalSeconds=";

    const int INPUT_IQ_SAMPLES_PER_SECOND = 192000;
    const int OUTPUT_IQ_SAMPLES_PER_SECOND = 12000;
    const int DECIMATE = INPUT_IQ_SAMPLES_PER_SECOND / OUTPUT_IQ_SAMPLES_PER_SECOND;
    const char DateFormatDescriptor[] = "%Y/%m/%d-%H:%M:%S";

    const int usage()
    {
        std::cerr << "Usage: SliceIQ [inputFile.wav] [outputFile.wav] " << InputCenterArg << "f  " << InputIsFlippedArg  << "  " << 
            InputStartArg << "YYYY/MM/DD-HH:MM:SS\\" << std::endl
            << " " << OutputCenterKHzArg << "f  [" << OutputStartSecondsArg << "s " << OutputStartTimeArg << "YYYY/MM/DD-HH:MM:SS] " << OutputIntervalSecondsArg << "s"
            << std::endl;
        return 1;
    }

    int process(std::ifstream& inputFile, double inputCenterKHz, std::chrono::system_clock::time_point inputStartTime,
        unsigned inputFramesToSkip, unsigned inputFramesToProcess, std::ofstream& outputFile, double outputCenterKHz,
        std::chrono::system_clock::time_point outputStartTime);
}


int main(int argc, char **argv)
{
    std::ifstream inputFile;
    std::ofstream outputFile;
    bool inputIQflipped = false;
    std::chrono::system_clock::time_point inputStartTime = std::chrono::system_clock::now();
    double inputCenterKHz = 0;

    std::chrono::system_clock::duration outputInterval = std::chrono::seconds(0);
    std::chrono::system_clock::duration outputStartOffset = std::chrono::seconds(0);
    std::chrono::system_clock::time_point outputStartTime = inputStartTime;
    bool outputStartTimeSpecified = false;
    double outputCenterKHz = 0;

    
    // parse command line arguments
    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];
        if (arg.find("--") != 0)
        {
            if (!inputFile.is_open())
            {
                inputFile.open(arg.c_str(), std::ifstream::binary);
                if (!inputFile.is_open())
                {
                    std::cerr << "Failed to open input \"" << arg << "\"" << std::endl;
                    return 1;
                }
            }
            else if (!outputFile.is_open())
            {
                outputFile.open(arg.c_str(), std::ofstream::binary);
                if (!outputFile.is_open())
                {
                    std::cerr << "Failed to open output \"" << arg << "\"" << std::endl;
                    return 1;
                }
            }
            else
                std::cerr << "Illegal command argument \"" << arg << "\"" << std::endl;
        }
        else if (arg.find(InputCenterArg) == 0)
        {
            inputCenterKHz = atof(arg.substr(sizeof(InputCenterArg) - 1).c_str());
            if (inputCenterKHz < 0)
            {
                std::cerr << arg << " cannot be less than zero" << std::endl;
                return 1;
            }
        }
        else if (arg.find(InputStartArg) == 0)
        {
            std::tm t = {};
            std::istringstream iss(arg.substr(sizeof(InputStartArg) - 1));
            iss >> std::get_time(&t, DateFormatDescriptor);
            if (iss.fail())
            {
                std::cerr << "Failed to parse time \"" << arg << "\"" << std::endl;
                return 1;
            }
            inputStartTime = std::chrono::system_clock::from_time_t(mktime(&t));
        }
        else if (arg.find(InputIsFlippedArg) == 0)
            inputIQflipped = true;
        else if (arg.find(OutputCenterKHzArg) == 0)
            outputCenterKHz = atof(arg.substr(sizeof(OutputCenterKHzArg) - 1).c_str());
        else if (arg.find(OutputStartTimeArg) == 0)
        {
            std::tm t = {};
            std::istringstream iss(arg.substr(sizeof(OutputStartTimeArg) - 1));
            iss >> std::get_time(&t, DateFormatDescriptor);
            if (iss.fail())
            {
                std::cerr << "Failed to parse time \"" << arg << "\"" << std::endl;
                return 1;
            }
            outputStartTime = std::chrono::system_clock::from_time_t(mktime(&t));
            outputStartTimeSpecified = true;
        }
        else if (arg.find(OutputIntervalSecondsArg) == 0)
        {
            outputInterval = std::chrono::seconds(atoi(arg.substr(sizeof(OutputIntervalSecondsArg) - 1).c_str()));
            if (outputInterval < std::chrono::seconds(0))
            {
                std::cerr << arg << " cannot be less than zero" << std::endl;
                return 1;
            }
        }
        else if (arg.find(OutputStartSecondsArg) == 0)
        {
            outputStartOffset = std::chrono::seconds(atoi(arg.substr(sizeof(OutputStartSecondsArg) - 1).c_str()));
            if (outputStartOffset < std::chrono::seconds(0))
            {
                std::cerr << arg << " cannot be less than zero" << std::endl;
                return 1;
            }
        }
        else
        {
            std::cerr << "Unrecognized command argument: \"" << arg << "\"" << std::endl;
            return 1;
        }
    }

    // validate command line arguments
    if (!inputFile.is_open())
    {
        std::cerr << "No input file specified" << std::endl;
        usage();
        return 1;
    }
    if (!outputFile.is_open())
    {
        std::cerr << "No output file specified" << std::endl;
        usage();
        return 1;
    }

    static const float MaxOutputDifferenceKhz = INPUT_IQ_SAMPLES_PER_SECOND / 2000.f;
    if (fabs(outputCenterKHz - inputCenterKHz) > MaxOutputDifferenceKhz)
    {
        std::cerr << "Output center frequency of " << outputCenterKHz << " must be within +/-" << MaxOutputDifferenceKhz << "KHz of " << inputCenterKHz << std::endl;
        return 1;
    }

    if (outputStartTimeSpecified)
    {
        if (outputStartTime < inputStartTime)
        {
            std::cerr << "Output start time must be after Input start time" << std::endl;
            return 1;
        }
        outputStartOffset = outputStartTime - inputStartTime;
    }
    else
        outputStartTime = inputStartTime + outputStartOffset;

    unsigned inputFramesToSkip = static_cast<unsigned>(INPUT_IQ_SAMPLES_PER_SECOND * std::chrono::duration_cast<std::chrono::seconds>(outputStartOffset).count());

    unsigned inputFramesToProcess = static_cast<unsigned>(INPUT_IQ_SAMPLES_PER_SECOND * std::chrono::duration_cast<std::chrono::seconds>(outputInterval).count());
    if (inputFramesToProcess == 0) // special case zero to mean process all remaining frames
        inputFramesToProcess = static_cast<unsigned>(-1l);

    return process(inputFile, inputCenterKHz,  inputStartTime,
         inputFramesToSkip,  inputFramesToProcess,  outputFile, outputCenterKHz,
         outputStartTime);
}

namespace Filter_Octave {
    const int SAMPLEFILTER_TAP_NUM(401);
    extern FilterCoeficient_t filter_taps[];
}

namespace {
    const int OUTPUT_CHUNK_FRAME_COUNT = 2048;
    const int STEREO = 2;
    struct NextBuffer {
        virtual ~NextBuffer() {};
        virtual void ProcessChunk(unsigned char* p, unsigned sze) = 0;
        virtual void Finish() = 0;
    };
    
    template <class Sample_t, unsigned SCALE=1>
    class Process : public NextBuffer
    {
    public:
        Process(std::ofstream& outputFile, double mixKhz, double outputCenterKHz,
            std::chrono::system_clock::time_point outputStartTime)
            : m_outputFile(outputFile)
            , m_MixIindex(0)
            , m_MixQindex(0)
            , m_QScale(1)
            , m_bandPassFilters(STEREO) // one for I one for Q 
            , m_outputDecimate(0)
            , m_outputBuffer(OUTPUT_CHUNK_FRAME_COUNT* STEREO)
            , m_outputBufferPosition(0)
            , m_dataChunkByteCountPos(0)
            , m_dataChunkByteCount(0)
        {
            // initialize precomputed sin/cos for the mix to outputCenterKHz
            // populate the sine table.
            // use the same table for cosine but start in different position.
            int mixF = static_cast<int>(mixKhz * 1000);
            bool isNeg = mixKhz < 0;
            if (isNeg)
                mixF = -mixF;
            m_QScale = isNeg ? -1.f : 1.f;

            bool closestOneNeg = PrecomputeSinCos::ComputeSinCos(
                INPUT_IQ_SAMPLES_PER_SECOND, mixF, m_MixCoef, m_MixQindex);
            if (mixF != 0)
            {
                if (closestOneNeg)
                    m_QScale *= -1.f;// flip mixQ summation if we're using upside-down cosine
            }

            // set up bandpass filters. The I and Q are identical to each other
            for (auto& f : m_bandPassFilters)
                f.setFilterDefinition(Filter_Octave::SAMPLEFILTER_TAP_NUM, Filter_Octave::filter_taps);

            // initialize output WAV file
            outputFile.write("RIFF", 4);
            std::vector<char> buf(4);
            outputFile.write(&buf[0], 4);
            outputFile.write("WAVE", 4);
            // FORMATETC
            outputFile.write("fmt ", 4); 
            outputFile.write("\020\0\0\0", 4); // 16 byte chunk size
            outputFile.write("\03\0", 2); // format number 3 -- float samples
            outputFile.write("\02\0", 2); // 2 channels = stereo
            uint32_t rate = OUTPUT_IQ_SAMPLES_PER_SECOND;
            uint16_t bitsPerSample = 8 * sizeof(float);
            uint16_t blockAlign = 2 * sizeof(float);
            uint32_t byteRate = rate * blockAlign;

            buf[0] = static_cast<char>(rate);
            buf[1] = static_cast<char>(rate >> 8);
            buf[2] = static_cast<char>(rate >> 16);
            buf[3] = static_cast<char>(rate >> 24);
            outputFile.write(&buf[0], 4);

            buf[0] = static_cast<char>(byteRate);
            buf[1] = static_cast<char>(byteRate>>8);
            buf[2] = static_cast<char>(byteRate>>16);
            buf[3] = static_cast<char>(byteRate>>24);
            outputFile.write(&buf[0], 4);
            buf[0] = static_cast<char>(blockAlign);
            buf[1] = static_cast<char>(blockAlign >> 8);
            outputFile.write(&buf[0], 2);
            buf[0] = static_cast<char>(bitsPerSample);
            buf[1] = static_cast<char>(bitsPerSample >> 8);
            outputFile.write(&buf[0], 2);
            // FORMATETC done

            // Write stuff in "0SDR" chunk so ReviewRecordedIQ can see what we did here.
            #pragma warning (push)
            #pragma warning (disable: 4996) // VS won't compile std::gmtime cuz its not thread safe. don't care here
            auto ot = std::chrono::system_clock::to_time_t(outputStartTime);
            auto tm = *std::gmtime(&ot);
            #pragma warning (pop)
            std::ostringstream oss;
            oss << "--outputStartTime=" << std::put_time(&tm, DateFormatDescriptor);
            oss << " --outputCenterKHz=" << outputCenterKHz;
            outputFile.write("0SDR", 4); // 0SDR chunk to show the parameters we used
            unsigned SdrChunkSize = static_cast<unsigned>(oss.str().length());
            // make the chunksize a multiple of 16. have no idea if this is necessary,
            // but I am not going to mess up the alignment
            SdrChunkSize += 15;
            SdrChunkSize <<= 4;
            SdrChunkSize >>= 4;
            while (oss.str().length() < SdrChunkSize)
                oss << " ";
            buf[0] = static_cast<char>(SdrChunkSize);
            buf[1] = static_cast<char>(SdrChunkSize>>8);
            buf[2] = static_cast<char>(SdrChunkSize>>16);
            buf[3] = static_cast<char>(SdrChunkSize>>24);
            outputFile.write(&buf[0], buf.size());
            outputFile.write(oss.str().c_str(), SdrChunkSize);

            outputFile.write("data", 4); // start the required, final 'data' chunk
            m_dataChunkByteCountPos = outputFile.tellp();
            memset(&buf[0], 0, 4);
            outputFile.write(&buf[0], 4);
        }

        void ProcessChunk(unsigned char* p, unsigned numFrames)
        {
            // TODO--If we're running on a big-endian machine, the byte-swapping codes of *p go here...
            for (auto q = reinterpret_cast<Sample_t*>(p); numFrames > 0 ; numFrames -= 1)
            {
                float inI = static_cast<float>(*q++);
                float inQ = static_cast<float>(*q++);

                unsigned sze = static_cast<unsigned>(m_MixCoef.size());
                double mixI = m_MixCoef[m_MixIindex++];
                if (m_MixIindex >= sze)
                    m_MixIindex = 0;
                double mixQ = m_MixCoef[m_MixQindex++];
                if (m_MixQindex >= sze)
                    m_MixQindex = 0;
                mixQ *= m_QScale; // flip for negative mixer frequency

                // The mix is a complex multiply
                double nextI = inI * mixI - inQ * mixQ;
                double nextQ = inQ * mixI + inI * mixQ;

                // low pass the mixed I separate from the mixed Q
                m_bandPassFilters[0].applySample(nextI);
                m_bandPassFilters[1].applySample(nextQ);
                m_outputDecimate += 1;
                if (m_outputDecimate >= DECIMATE)
                {
                    m_outputDecimate = 0;
                    auto outI = m_bandPassFilters[0].value();
                    auto outQ = m_bandPassFilters[1].value();
                    // TODO..output buffer must be little endian. Big endian machine must fix.
                    m_outputBuffer[m_outputBufferPosition++] = static_cast<float>(outI);
                    m_outputBuffer[m_outputBufferPosition++] = static_cast<float>(outQ);
                    if (m_outputBufferPosition >= m_outputBuffer.size())
                        writeDataChunk();
                }
            }
        }
        
        void Finish()
        {
            if (m_outputBufferPosition > 0)
                writeDataChunk();

            // RIFF format requires us to seek back into the header of the
            // file and overwrite two different byte counts.

            auto posHere = m_outputFile.tellp();
            uint32_t RiffChunkSize = static_cast<uint32_t>(posHere);
            RiffChunkSize -= 8;
            
            std::vector<char> buf(4);
            buf[0] = static_cast<char>(RiffChunkSize);
            buf[1] = static_cast<char>(RiffChunkSize >> 8);
            buf[2] = static_cast<char>(RiffChunkSize >> 16);
            buf[3] = static_cast<char>(RiffChunkSize >> 24);
            m_outputFile.seekp(4);
            m_outputFile.write(&buf[0], buf.size());

            buf[0] = static_cast<char>(m_dataChunkByteCount);
            buf[1] = static_cast<char>(m_dataChunkByteCount >> 8);
            buf[2] = static_cast<char>(m_dataChunkByteCount >> 16);
            buf[3] = static_cast<char>(m_dataChunkByteCount >> 24);
            m_outputFile.seekp(m_dataChunkByteCountPos);
            m_outputFile.write(&buf[0], buf.size());

            m_outputFile.close();
        }
    private:
        static const double Scale;
        std::ofstream& m_outputFile;
        std::vector<double> m_MixCoef;
        unsigned m_MixIindex;
        unsigned m_MixQindex;
        double m_QScale;
        std::vector<CFIRFilter> m_bandPassFilters;
        unsigned m_outputDecimate;
        unsigned m_outputBufferPosition;
        std::vector<float> m_outputBuffer;
        std::streampos m_dataChunkByteCountPos;
        uint32_t m_dataChunkByteCount;

        void writeDataChunk()
        {
            uint32_t chunkSize = m_outputBufferPosition * sizeof(float);
            m_outputBufferPosition = 0;
            m_outputFile.write(reinterpret_cast<const char*>(&m_outputBuffer[0]), chunkSize);
            m_dataChunkByteCount += chunkSize;
        }
    };

    template <class Sample_t, unsigned SCALE >
    const double Process<Sample_t, SCALE>::Scale = 1.0 / SCALE;

    int process(std::ifstream& inputFile, double inputCenterKHz, std::chrono::system_clock::time_point inputStartTime,
        unsigned inputFramesToSkip, unsigned inputFramesToProcess, std::ofstream& outputFile, double outputCenterKHz,
        std::chrono::system_clock::time_point outputStartTime)
    {
        RiffReader rr(inputFile);

        try {
            rr.ParseHeader();
        }
        catch (const std::exception &e)
        {
            std::cerr << e.what() << std::endl;
            return 1;
        }

        if (rr.get_sampleRate() != INPUT_IQ_SAMPLES_PER_SECOND)
        {
            std::cerr << "Input file at " << rr.get_sampleRate() << " samples per second cannot be processed at " << INPUT_IQ_SAMPLES_PER_SECOND << std::endl;
            return 1;
        }
        if (rr.get_numChannels() != 2)
        {
            std::cerr << "Input file has " << rr.get_numChannels() << " channels, which is not the 2 required." << std::endl;
            return 1;
        }

        std::shared_ptr<NextBuffer> pOutput;

        auto format = rr.get_format();
        auto bitsPerSample = rr.get_bitsPerSample();
        auto blockAlign = rr.get_blockAlign();

        if (format == 1 && bitsPerSample == 16)
            pOutput.reset(new Process<int16_t, 0x7FFFu>(outputFile,  outputCenterKHz- inputCenterKHz, outputCenterKHz,  outputStartTime));
        else if (format == 3 && bitsPerSample == 32)
            pOutput.reset(new Process<float>(outputFile, outputCenterKHz - inputCenterKHz, outputCenterKHz, outputStartTime));
        else {
            std::cerr << "Cannot process format number " << format << " with bits per sample=" << bitsPerSample << std::endl;
            return 1;
        }

        // set up a couple of function objects for the RiffReader to call us back with

        RiffReader::DataChunkFcn_t procFcn = [pOutput, blockAlign, &inputFramesToProcess] (unsigned char *p, unsigned numFrames)
        {
            numFrames = std::min(inputFramesToProcess, numFrames);
            if (numFrames > 0)
            {
                pOutput->ProcessChunk(p, numFrames);
                inputFramesToProcess -= numFrames;
            }
            return inputFramesToProcess != 0;
        };

        RiffReader::DataChunkFcn_t dataFcn = procFcn;
        
        if (inputFramesToSkip)
            dataFcn = [inputFramesToSkip, &rr, &dataFcn, &procFcn] (unsigned char *, unsigned)
            {
                // update the file handle
                rr.SeekToFrameNumber(inputFramesToSkip);
                // overwrite the function object so next call processes after the skip.
                dataFcn = procFcn;
                return true;
            };

        rr.ProcessChunks(dataFcn);

        pOutput->Finish();
        return 0;
    }
}

// past here be filter coefficients

namespace Filter_Octave {
    /*  f = 10500 / 192000
        b = fir1(400, f)
        fd = fopen("d:/temp/10500.txt", "w")
        fprintf(fd, "%10g,\n", b)
        fclose(fd)    
    */
    FilterCoeficient_t filter_taps[SAMPLEFILTER_TAP_NUM] =
    {
9.81549e-05,
0.000113028,
0.000125003,
0.000133747,
0.000138991,
0.000140533,
0.000138247,
0.000132085,
0.000122087,
0.000108378,
9.11761e-05,
7.07902e-05,
4.76195e-05,
2.21512e-05,
-5.0452e-06,
-3.33251e-05,
-6.19794e-05,
-9.02473e-05,
-0.000117332,
-0.000142418,
-0.000164694,
-0.000183369,
-0.000197705,
-0.000207032,
-0.000210779,
-0.000208495,
-0.000199871,
-0.000184762,
-0.000163201,
-0.000135412,
-0.000101821,
-6.3053e-05,
-1.99308e-05,
2.65357e-05,
7.51676e-05,
0.00012464,
0.000173515,
0.000220272,
0.000263358,
0.000301225,
0.000332383,
0.00035545,
0.000369202,
0.000372621,
0.000364943,
0.000345696,
0.000314735,
0.000272269,
0.000218875,
0.000155506,
8.34812e-05,
4.4726e-06,
-7.95297e-05,
-0.000166258,
-0.000253224,
-0.000337777,
-0.000417186,
-0.000488713,
-0.000549702,
-0.000597669,
-0.000630382,
-0.000645954,
-0.000642917,
-0.000620295,
-0.000577656,
-0.000515165,
-0.000433605,
-0.00033439,
-0.000219556,
-9.17314e-05,
4.59135e-05,
0.000189734,
0.000335699,
0.000479496,
0.000616647,
0.000742642,
0.000853072,
0.000943775,
0.00101097,
0.00105141,
0.00106247,
0.00104232,
0.000989951,
0.00090531,
0.000789308,
0.000643857,
0.000471856,
0.000277154,
6.44747e-05,
-0.000160681,
-0.000392166,
-0.000623345,
-0.000847271,
-0.00105688,
-0.0012452,
-0.00140558,
-0.00153186,
-0.00161866,
-0.0016615,
-0.00165701,
-0.0016031,
-0.00149905,
-0.00134562,
-0.00114506,
-0.000901154,
-0.000619154,
-0.000305683,
3.13903e-05,
0.000383135,
0.000739776,
0.00109095,
0.00142598,
0.00173418,
 0.0020052,
0.00222931,
0.00239773,
0.00250293,
0.00253893,
0.00250151,
0.00238844,
0.00219962,
0.00193719,
0.00160553,
0.00121129,
0.000763231,
0.000272113,
-0.000249564,
-0.000787838,
-0.00132763,
-0.00185312,
-0.00234822,
-0.00279696,
-0.00318403,
-0.00349518,
-0.00371776,
-0.0038411,
-0.00385691,
-0.00375964,
-0.0035468,
-0.00321907,
-0.00278054,
-0.00223866,
-0.00160425,
-0.000891279,
-0.000116672,
0.00070006,
0.00153725,
0.00237156,
0.00317861,
0.00393353,
0.00461173,
0.00518951,
0.00564479,
0.00595784,
0.00611187,
0.00609368,
0.00589418,
0.00550887,
0.00493815,
0.00418761,
0.00326811,
0.00219578,
0.000991891,
-0.000317443,
-0.00170168,
-0.0031264,
-0.00455392,
-0.00594414,
-0.00725529,
-0.00844495,
-0.00947096,
-0.0102925,
-0.0108709,
-0.0111709,
-0.0111614,
-0.0108164,
-0.0101158,
-0.00904593,
-0.00760025,
-0.00577968,
-0.00359284,
-0.00105609,
0.00180654,
0.00496366,
0.00837698,
 0.0120019,
 0.0157883,
 0.0196813,
 0.0236227,
 0.0275515,
 0.0314056,
 0.0351226,
 0.0386417,
  0.041904,
 0.0448545,
 0.0474431,
 0.0496252,
  0.051363,
 0.0526265,
 0.0533936,
 0.0536507,
 0.0533936,
 0.0526265,
  0.051363,
 0.0496252,
 0.0474431,
 0.0448545,
  0.041904,
 0.0386417,
 0.0351226,
 0.0314056,
 0.0275515,
 0.0236227,
 0.0196813,
 0.0157883,
 0.0120019,
0.00837698,
0.00496366,
0.00180654,
-0.00105609,
-0.00359284,
-0.00577968,
-0.00760025,
-0.00904593,
-0.0101158,
-0.0108164,
-0.0111614,
-0.0111709,
-0.0108709,
-0.0102925,
-0.00947096,
-0.00844495,
-0.00725529,
-0.00594414,
-0.00455392,
-0.0031264,
-0.00170168,
-0.000317443,
0.000991891,
0.00219578,
0.00326811,
0.00418761,
0.00493815,
0.00550887,
0.00589418,
0.00609368,
0.00611187,
0.00595784,
0.00564479,
0.00518951,
0.00461173,
0.00393353,
0.00317861,
0.00237156,
0.00153725,
0.00070006,
-0.000116672,
-0.000891279,
-0.00160425,
-0.00223866,
-0.00278054,
-0.00321907,
-0.0035468,
-0.00375964,
-0.00385691,
-0.0038411,
-0.00371776,
-0.00349518,
-0.00318403,
-0.00279696,
-0.00234822,
-0.00185312,
-0.00132763,
-0.000787838,
-0.000249564,
0.000272113,
0.000763231,
0.00121129,
0.00160553,
0.00193719,
0.00219962,
0.00238844,
0.00250151,
0.00253893,
0.00250293,
0.00239773,
0.00222931,
 0.0020052,
0.00173418,
0.00142598,
0.00109095,
0.000739776,
0.000383135,
3.13903e-05,
-0.000305683,
-0.000619154,
-0.000901154,
-0.00114506,
-0.00134562,
-0.00149905,
-0.0016031,
-0.00165701,
-0.0016615,
-0.00161866,
-0.00153186,
-0.00140558,
-0.0012452,
-0.00105688,
-0.000847271,
-0.000623345,
-0.000392166,
-0.000160681,
6.44747e-05,
0.000277154,
0.000471856,
0.000643857,
0.000789308,
0.00090531,
0.000989951,
0.00104232,
0.00106247,
0.00105141,
0.00101097,
0.000943775,
0.000853072,
0.000742642,
0.000616647,
0.000479496,
0.000335699,
0.000189734,
4.59135e-05,
-9.17314e-05,
-0.000219556,
-0.00033439,
-0.000433605,
-0.000515165,
-0.000577656,
-0.000620295,
-0.000642917,
-0.000645954,
-0.000630382,
-0.000597669,
-0.000549702,
-0.000488713,
-0.000417186,
-0.000337777,
-0.000253224,
-0.000166258,
-7.95297e-05,
4.4726e-06,
8.34812e-05,
0.000155506,
0.000218875,
0.000272269,
0.000314735,
0.000345696,
0.000364943,
0.000372621,
0.000369202,
0.00035545,
0.000332383,
0.000301225,
0.000263358,
0.000220272,
0.000173515,
0.00012464,
7.51676e-05,
2.65357e-05,
-1.99308e-05,
-6.3053e-05,
-0.000101821,
-0.000135412,
-0.000163201,
-0.000184762,
-0.000199871,
-0.000208495,
-0.000210779,
-0.000207032,
-0.000197705,
-0.000183369,
-0.000164694,
-0.000142418,
-0.000117332,
-9.02473e-05,
-6.19794e-05,
-3.33251e-05,
-5.0452e-06,
2.21512e-05,
4.76195e-05,
7.07902e-05,
9.11761e-05,
0.000108378,
0.000122087,
0.000132085,
0.000138247,
0.000140533,
0.000138991,
0.000133747,
0.000125003,
0.000113028,
9.81549e-05,
    };
}

