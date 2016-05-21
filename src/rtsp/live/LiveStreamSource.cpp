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
#include <alloca.h>
#include <hi_type.h>
#include <hi_defines.h>
#include <hi_comm_sys.h>
#include <hi_comm_venc.h>
#include <mpi_sys.h>
#include <mpi_venc.h>
#include <mpi_aenc.h>
#include <mpi_sys.h>
#include "LiveStreamSource.hh"
#include "stream_descriptor.h"

LiveVideoStreamSource*
LiveVideoStreamSource::createNew(UsageEnvironment& env, StreamChannel chn)
{
    return new LiveVideoStreamSource(env, chn);
}

static void ClearVideoStreamBuffer(StreamChannel chn)
{
    VENC_CHN_STAT_S stStat;
    VENC_STREAM_S stStream;
    HI_S32 s32Ret;

    while (TRUE) {
        s32Ret = HI_MPI_VENC_Query(chn, &stStat);
        if ((HI_SUCCESS != s32Ret) || (stStat.u32CurPacks <= 0))
            break;
        if (stStat.u32CurPacks <= 0)
            break;

        stStream.pstPack = (VENC_PACK_S *)alloca(sizeof(VENC_PACK_S) * stStat.u32CurPacks);
        stStream.u32PackCount = stStat.u32CurPacks;
        stStream.u32Seq = 0;
        memset(&stStream.stH264Info, 0, sizeof(VENC_STREAM_INFO_H264_S));
        s32Ret = HI_MPI_VENC_GetStream(chn, &stStream, HI_TRUE);
        if (HI_SUCCESS != s32Ret)
            break;;
        s32Ret = HI_MPI_VENC_ReleaseStream(chn, &stStream);
        if (HI_SUCCESS != s32Ret)
            break;
    }
}

LiveVideoStreamSource::LiveVideoStreamSource(UsageEnvironment& env, StreamChannel chn)
    : FramedSource(env), fChannelNo(chn)
{
    TaskScheduler &scheduler = envir().taskScheduler();

    ClearVideoStreamBuffer(fChannelNo);

    HI_MPI_VENC_RequestIDRInst(fChannelNo);

	vencFd = HI_MPI_VENC_GetFd(fChannelNo);

    scheduler.setBackgroundHandling(vencFd, SOCKET_READABLE,
                                    (TaskScheduler::BackgroundHandlerProc*)deliverFrame0,
                                    this);
}

LiveVideoStreamSource::~LiveVideoStreamSource() {
    envir().taskScheduler().disableBackgroundHandling(vencFd);
}

