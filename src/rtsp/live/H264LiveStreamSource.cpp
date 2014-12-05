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
// A template for a MediaSource encapsulating an audio/video input device
//
// NOTE: Sections of this code labeled "%%% TO BE WRITTEN %%%" are incomplete, and need to be written by the programmer
// (depending on the features of the particular device).
// Implementation

#include <MediaSink.hh>
#include <hi_mem.h>
#include "H264LiveStreamSource.hh"
#include "stream_descriptor.h"
#include "interface/media_video_interface.h"

H264LiveStreamSource*
H264LiveStreamSource::createNew(UsageEnvironment& env, H264LiveStreamParameters params)
{
    return new H264LiveStreamSource(env, params);
}

EventTriggerId H264LiveStreamSource::eventTriggerId = 0;
unsigned H264LiveStreamSource::referenceCount = 0;
bool H264LiveStreamSource::firstDeliverFrame = True;

H264LiveStreamSource::H264LiveStreamSource(UsageEnvironment& env, H264LiveStreamParameters params)
    : FramedSource(env), fParams(params)
{
    if (referenceCount == 0) {
        // Any global initialization of the device would be done here:
        //%%% TO BE WRITTEN %%%
        //fTmpFile = fopen("./test.data", "w");
    }
    ++referenceCount;
    ipcam_ivideo_register_rtsp_source((IpcamIVideo *)fParams.fVideoEngine, (void *)this);

    // Any instance-specific initialization of the device would be done here:
    //%%% TO BE WRITTEN %%%

    // We arrange here for our "deliverFrame" member function to be called
    // whenever the next frame of data becomes available from the device.
    //
    // If the device can be accessed as a readable socket, then one easy way to do this is using a call to
    //     envir().taskScheduler().turnOnBackgroundReadHandling( ... )
    // (See examples of this call in the "liveMedia" directory.)
    //
    // If, however, the device *cannot* be accessed as a readable socket, then instead we can implement it using 'event triggers':
    // Create an 'event trigger' for this device (if it hasn't already been done):
    if (eventTriggerId == 0) {
         eventTriggerId = envir().taskScheduler().createEventTrigger(deliverFrame0);
    }
}

H264LiveStreamSource::~H264LiveStreamSource() {
    // Any instance-specific 'destruction' (i.e., resetting) of the device would be done here:
    //%%% TO BE WRITTEN %%%
    ipcam_ivideo_unregister_rtsp_source((IpcamIVideo *)fParams.fVideoEngine, (void *)this);
    --referenceCount;
    //fflush(fTmpFile);
    //fclose(fTmpFile);
    if (referenceCount == 0)
    {
        // Any global 'destruction' (i.e., resetting) of the device would be done here:
        //%%% TO BE WRITTEN %%%
         
        // Reclaim our 'event trigger'
        envir().taskScheduler().deleteEventTrigger(eventTriggerId);
        eventTriggerId = 0;
        firstDeliverFrame = True;
    } // Any instance-specific 'destruction' (i.e., resetting) of the device would be done here:
}

unsigned H264LiveStreamSource::getRefCount()
{
    return referenceCount;
}

void H264LiveStreamSource::doGetNextFrame() {
     // This function is called (by our 'downstream' object) when it asks for new data.

     // Note: If, for some reason, the source device stops being readable (e.g., it gets closed), then you do the following:
     if (0 /* the source stops being readable */ /*%%% TO BE WRITTEN %%%*/) {
          handleClosure(this);
          return;
     }

     // If a new frame of data is immediately available to be delivered, then do this now:
     if (ipcam_ivideo_has_video_data((IpcamIVideo *)fParams.fVideoEngine)) /* a new frame of data is immediately available to be delivered*/ /*%%% TO BE WRITTEN %%%*/
     {
          deliverFrame();
     }

     // No new data is immediately available to be delivered.  We don't do anything more here.
     // Instead, our event trigger must be called (e.g., from a separate thread) when new data becomes available.
}

void H264LiveStreamSource::deliverFrame0(void* clientData) {
     if (clientData)
          ((H264LiveStreamSource*)clientData)->deliverFrame();
}

