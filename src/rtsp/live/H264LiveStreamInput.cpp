//==============================================================================//
// INDUSTRIAL MONITORING AND CONTROL 2014                                       //
//==============================================================================//
//      ___           ___           ___                                         //
//     /\__\         /\  \         /\__\                                        //
//    /:/  /        |::\  \       /:/  /                                        //
//   /:/__/         |:|:\  \     /:/  /                                         //
//  /::\  \ ___   __|:|\:\  \   /:/  /  ___                                     //
// /:/\:\  /\__\ /::::|_\:\__\ /:/__/  /\__\                                    //
// \/__\:\/:/  / \:\~~\  \/__/ \:\  \ /:/  /                                    //
//      \::/  /   \:\  \        \:\  /:/  /                                     //
//      /:/  /     \:\  \        \:\/:/  /                                      //
//     /:/  /       \:\__\        \::/  /                                       //
//     \/__/         \/__/         \/__/                                        //
//                                                                              //
// Webstore and Free Code:                                                      //
// http://www.imc-store.com.au/                                                 //
//                                                                              //
// Industrial/Commercial Information                                            //
// http://imcontrol.com.au/                                                     //
//                                                                              //
//------------------------------------------------------------------------------//
// LIVE555 FFMPEG ENCODER CLASS EXAMPLE                                         //
//------------------------------------------------------------------------------//
// Notes: This C++ program is designed to simplify taking with the FFMPEG       //
// and Live555 library. It provides some basic functions to encode a video from //
// your code and restream it over rtsp.											//
//                                                                              //
// Usage: Use this project as a template for your own program, and play the		//
// RTSP stream using rtsp://127.0.0.1 from VLC.									//
//==============================================================================//

#include "H264LiveStreamInput.hh"

H264LiveStreamInput* H264LiveStreamInput::createNew(UsageEnvironment& env, void *videoEngine) {
	if (!fHaveInitialized) {
		fHaveInitialized = True;
	}

	return new H264LiveStreamInput(env, videoEngine);
}

FramedSource* H264LiveStreamInput::videoSource() {
	if (fOurVideoSource == NULL || H264LiveStreamSource::getRefCount() == 0) {
		fOurVideoSource = H264LiveStreamSource::createNew(envir(), fDevParams);
	}
	return fOurVideoSource;
}

H264LiveStreamInput::H264LiveStreamInput(UsageEnvironment& env, void *videoEngine)
    : Medium(env), fVideoEngine(videoEngine) {
    fDevParams.fVideoEngine = fVideoEngine;
}

H264LiveStreamInput::~H264LiveStreamInput() {
	if (fOurVideoSource != NULL && H264LiveStreamSource::getRefCount() != 0) {
		H264LiveStreamSource::handleClosure(fOurVideoSource);
	}
}

Boolean H264LiveStreamInput::fHaveInitialized = False;
int H264LiveStreamInput::fOurVideoFileNo = -1;
FramedSource* H264LiveStreamInput::fOurVideoSource = NULL;
