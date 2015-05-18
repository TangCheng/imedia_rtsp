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

#include <H264VideoRTPSink.hh>
#include <H264VideoStreamDiscreteFramer.hh>
#include "imedia.h"
#include "H264LiveStreamSource.hh"
#include "LiveStreamServerMediaSession.hh"

H264LiveStreamServerMediaSubsession*
H264LiveStreamServerMediaSubsession::createNew(UsageEnvironment& env, StreamChannel chn) {
  return new H264LiveStreamServerMediaSubsession(env, chn);
}

H264LiveStreamServerMediaSubsession::
H264LiveStreamServerMediaSubsession(UsageEnvironment& env,
                                    H264LiveStreamInput& input)
    : OnDemandServerMediaSubsession(env, True /* always reuse the first source */),
      fAuxSDPLine(NULL), fDoneFlag(0), fDummyRTPSink(NULL),
    fChannelNo(chn) {
}

H264LiveStreamServerMediaSubsession::~H264LiveStreamServerMediaSubsession() {
  delete[] fAuxSDPLine;
}

static void afterPlayingDummy(void* clientData) {
  H264LiveStreamServerMediaSubsession* subsess = (H264LiveStreamServerMediaSubsession*)clientData;
  subsess->afterPlayingDummy1();
}

void H264LiveStreamServerMediaSubsession::afterPlayingDummy1() {
  // Unschedule any pending 'checking' task:
  envir().taskScheduler().unscheduleDelayedTask(nextTask());
  // Signal the event loop that we're done:
  setDoneFlag();
}

static void checkForAuxSDPLine(void* clientData) {
  H264LiveStreamServerMediaSubsession* subsess = (H264LiveStreamServerMediaSubsession*)clientData;
  subsess->checkForAuxSDPLine1();
}

void H264LiveStreamServerMediaSubsession::checkForAuxSDPLine1() {
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

char const* H264LiveStreamServerMediaSubsession::getAuxSDPLine(RTPSink* rtpSink, FramedSource* inputSource) {
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

FramedSource* H264LiveStreamServerMediaSubsession::createNewStreamSource(unsigned /*clientSessionId*/, unsigned& estBitrate) {
  estBitrate = 5000; // kbps, estimate

  // Create the video source:
  H264LiveStreamSource *video_source = H264LiveStreamSource::createNew(envir(), fChannelNo);
 
  // Create a framer for the Video Elementary Stream:
  return H264VideoStreamDiscreteFramer::createNew(envir(), video_source);
}

RTPSink* H264LiveStreamServerMediaSubsession
::createNewRTPSink(Groupsock* rtpGroupsock,
		   unsigned char rtpPayloadTypeIfDynamic,
		   FramedSource* /*inputSource*/) {
  H264VideoRTPSink *rtp_sink = H264VideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
  rtp_sink->setPacketSizes(1000, 1456 * 10);
  return rtp_sink;
}

