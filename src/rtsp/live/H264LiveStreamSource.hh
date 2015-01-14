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

#ifndef _H264_LIVE_STREAME_SOURCE_HH
#define _H264_LIVE_STREAME_SOURCE_HH

#ifndef _FRAMED_SOURCE_HH
#include <FramedSource.hh>
#endif

// The following class can be used to define specific encoder parameters
class H264LiveStreamParameters {
  //%%% TO BE WRITTEN %%%
public:
	StreamChannel fChannelNo;
    void *fVideoEngine;
};

class H264LiveStreamSource: public FramedSource
{
public:
    static H264LiveStreamSource* createNew(UsageEnvironment& env, H264LiveStreamParameters params);
    //static unsigned getRefCount();

public:
    EventTriggerId eventTriggerId; 
    // Note that this is defined here to be a static class variable, because this code is intended to illustrate how to
    // encapsulate a *single* device - not a set of devices.
    // You can, however, redefine this to be a non-static member variable.

protected:
    H264LiveStreamSource(UsageEnvironment& env, H264LiveStreamParameters params);
    // called only by createNew(), or by subclass constructors
    virtual ~H264LiveStreamSource();

private:
    // redefined virtual functions:
    virtual void doGetNextFrame();
    //virtual void doStopGettingFrames(); // optional
    static void deliverFrame0(void* clientData);
    void deliverFrame();

private:
    //static unsigned referenceCount; // used to count how many instances of this class currently exist
    bool firstDeliverFrame;
    H264LiveStreamParameters fParams;
    int vencFd;
};

#endif
