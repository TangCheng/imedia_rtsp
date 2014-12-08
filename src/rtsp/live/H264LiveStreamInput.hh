#ifndef __H264_LIVE_STREAM_INPUT_HH
#define __H264_LIVE_STREAM_INPUT_HH

#include <MediaSink.hh>
#include "H264LiveStreamSource.hh"

class H264LiveStreamInput: public Medium {
public:
  static H264LiveStreamInput* createNew(UsageEnvironment& env, void *videoEngine, StreamChannel chn);

  FramedSource* videoSource();

private:
  H264LiveStreamInput(UsageEnvironment& env, void *videoEngine, StreamChannel chn); // called only by createNew()
  virtual ~H264LiveStreamInput();

private:
  friend class WISVideoOpenFileSource;
  FramedSource* fOurVideoSource;
  void *fVideoEngine;
  H264LiveStreamParameters fDevParams;
};

#endif