void LiveVideoStreamSource::doGetNextFrame() {
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

void LiveVideoStreamSource::deliverFrame0(void *clientData, int mask)
{
     if (clientData)
          ((LiveVideoStreamSource*)clientData)->deliverFrame();
}

void LiveVideoStreamSource::deliverFrame() {
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

    // we're not ready for the data yet
    if (!isCurrentlyAwaitingData())
        return;

    VENC_CHN_STAT_S stStat;
    VENC_STREAM_S stStream;
    HI_S32 s32Ret;

    s32Ret = HI_MPI_VENC_Query(fChannelNo, &stStat);
    if (HI_SUCCESS != s32Ret)
    {
        return;
    }

    if (stStat.u32CurPacks <= 0) {
        return;
    }

    stStream.pstPack = (VENC_PACK_S *)alloca(sizeof(VENC_PACK_S) * stStat.u32CurPacks);
    stStream.u32PackCount = stStat.u32CurPacks;
    stStream.u32Seq = 0;
    memset(&stStream.stH264Info, 0, sizeof(VENC_STREAM_INFO_H264_S));
    s32Ret = HI_MPI_VENC_GetStream(fChannelNo, &stStream, HI_FALSE);
    if (HI_SUCCESS != s32Ret)
    {
        g_critical("HI_MPI_VENC_GetStream failed with %#x!\n", s32Ret);
        return;
    }

    fPresentationTime.tv_sec = stStream.pstPack[0].u64PTS / 1000000UL;
    fPresentationTime.tv_usec = stStream.pstPack[0].u64PTS % 1000000UL;

    fFrameSize = 0;
    for (int i = 0; i < stStream.u32PackCount; i++) {
        for (int j = 0; j < ARRAY_SIZE(stStream.pstPack[i].pu8Addr); j++) {
            HI_U8 *p = stStream.pstPack[i].pu8Addr[j];
            HI_U32 len = stStream.pstPack[i].u32Len[j];

            if (len == 0)
                continue;

            if (len >= 3 && p[0] == 0x00 && p[1] == 0x00 && p[2] == 0x01) {
                p += 3;
                len -= 3;
            }
            if (len >= 4 && p[0] == 0x00 && p[1] == 0x00 && p[2] == 0x00 && p[3] == 0x01) {
                p += 4;
                len -= 4;
            }

            if (fFrameSize + len > fMaxSize) {
                g_critical("Package Length execute the fMaxSize\n");
                break;
            }

            memmove(&fTo[fFrameSize], p, len);
            fFrameSize += len;
        }
    }

    s32Ret = HI_MPI_VENC_ReleaseStream(fChannelNo, &stStream);
    if (HI_SUCCESS != s32Ret)
    {
        g_critical("HI_MPI_VENC_ReleaseStream failed with %#x!\n", s32Ret);
    }

    // After delivering the data, inform the reader that it is now available:
    FramedSource::afterGetting(this);
}


////////////////////////////////////////////////////////////////////////////////
// Audio
////////////////////////////////////////////////////////////////////////////////

LiveAudioStreamSource*
LiveAudioStreamSource::createNew(UsageEnvironment& env, StreamChannel chn)
{
    return new LiveAudioStreamSource(env, chn);
}

static void ClearAudioStreamBuffer(StreamChannel chn)
{
    AUDIO_STREAM_S stStream;
    HI_S32 s32Ret;

    while (TRUE) {
		s32Ret = HI_MPI_AENC_GetStream(chn, &stStream, HI_FALSE);
		if (s32Ret != HI_SUCCESS)
			break;
		s32Ret = HI_MPI_AENC_ReleaseStream(chn, &stStream);
	}
}

LiveAudioStreamSource
::LiveAudioStreamSource(UsageEnvironment& env, StreamChannel chn)
    : FramedSource(env), fChannelNo(chn)
{
    TaskScheduler &scheduler = envir().taskScheduler();

	aencFd = HI_MPI_AENC_GetFd(fChannelNo);

	ClearAudioStreamBuffer(fChannelNo);

	scheduler.setBackgroundHandling(aencFd, SOCKET_READABLE,
                                    (TaskScheduler::BackgroundHandlerProc*)deliverFrame0,
                                    this);
}

LiveAudioStreamSource::~LiveAudioStreamSource()
{
    envir().taskScheduler().disableBackgroundHandling(aencFd);
}

void LiveAudioStreamSource::doGetNextFrame() {
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

void LiveAudioStreamSource::deliverFrame0(void* clientData, int mask)
{
    if (clientData)
        ((LiveAudioStreamSource*)clientData)->deliverFrame();
}

void LiveAudioStreamSource::deliverFrame()
{
    if (!isCurrentlyAwaitingData()) return; // we're not ready for the data yet

    AUDIO_STREAM_S stStream;
    HI_S32 s32Ret;

    s32Ret = HI_MPI_AENC_GetStream(fChannelNo, &stStream, HI_FALSE);
    if (HI_SUCCESS != s32Ret)
    {
        g_critical("HI_MPI_AENC_GetStream failed with %#x!\n", s32Ret);
        return;
    }

    fPresentationTime.tv_sec = stStream.u64TimeStamp / 1000000UL;
    fPresentationTime.tv_usec = stStream.u64TimeStamp % 1000000UL;

    fFrameSize = 0;
	if (stStream.u32Len <= fMaxSize) {
		fFrameSize = stStream.u32Len - 4;
		fNumTruncatedBytes = 0;
	}
	else {
		fFrameSize = fMaxSize;
		fNumTruncatedBytes = stStream.u32Len - 4 - fMaxSize;
	}
	memmove(fTo, stStream.pStream + 4, fFrameSize);

    s32Ret = HI_MPI_AENC_ReleaseStream(fChannelNo, &stStream);
    if (HI_SUCCESS != s32Ret)
    {
        g_critical("HI_MPI_AENC_ReleaseStream failed with %#x!\n", s32Ret);
    }

    // After delivering the data, inform the reader that it is now available:
    FramedSource::afterGetting(this);
}
