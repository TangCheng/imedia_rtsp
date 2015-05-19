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
// Implementation

#include <iostream>
#include <liveMedia.hh>
#include <string.h>
#include "H264LiveStreamServerMediaSubsession.hh"
#include "H264LiveStreamInput.hh"
#include "LiveStreamRTSPServer.hh"

LiveStreamRTSPServer*
LiveStreamRTSPServer::createNew(UsageEnvironment& env, Port ourPort,
			     UserAuthenticationDatabase* authDatabase,
			     unsigned reclamationTestSeconds) {
  int ourSocket = setUpOurSocket(env, ourPort);
  if (ourSocket == -1) return NULL;

  return new LiveStreamRTSPServer(env, ourSocket, ourPort, authDatabase, reclamationTestSeconds);
}

LiveStreamRTSPServer::LiveStreamRTSPServer(UsageEnvironment& env,
                                           int ourSocket, Port ourPort,
                                           UserAuthenticationDatabase* authDatabase,
                                           unsigned reclamationTestSeconds)
  : RTSPServer(env, ourSocket, ourPort, authDatabase, reclamationTestSeconds),
  fStreamInput(HashTable::create(ONE_WORD_HASH_KEYS)),
  fResolutionRE("<resolution>"),
  fChannelRE("<channel>") {
}

LiveStreamRTSPServer::~LiveStreamRTSPServer() {
  // Delete all server media sessions
  H264LiveStreamInput* streamInput;
  while ((streamInput = (H264LiveStreamInput*)fStreamInput->getFirst()) != NULL) {
    //removeServerMediaSession(serverMediaSession); // will delete it, because it no longer has any 'client session' objects using it
  }
  delete fStreamInput;
}

void LiveStreamRTSPServer::addStreamInput(H264LiveStreamInput *streamInput)
{
    StreamChannel chn = streamInput->channelNo();
    removeStreamInput(chn); // in case an existing 'H264LiveStreamInput' with this channel already exists
    fStreamInput->Add((char const*)chn, (void*)streamInput);
}

void LiveStreamRTSPServer::removeStreamInput(H264LiveStreamInput *streamInput)
{
    if (streamInput == NULL) return;

    fStreamInput->Remove((char const*)streamInput->channelNo());
    Medium::close(streamInput);
}

void LiveStreamRTSPServer::removeStreamInput(StreamChannel chn)
{
    removeStreamInput((H264LiveStreamInput*)(fStreamInput->Lookup((char const*)chn)));
}

ServerMediaSession* LiveStreamRTSPServer
::lookupServerMediaSession(char const* streamName, Boolean isFirstLookupInSession) {
  ServerMediaSession *sms = NULL;
  HashTable::Iterator *iter;
  H264LiveStreamInput *streamInput;
  char const* key;

  iter = HashTable::Iterator::create(*fStreamInput);
  while ((streamInput = (H264LiveStreamInput*)(iter->next(key))) != NULL) {
    StreamDescriptor *desc = streamInput->streamDesc();
    if (desc && desc->v_desc.path && desc->v_desc.resolution) {
      char channel[16];
      snprintf(channel, sizeof(channel), "%d", streamInput->channelNo());
      std::string path = desc->v_desc.path;
      fResolutionRE.GlobalReplace(desc->v_desc.resolution, &path);
      fChannelRE.GlobalReplace(channel, &path);

      if (path.compare(streamName) == 0) {
        sms = RTSPServer::lookupServerMediaSession(streamName);
        if (sms == NULL) {
          char const* descriptionString = "Session streamed by \"iRTSP\"";
          sms = ServerMediaSession::createNew(envir(),
                                              streamName,
                                              streamName,
                                              descriptionString);
          sms->addSubsession(H264LiveStreamServerMediaSubsession::createNew(envir(), *streamInput));
          addServerMediaSession(sms);
        }
        break;
      }
    }
  }
  return sms;
}
