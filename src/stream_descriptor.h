#ifndef __STREAM_DESCRIPTOR_H__
#define __STREAM_DESCRIPTOR_H__

#include <glib.h>

#if defined(HI3516)
#define IMAGE_WIDTH          1920
#define IMAGE_HEIGHT         1080
#elif defined(HI3518)
#define IMAGE_WIDTH          1280
#define IMAGE_HEIGHT         1720
#else
#error "NO chip selected!"
#endif

#define ARRAY_SIZE(array)    (sizeof(array) / sizeof(array[0]))

enum StreamType
{
     AUDIO_STREAM = 0,
     VIDEO_STREAM = 1,
     INVALIDE_STREAM_TYPE,
};

enum VideoFormat
{
     VIDEO_FORMAT_MPEG4 = 0,
     VIDEO_FORMAT_H264 = 1,
     VIDEO_FORMAT_MJPEG = 2,
     VIDEO_FORMAT_LAST,
};

enum BitRateType
{
     CONSTANT_BIT_RATE = 0,
     VIRIABLE_BIT_RATE = 1,
     BIT_RATE_TYPE_LAST,
};

typedef struct _VideoStreamDescriptor
{
     enum VideoFormat format;
     guint image_width;
     guint image_height;
     guint frame_rate;
     guint iframe_ratio;
     enum BitRateType bit_rate_type;
     guint bit_rate;
} VideoStreamDescriptor;

enum AudioFormat
{
     AUDIO_FORMAT_G_711 = 0,
     AUDIO_FORMAT_G_726 = 1,
     AUDIO_FORMAT_LAST,
};

typedef struct _AudioStreamDescriptor
{
     enum AudioFormat format;
} AudioStreamDescriptor;

typedef struct _StreamDescriptor
{
     enum StreamType type;
     union
     {
          VideoStreamDescriptor v_desc;
          AudioStreamDescriptor a_desc;
     };
} StreamDescriptor;

#endif /* __STREAM_DESCRIPTOR_H__ */
