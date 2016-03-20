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
// on demand, from a H264 Elementary Stream video file.
// C++ header

#ifndef _LIVE_STREAM_SERVER_MEDIA_SUBSESSION_HH
#define _LIVE_STREAM_SERVER_MEDIA_SUBSESSION_HH

#ifndef _ON_DEMAND_SERVER_MEDIA_SUBSESSION_HH
#include <OnDemandServerMediaSubsession.hh>
#endif
#include "LiveStreamInput.hh"

class LiveVideoStreamServerMediaSubsession: public OnDemandServerMediaSubsession {
public:
  static LiveVideoStreamServerMediaSubsession*
  createNew(UsageEnvironment& env, LiveStreamInput& streamInput);

  // Used to implement "getAuxSDPLine()":
  void checkForAuxSDPLine1();
  void afterPlayingDummy1();

protected:
  LiveStreamInput &fLiveStreamInput;
  LiveVideoStreamServerMediaSubsession(UsageEnvironment& env, LiveStreamInput& streamInput);
      // called only by createNew();
  virtual ~LiveVideoStreamServerMediaSubsession();

  void setDoneFlag() { fDoneFlag = ~0; }

protected: // redefined virtual functions
  virtual char const* getAuxSDPLine(RTPSink* rtpSink,
				    FramedSource* inputSource);

  virtual FramedSource* createNewStreamSource(unsigned clientSessionId,
					      unsigned& estBitrate);
  virtual RTPSink* createNewRTPSink(Groupsock* rtpGroupsock,
                                    unsigned char rtpPayloadTypeIfDynamic,
				    FramedSource* inputSource);

private:
  char* fAuxSDPLine;
  char fDoneFlag; // used when setting up "fAuxSDPLine"
  RTPSink* fDummyRTPSink; // ditto
};


////////////////////////////////////////////////////////////////////////////////
// Audio
////////////////////////////////////////////////////////////////////////////////

class LiveAudioStreamServerMediaSubsession: public OnDemandServerMediaSubsession {
public:
  static LiveAudioStreamServerMediaSubsession*
  createNew(UsageEnvironment& env, LiveStreamInput& streamInput);

protected:
  LiveAudioStreamServerMediaSubsession(UsageEnvironment& env, LiveStreamInput& streamInput);
  virtual ~LiveAudioStreamServerMediaSubsession();

protected: // redefined virtual functions
  virtual FramedSource* createNewStreamSource(unsigned clientSessionId,
					      unsigned& estBitrate);
  virtual RTPSink* createNewRTPSink(Groupsock* rtpGroupsock,
                                    unsigned char rtpPayloadTypeIfDynamic,
				    FramedSource* inputSource);

private:
  LiveStreamInput &fLiveStreamInput;
};

#endif
