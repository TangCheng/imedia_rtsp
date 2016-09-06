/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// "liveMedia"
// Copyright (c) 1996-2013 Live Networks, Inc.  All rights reserved.
// A 'ServerMediaSubsession' object that creates new, unicast, "RTPSink"s
// on demand, from a H264 video file.
// Implementation

#include "stream_descriptor.h"
#include "LiveStreamServerMediaSubsession.hh"
#include "LiveStreamSource.hh"
#include <SimpleRTPSink.hh>
#include <H264VideoRTPSink.hh>
#include <H264VideoStreamDiscreteFramer.hh>

LiveVideoStreamServerMediaSubsession*
LiveVideoStreamServerMediaSubsession::createNew(UsageEnvironment& env,
					      LiveStreamInput& streamInput) {
  return new LiveVideoStreamServerMediaSubsession(env, streamInput);
}

LiveVideoStreamServerMediaSubsession::LiveVideoStreamServerMediaSubsession(UsageEnvironment& env,
								       LiveStreamInput& streamInput)
    : OnDemandServerMediaSubsession(env, True /* always reuse the first source */),
      fStreamSource(NULL), fAuxSDPLine(NULL), fDoneFlag(0), fDummyRTPSink(NULL),
      fLiveStreamInput(streamInput) {
}

LiveVideoStreamServerMediaSubsession::~LiveVideoStreamServerMediaSubsession() {
  delete[] fAuxSDPLine;
}

static void afterPlayingDummy(void* clientData) {
  LiveVideoStreamServerMediaSubsession* subsess = (LiveVideoStreamServerMediaSubsession*)clientData;
  subsess->afterPlayingDummy1();
}

void LiveVideoStreamServerMediaSubsession::afterPlayingDummy1() {
  // Unschedule any pending 'checking' task:
  envir().taskScheduler().unscheduleDelayedTask(nextTask());
  // Signal the event loop that we're done:
  setDoneFlag();
}

static void checkForAuxSDPLine(void* clientData) {
  LiveVideoStreamServerMediaSubsession* subsess = (LiveVideoStreamServerMediaSubsession*)clientData;
  subsess->checkForAuxSDPLine1();
}

void LiveVideoStreamServerMediaSubsession::checkForAuxSDPLine1() {
  char const* dasl;

  if (fAuxSDPLine != NULL) {
    // Signal the event loop that we're done:
    setDoneFlag();
  } else if (fDummyRTPSink != NULL && (dasl = fDummyRTPSink->auxSDPLine()) != NULL) {
    fAuxSDPLine = strDup(dasl);
    fDummyRTPSink = NULL;

    // Signal the event loop that we're done:
    setDoneFlag();
  } else {
    // try again after a brief delay:
    int uSecsToDelay = 100000; // 100 ms
    nextTask() = envir().taskScheduler().scheduleDelayedTask(uSecsToDelay,
			      (TaskFunc*)checkForAuxSDPLine, this);
  }
}

char const* LiveVideoStreamServerMediaSubsession::getAuxSDPLine(RTPSink* rtpSink, FramedSource* inputSource) {
  if (fAuxSDPLine != NULL) return fAuxSDPLine; // it's already been set up (for a previous client)

  if (fDummyRTPSink == NULL) { // we're not already setting it up for another, concurrent stream
    // Note: For H264 video files, the 'config' information ("profile-level-id" and "sprop-parameter-sets") isn't known
    // until we start reading the file.  This means that "rtpSink"s "auxSDPLine()" will be NULL initially,
    // and we need to start reading data from our file until this changes.
    fDummyRTPSink = rtpSink;

    // Start reading the file:
    fDummyRTPSink->startPlaying(*inputSource, afterPlayingDummy, this);

    // Check whether the sink's 'auxSDPLine()' is ready:
    checkForAuxSDPLine(this);
  }

  envir().taskScheduler().doEventLoop(&fDoneFlag);

  return fAuxSDPLine;
}

