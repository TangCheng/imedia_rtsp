//==============================================================================//
// Notes: This C++ program is designed to simplify taking with the FFMPEG       //
// and Live555 library. It provides some basic functions to encode a video from //
// your code and restream it over rtsp.											//
//                                                                              //
// Usage: Use this project as a template for your own program, and play the		//
// RTSP stream using rtsp://127.0.0.1 from VLC.									//
//==============================================================================//

#include "stream_descriptor.h"
#include "LiveStreamInput.hh"

LiveStreamInput* LiveStreamInput::createNew(UsageEnvironment& env, StreamChannel chn, StreamDescriptor *desc) {
	return new LiveStreamInput(env, chn, desc);
}

FramedSource* LiveStreamInput::audioSource() {
    if (1/*fOurAudioSource == NULL*/) {
        fOurAudioSource = LiveAudioStreamSource::createNew(envir(), fChannelNo);
    }
	return fOurAudioSource;
}

FramedSource* LiveStreamInput::videoSource() {
    if (1/*fOurVideoSource == NULL*/) {
        fOurVideoSource = LiveVideoStreamSource::createNew(envir(), fChannelNo);
    }
	return fOurVideoSource;
}

LiveStreamInput::LiveStreamInput(UsageEnvironment& env, StreamChannel chn, StreamDescriptor *desc)
    : Medium(env), fOurVideoSource(NULL), fChannelNo(chn), fStreamDesc(desc)
{
}

LiveStreamInput::~LiveStreamInput() {
	if (fOurVideoSource != NULL) {
		LiveVideoStreamSource::handleClosure(fOurVideoSource);
	}
}