void H264LiveStreamSource::deliverFrame() {
     // This function is called when new frame data is available from the device.
     // We deliver this data by copying it to the 'downstream' object, using the following parameters (class members):
     // 'in' parameters (these should *not* be modified by this function):
     //     fTo: The frame data is copied to this address.
     //         (Note that the variable "fTo" is *not* modified.  Instead,
     //          the frame data is copied to the address pointed to by "fTo".)
     //     fMaxSize: This is the maximum number of bytes that can be copied
     //         (If the actual frame is larger than this, then it should
     //          be truncated, and "fNumTruncatedBytes" set accordingly.)
     // 'out' parameters (these are modified by this function):
     //     fFrameSize: Should be set to the delivered frame size (<= fMaxSize).
     //     fNumTruncatedBytes: Should be set iff the delivered frame would have been
     //         bigger than "fMaxSize", in which case it's set to the number of bytes
     //         that have been omitted.
     //     fPresentationTime: Should be set to the frame's presentation time
     //         (seconds, microseconds).  This time must be aligned with 'wall-clock time' - i.e., the time that you would get
     //         by calling "gettimeofday()".
     //     fDurationInMicroseconds: Should be set to the frame's duration, if known.
     //         If, however, the device is a 'live source' (e.g., encoded from a camera or microphone), then we probably don't need
     //         to set this variable, because - in this case - data will never arrive 'early'.
     // Note the code below.

     if (!isCurrentlyAwaitingData()) return; // we're not ready for the data yet

     unsigned int newFrameSize = 0; //%%% TO BE WRITTEN %%%
     StreamData *data = (StreamData *)ipcam_ivideo_get_video_data((IpcamIVideo *)fParams.fVideoEngine);

     if (data && data->magic == 0xdeadbeef)
     {
          if (data->isIFrame || !firstDeliverFrame)
          {
              firstDeliverFrame = False;
              newFrameSize = data->len;
              //gettimeofday(&fPresentationTime, NULL); // If you have a more accurate time - e.g., from an encoder - then use that instead.
              fPresentationTime = data->pts;
              //printf("%lu.%06lu\n", fPresentationTime.tv_sec, fPresentationTime.tv_usec);

              // Deliver the data here:
              if (newFrameSize > fMaxSize)
              {
                   fFrameSize = fMaxSize;
                   fNumTruncatedBytes = newFrameSize - fMaxSize;
              } else
              {
                   fFrameSize = newFrameSize;
              }
              // If the device is *not* a 'live source' (e.g., it comes instead from a file or buffer), then set "fDurationInMicroseconds" here.
              memcpy(fTo, data->data, fFrameSize);
              //fwrite(data->data, 1, fFrameSize, fTmpFile);
          }
     }
     if (data)
     {
          data->magic = 0x0;
          data->pts.tv_sec = 0;
          data->pts.tv_usec = 0;
          data->len = 0;
          ipcam_ivideo_release_video_data((IpcamIVideo *)fParams.fVideoEngine, data);
     }

     // After delivering the data, inform the reader that it is now available:
     if (newFrameSize > 0)
          FramedSource::afterGetting(this);
          
}

// The following code would be called to signal that a new frame of data has become available.
// This (unlike other "LIVE555 Streaming Media" library code) may be called from a separate thread.
// (Note, however, that "triggerEvent()" cannot be called with the same 'event trigger id' from different threads.
// Also, if you want to have multiple device threads, each one using a different 'event trigger id', then you will need
// to make "eventTriggerId" a non-static member variable of "H264LiveStreamSource".)
/*
void signalNewFrameData(H264LiveStreamSource *ourDevice)
{
    TaskScheduler* ourScheduler = NULL; //%%% TO BE WRITTEN %%%
    //H264LiveStreamSource* ourDevice  = NULL; //%%% TO BE WRITTEN %%%
    ourScheduler = &(ourDevice->envir().taskScheduler());

    if (ourScheduler != NULL)
    { // sanity check
        ourScheduler->triggerEvent(H264LiveStreamSource::eventTriggerId, ourDevice);
    }
}
*/
