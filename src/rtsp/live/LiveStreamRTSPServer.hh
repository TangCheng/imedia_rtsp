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
// Copyright (c) 1996-2015, Live Networks, Inc.  All rights reserved
// A subclass of "RTSPServer" that creates "ServerMediaSession"s on demand,
// based on whether or not the specified stream name exists as a file
// Header file

#ifndef _LIVE_STREAM_RTSP_SERVER_HH
#define _LIVE_STREAM_RTSP_SERVER_HH

class LiveStreamRTSPServer: public RTSPServer {
public:
  static LiveStreamRTSPServer* createNew(UsageEnvironment& env, Port ourPort = 554,
                                         UserAuthenticationDatabase* authDatabase = NULL,
                                         unsigned reclamationTestSeconds = 65);

  void addStreamInput(H264LiveStreamInput *streamInput);
  void removeStreamInput(StreamChannel chn);
  void removeStreamInput(H264LiveStreamInput *streamInput);

protected:
  LiveStreamRTSPServer(UsageEnvironment& env,
                       int ourSocket, Port ourPort,
                       UserAuthenticationDatabase* authDatabase,
                       unsigned reclamationTestSeconds);
  // called only by createNew();
  virtual ~LiveStreamRTSPServer();

protected: // redefined virtual functions
  virtual ServerMediaSession*
  lookupServerMediaSession(char const* streamName, Boolean isFirstLookupInSession);

private:
  HashTable* fStreamInput;
};

#endif
