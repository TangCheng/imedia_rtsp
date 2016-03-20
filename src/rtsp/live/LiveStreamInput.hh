#ifndef __LIVE_STREAM_INPUT_HH
#define __LIVE_STREAM_INPUT_HH

#include <MediaSink.hh>
#include "LiveStreamSource.hh"

class LiveStreamInput: public Medium {
public:
  static LiveStreamInput* createNew(UsageEnvironment& env, StreamChannel chn, StreamDescriptor *desc);

  FramedSource* audioSource();
  FramedSource* videoSource();

  StreamChannel channelNo() const { return fChannelNo; }
  StreamDescriptor* streamDesc() const { return fStreamDesc; }

private:
  LiveStreamInput(UsageEnvironment& env, StreamChannel chn, StreamDescriptor *desc); // called only by createNew()
  virtual ~LiveStreamInput();

private:
  FramedSource* fOurVideoSource;
  FramedSource* fOurAudioSource;
  StreamChannel fChannelNo;
  StreamDescriptor *fStreamDesc;
};

#endif
