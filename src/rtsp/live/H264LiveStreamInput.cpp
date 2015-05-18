//==============================================================================//
// Notes: This C++ program is designed to simplify taking with the FFMPEG       //
// and Live555 library. It provides some basic functions to encode a video from //
// your code and restream it over rtsp.											//
//                                                                              //
// Usage: Use this project as a template for your own program, and play the		//
// RTSP stream using rtsp://127.0.0.1 from VLC.									//
//==============================================================================//

#include "stream_descriptor.h"
#include "H264LiveStreamInput.hh"

H264LiveStreamInput* H264LiveStreamInput::createNew(UsageEnvironment& env, StreamChannel chn, StreamDescriptor *desc) {
	return new H264LiveStreamInput(env, chn, desc);
}

FramedSource* H264LiveStreamInput::videoSource() {
    if (1/*fOurVideoSource == NULL*/) {
        fOurVideoSource = H264LiveStreamSource::createNew(envir(), fChannelNo);
    }
	return fOurVideoSource;
}

H264LiveStreamInput::H264LiveStreamInput(UsageEnvironment& env, StreamChannel chn, StreamDescriptor *desc)
    : Medium(env), fOurVideoSource(NULL), fChannelNo(chn), fStreamDesc(desc)
{
}

H264LiveStreamInput::~H264LiveStreamInput() {
	if (fOurVideoSource != NULL) {
		H264LiveStreamSource::handleClosure(fOurVideoSource);
	}
}
