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
// a MediaSource get h264 live stream from hardware
// 
// NOTE: Sections of this code labeled "%%% TO BE WRITTEN %%%" are incomplete, and needto be written by the programmer
// (depending on the features of the particulardevice).
// C++ header

#ifndef _LIVE_STREAME_SOURCE_HH
#define _LIVE_STREAME_SOURCE_HH

#include "stream_descriptor.h"

#ifndef _FRAMED_SOURCE_HH
#include <FramedSource.hh>
#endif

class LiveVideoStreamSource: public FramedSource
{
public:
    static LiveVideoStreamSource* createNew(UsageEnvironment& env, StreamChannel chn);

protected:
    LiveVideoStreamSource(UsageEnvironment& env, StreamChannel chn);
    // called only by createNew(), or by subclass constructors
    virtual ~LiveVideoStreamSource();

private:
    // redefined virtual functions:
    virtual void doGetNextFrame();
    //virtual void doStopGettingFrames(); // optional
	static void deliverFrame0(void *clientData, int mask);
    void deliverFrame();

private:
    StreamChannel fChannelNo;
    int vencFd;
};

class LiveAudioStreamSource: public FramedSource
{
public:
    static LiveAudioStreamSource* createNew(UsageEnvironment& env, StreamChannel chn);

protected:
    LiveAudioStreamSource(UsageEnvironment& env, StreamChannel chn);
    // called only by createNew(), or by subclass constructors
    virtual ~LiveAudioStreamSource();

private:
    // redefined virtual functions:
    virtual void doGetNextFrame();
    //virtual void doStopGettingFrames(); // optional
    static void deliverFrame0(void* clientData, int mask);
    void deliverFrame();

private:
	StreamChannel fChannelNo;
	int aencFd;
};

#endif
