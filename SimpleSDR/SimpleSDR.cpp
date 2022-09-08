#include "pch.h"
#include "SimpleSDR.h"
#include "SimpleSdrImpl.h"

namespace XDSdr {
    SimpleSDR::SimpleSDR(System::String ^SourceFile, System::IntPtr audioSink)
    {
        if (audioSink.ToPointer() == 0)
            throw gcnew System::Exception("SimpleSDR requires an Audio Sink");
        try {
            m_impl = new impl::SimpleSDR(msclr::interop::marshal_as<std::string>(SourceFile), audioSink.ToPointer());
        }
        catch (std::exception& e)
        {
            throw gcnew System::Exception(gcnew System::String(e.what()));
        }
    }

    SimpleSDR::~SimpleSDR()
    {        this->!SimpleSDR();    }

    SimpleSDR::!SimpleSDR()
    {        Close();    }

    void SimpleSDR::Close()
    {
        if (m_impl)
            m_impl->Close();
        delete m_impl;
        m_impl = 0;
    }

    void SimpleSDR::Play()
    {
        m_impl->Play();
    }

    void SimpleSDR::Pause()
    {
        m_impl->Pause();
    }

    float SimpleSDR::PlayLengthSeconds::get()
    {
        return m_impl->GetPlayLengthSeconds();
    }

    float SimpleSDR::IfBoundaryAbsHz::get()
    {
        return m_impl->GetIfBoundaryAbsHz();
    }

    float SimpleSDR::PlayPositionSeconds::get()
    {
        return m_impl->GetPlayPositionSeconds();
    }

    void SimpleSDR::PlayPositionSeconds::set(float v)
    {
        return m_impl->SetPlayPositionSeconds(v);
    }

    float SimpleSDR::RxFrequencyCenterHz::get()
    {
        return m_impl->GetRxFrequencyCenterHz();
    }

    void SimpleSDR::RxFrequencyCenterHz::set(float v)
    {
        return m_impl->SetRxFrequencyCenterHz(v);
    }
    float SimpleSDR::RxFrequencyBfoOffsetHz::get()
    {
        return m_impl->GetRxFrequencyBfoOffsetHz();
    }

    void SimpleSDR::RxFrequencyBfoOffsetHz::set(float v)
    {
        return m_impl->SetRxFrequencyBfoOffsetHz(v);
    }

    SdrDecodeBandwidth SimpleSDR::Bandwidth::get()
    {
        return static_cast<SdrDecodeBandwidth>(static_cast<int>(m_impl->GetBandwidth()));
    }

    void SimpleSDR::Bandwidth::set(SdrDecodeBandwidth v)
    {
        return m_impl->SetBandwidth(static_cast<impl::SimpleSDR::SdrDecodeBandwidth>(static_cast<int>(v)));
    }

    System::String^ SimpleSDR::FromSliceIQ::get  ()
    {
        return msclr::interop::marshal_as<System::String^>(m_impl->FromSliceIQ());
    }

}
