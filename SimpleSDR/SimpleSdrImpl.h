/* Copyright (c) 2022, Wayne Wright, W5XD. All rights reserved. */
#pragma once
#include <string>
#include <memory>
namespace XDSdr {
    namespace impl {
        class SimpleSDRImpl;
        
        class SimpleSDR {
        public:
            enum SdrDecodeBandwidth { NARROW_CW, WIDE_CW, NARROW_SSB, WIDE_SSB, UNINITIALIZED};
            SimpleSDR(const std::string &fileName, void *);
            void Close();
            void Play();
            void Pause();
            float GetPlayLengthSeconds();
            float GetIfBoundaryAbsHz();
            float GetPlayPositionSeconds();
            void SetPlayPositionSeconds(float);
            float GetRxFrequencyCenterHz();
            void SetRxFrequencyCenterHz(float);
            float GetRxFrequencyBfoOffsetHz();
            void SetRxFrequencyBfoOffsetHz(float);
            std::string FromSliceIQ();
            SdrDecodeBandwidth GetBandwidth();
            void SetBandwidth(SdrDecodeBandwidth);
        protected:
            std::shared_ptr<SimpleSDRImpl> m_impl;
        };
    }
}

