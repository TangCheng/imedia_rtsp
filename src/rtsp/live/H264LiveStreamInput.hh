#ifndef __H264_LIVE_STREAM_INPUT_HH
#define __H264_LIVE_STREAM_INPUT_HH

#include <MediaSink.hh>
#include "H264LiveStreamSource.hh"

class H264LiveStreamInput: public Medium {
public:
  static H264LiveStreamInput* createNew(UsageEnvironment& env, StreamChannel chn, StreamDescriptor *desc);

  FramedSource* videoSource();

  StreamChannel channelNo() const { return fChannelNo; }
  StreamDescriptor* streamDesc() const { return fStreamDesc; }

private:
  H264LiveStreamInput(UsageEnvironment& env, StreamChannel chn, StreamDescriptor *desc); // called only by createNew()
  virtual ~H264LiveStreamInput();

private:
  FramedSource* fOurVideoSource;
  StreamChannel fChannelNo;
  StreamDescriptor *fStreamDesc;
};

#endif