FramedSource* LiveVideoStreamServerMediaSubsession::createNewStreamSource(unsigned /*clientSessionId*/, unsigned& estBitrate) {
  estBitrate = 5000; // kbps, estimate

  // StreamSource has already been created
  if (fStreamSource) {
    LiveVideoStreamSource *liveSource =
		  dynamic_cast<LiveVideoStreamSource*>(fStreamSource->inputSource());
    liveSource->referenceCount()++;
    return fStreamSource;
  }
 
  // Create a framer for the Video Elementary Stream:
  fStreamSource = H264VideoStreamDiscreteFramer::createNew(envir(), fLiveStreamInput.videoSource());
  return fStreamSource;
}

void LiveVideoStreamServerMediaSubsession::
closeStreamSource(FramedSource *inputSource)
{
  // Sanity check, should not happend
  if (fStreamSource != inputSource) {
    Medium::close(inputSource);
    return;
  }

  LiveVideoStreamSource *liveSource = 
      dynamic_cast<LiveVideoStreamSource*>(fStreamSource->inputSource());
  if (--liveSource->referenceCount() == 0) {
    Medium::close(fStreamSource);
    fStreamSource = NULL;
  }
}

RTPSink* LiveVideoStreamServerMediaSubsession
::createNewRTPSink(Groupsock* rtpGroupsock,
		   unsigned char rtpPayloadTypeIfDynamic,
		   FramedSource* /*inputSource*/) {
  H264VideoRTPSink *rtp_sink = H264VideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
  rtp_sink->setPacketSizes(1000, 1456 * 10);
  return rtp_sink;
}


////////////////////////////////////////////////////////////////////////////////
// Audio
////////////////////////////////////////////////////////////////////////////////

LiveAudioStreamServerMediaSubsession* LiveAudioStreamServerMediaSubsession
::createNew(UsageEnvironment& env, LiveStreamInput& streamInput)
{
    return new LiveAudioStreamServerMediaSubsession(env, streamInput);
}

LiveAudioStreamServerMediaSubsession
::LiveAudioStreamServerMediaSubsession(UsageEnvironment& env, LiveStreamInput& streamInput)
    : OnDemandServerMediaSubsession(env, True /* always reuse the first source */),
      fLiveStreamInput(streamInput)
{
}

LiveAudioStreamServerMediaSubsession::~LiveAudioStreamServerMediaSubsession()
{
}

FramedSource* LiveAudioStreamServerMediaSubsession
::createNewStreamSource(unsigned clientSessionId, unsigned& estBitrate)
{
    estBitrate = 64; // kbps, estimate

    // Create the audio source:
	return fLiveStreamInput.audioSource();
}

RTPSink* LiveAudioStreamServerMediaSubsession
::createNewRTPSink(Groupsock* rtpGroupsock,
                   unsigned char rtpPayloadTypeIfDynamic,
				   FramedSource* inputSource)
{
    uint32_t samplerate = 8000;//fAudioStream.getSampleRate();
    uint32_t nr_chans = 1;
    const char *mimeType = "PCMU";
    unsigned char payloadFormatCode = rtpPayloadTypeIfDynamic;

#if 0
    IAudioEncoder::EncodingType encoding;
    encoding = fAudioStream.getEncoding();
    switch (encoding) {
    case IAudioEncoder::ADPCM:
        mimeType = "DVI4";
        break;
    case IAudioEncoder::LPCM:
        mimeType = "L16";
        break;
    case IAudioEncoder::G711A:
        mimeType = "PCMA";
        break;
    case IAudioEncoder::G711U:
        mimeType = "PCMU";
        break;
    case IAudioEncoder::G726:
        mimeType = "G726-40";
        break;
    }
#endif

	RTPSink *rtp_sink
        = SimpleRTPSink::createNew(envir(), rtpGroupsock,
                                   payloadFormatCode, samplerate,
                                   "audio", mimeType, nr_chans);
    return rtp_sink;
}
