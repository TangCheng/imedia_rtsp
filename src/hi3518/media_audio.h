#ifndef __MEDIA_AUDIO_H__
#define __MEDIA_AUDIO_H__

#include "stream_descriptor.h"

struct IpcamMediaAudio;
typedef struct IpcamMediaAudio IpcamMediaAudio;

IpcamMediaAudio *ipcam_media_audio_new(void);
void ipcam_media_audio_free(IpcamMediaAudio *audio);

gint32 ipcam_media_audio_start(IpcamMediaAudio *audio, StreamDescriptor *desc);
gint32 ipcam_media_audio_stop(IpcamMediaAudio *audio);
void ipcam_media_audio_param_change(IpcamMediaAudio *audio, StreamDescriptor *desc);

#endif /* __MEDIA_AUDIO_H__ */
