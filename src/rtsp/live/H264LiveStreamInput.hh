#ifndef __H264_LIVE_STREAM_INPUT_HH
#define __H264_LIVE_STREAM_INPUT_HH

#include <MediaSink.hh>
#include "H264LiveStreamSource.hh"

class H264LiveStreamInput: public Medium {
public:
  static H264LiveStreamInput* createNew(UsageEnvironment& env, void *videoEngine);

  FramedSource* videoSource();

private:
  H264LiveStreamInput(UsageEnvironment& env, void *videoEngine); // called only by createNew()
  virtual ~H264LiveStreamInput();

private:
  friend class WISVideoOpenFileSource;
  static Boolean fHaveInitialized;
  static int fOurVideoFileNo;
  static FramedSource* fOurVideoSource;
  void *fVideoEngine;
  H264LiveStreamParameters fDevParams;
};

#endif
