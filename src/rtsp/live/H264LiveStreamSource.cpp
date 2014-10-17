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
#include "H264LiveStreamSource.hh"
#include "mpi_venc.h"
#include "hi_mem.h"

H264LiveStreamSource*
H264LiveStreamSource::createNew(UsageEnvironment& env)
{
    return new H264LiveStreamSource(env);
}

H264LiveStreamSource::H264LiveStreamSource(UsageEnvironment& env)
    : FramedSource(env)
{
    mFD = HI_MPI_VENC_GetFd(0);
    envir().taskScheduler().turnOnBackgroundReadHandling(mFD,
                                                         (TaskScheduler::BackgroundHandlerProc*)&deliverFrame0, this);
}

H264LiveStreamSource::~H264LiveStreamSource() {
    // Any instance-specific 'destruction' (i.e., resetting) of the device would be done here:
    //%%% TO BE WRITTEN %%%
    envir().taskScheduler().turnOffBackgroundReadHandling(mFD);
    
    // Any global 'destruction' (i.e., resetting) of the device would be done here:
    //%%% TO BE WRITTEN %%%
    // Reclaim our 'event trigger'
}

void H264LiveStreamSource::doGetNextFrame() {
     // This function is called (by our 'downstream' object) when it asks for new data.

     // Note: If, for some reason, the source device stops being readable (e.g., it gets closed), then you do the following:
     if (0 /* the source stops being readable */ /*%%% TO BE WRITTEN %%%*/) {
          handleClosure(this);
          return;
     }

     // If a new frame of data is immediately available to be delivered, then do this now:
     if (0 /* a new frame of data is immediately available to be delivered*/ /*%%% TO BE WRITTEN %%%*/) {
          deliverFrame();
     }

     // No new data is immediately available to be delivered.  We don't do anything more here.
     // Instead, our event trigger must be called (e.g., from a separate thread) when new data becomes available.
}

void H264LiveStreamSource::deliverFrame0(void* clientData, int mask) {
     if (clientData)
          ((H264LiveStreamSource*)clientData)->deliverFrame();
}

#define MIN(X, Y)                               \
     ({                                         \
          __typeof__ (X) __x = (X);             \
          __typeof__ (Y) __y = (Y);             \
          (__x < __y) ? __x : __y;              \
     })

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

     VENC_CHN_STAT_S stStat;
     VENC_STREAM_S stStream;
     HI_S32 s32Ret;
     static HI_BOOL first_time = HI_TRUE;
     static HI_U64 u64PTSBase;

     /*******************************************************
      step 2.1 : query how many packs in one-frame stream.
     *******************************************************/
     memset(&stStream, 0, sizeof(stStream));
     s32Ret = HI_MPI_VENC_Query(0, &stStat);
     if (HI_SUCCESS != s32Ret)
     {
          printf("HI_MPI_VENC_Query chn[%d] failed with %#x!\n", 0, s32Ret);
          return;
     }

     /*******************************************************
      step 2.2 : malloc corresponding number of pack nodes.
     *******************************************************/
     stStream.pstPack = (VENC_PACK_S *)malloc(sizeof(VENC_PACK_S) * stStat.u32CurPacks);
     if (NULL == stStream.pstPack)
     {
          printf("malloc stream pack failed!\n");
          return;
     }
                    
     /*******************************************************
      step 2.3 : call mpi to get one-frame stream
     *******************************************************/
     stStream.u32PackCount = stStat.u32CurPacks;
     s32Ret = HI_MPI_VENC_GetStream(0, &stStream, HI_TRUE);
     if (HI_SUCCESS != s32Ret)
     {
          free(stStream.pstPack);
          stStream.pstPack = NULL;
          printf("HI_MPI_VENC_GetStream failed with %#x!\n", s32Ret);
          return;
     }

     /*******************************************************
      step 2.4 : send frame to live stream
     *******************************************************/
     HI_U8 *p = NULL;
     int i;
#if 0
     if (first_time)
     {
         first_time = HI_FALSE;
         u64PTSBase = stStream.pstPack[0].u64PTS;
         fPresentationTime.tv_sec = 0;
         fPresentationTime.tv_usec = 0;
     }
     else
     {
          fPresentationTime.tv_sec = (stStream.pstPack[0].u64PTS - u64PTSBase) / 1000000UL;
          fPresentationTime.tv_usec = (stStream.pstPack[0].u64PTS - u64PTSBase) % 1000000UL;
     }
#endif
     fPresentationTime.tv_sec = stStream.pstPack[0].u64PTS / 1000000UL;
     fPresentationTime.tv_usec = stStream.pstPack[0].u64PTS % 1000000UL;
     //gettimeofday(&fPresentationTime, NULL);

     HI_U64 pos = 0;
     HI_U64 left = 0;
     for (i = 0; i < stStream.u32PackCount; i++)
     {
          left = (fMaxSize - pos);
          p = stStream.pstPack[i].pu8Addr[0];
          newFrameSize += stStream.pstPack[i].u32Len[0];
          if (p[0] == 0x00 && p[1] == 0x00 && p[2] == 0x00 && p[3] == 0x01)
          {
              left = MIN(left, stStream.pstPack[i].u32Len[0] - 4);
              if (pos + left <= fMaxSize)
              {
                  memcpy(fTo + pos, p + 4, left);
                  pos += left;
              }
              newFrameSize -= 4;
          }
          else
          {
              left = MIN(left, stStream.pstPack[i].u32Len[0]);
              if (pos + left <= fMaxSize)
              {
                  memcpy(fTo + pos, p, left);
                  pos += left;
              }
          }
          if (stStream.pstPack[i].u32Len[1] > 0)
          {
              left = (fMaxSize - pos);
              p = stStream.pstPack[i].pu8Addr[1];
              newFrameSize += stStream.pstPack[i].u32Len[1];

              left = MIN(left, stStream.pstPack[i].u32Len[1]);
              if (pos + left <= fMaxSize)
              {
                  memcpy(fTo + pos, p, left);
                  pos += left;
              }
           }
      }
      // Deliver the data here:
      if (newFrameSize > fMaxSize)
      {
          fFrameSize = fMaxSize;
          fNumTruncatedBytes = newFrameSize - fMaxSize;
      } else
      {
          fFrameSize = newFrameSize;
      }
                 
     /*******************************************************
      step 2.5 : release stream
     *******************************************************/
     s32Ret = HI_MPI_VENC_ReleaseStream(0, &stStream);
     if (HI_SUCCESS != s32Ret)
     {
          free (stStream.pstPack);
          stStream.pstPack = NULL;
          return;
     }
     /*******************************************************
      step 2.6 : free pack nodes
     *******************************************************/
     free(stStream.pstPack);
     stStream.pstPack = NULL;

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
  void signalNewFrameData() {
  TaskScheduler* ourScheduler = NULL; //%%% TO BE WRITTEN %%%
  H264LiveStreamSource* ourDevice  = NULL; //%%% TO BE WRITTEN %%%

  if (ourScheduler != NULL) { // sanity check
  ourScheduler->triggerEvent(H264LiveStreamSource::eventTriggerId, ourDevice);
  }
  }
*/
