/* Copyright (c) 2022, Wayne Wright, W5XD. All rights reserved. */
#pragma once
namespace XDSdr {
    namespace impl {
        class SimpleSDR;
    }
    public enum class SdrDecodeBandwidth { NARROW_CW, WIDE_CW, NARROW_SSB, WIDE_SSB};
    public ref class SimpleSDR
    {
    public:
        SimpleSDR(System::String^ sourceFile, System::IntPtr audioSink);
        ~SimpleSDR();
        !SimpleSDR();
        void Play();
        void Pause();
        property float PlayLengthSeconds { float get(); }
        property float IfBoundaryAbsHz { float get(); }
        property float PlayPositionSeconds { float get(); void set(float); }
        property float RxFrequencyCenterHz { float get(); void set(float); }
        property float RxFrequencyBfoOffsetHz {float get(); void set(float); }
        property System::String^ FromSliceIQ { System::String ^get();}
        property SdrDecodeBandwidth Bandwidth { SdrDecodeBandwidth get(); void set(SdrDecodeBandwidth); }
        void Close();
    private:
        impl::SimpleSDR* m_impl;
    };
};

