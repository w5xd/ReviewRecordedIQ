#pragma once
namespace XD {
	struct AudioSink {
		virtual bool  AddMonoSoundFrames(const short *p, unsigned frameCount) = 0;
		virtual void  AudioComplete() = 0; // end of wav file playback
		virtual void  ReleaseSink() = 0; // free memory. do not access after this.
	};

}
