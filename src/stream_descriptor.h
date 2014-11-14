#ifndef __STREAM_DESCRIPTOR_H__
#define __STREAM_DESCRIPTOR_H__

#include <glib.h>
#include <time.h>

#if defined(HI3516)
#define IMAGE_MAX_WIDTH          1920
#define IMAGE_MAX_HEIGHT         1200
#elif defined(HI3518)
#define IMAGE_MAX_WIDTH          1280
#define IMAGE_MAX_HEIGHT         960
#else
#error "NO chip selected!"
#endif

#define ARRAY_SIZE(array)    (sizeof(array) / sizeof(array[0]))

enum StreamChannel
{
    MASTER_CHN = 0,
    SLAVE_CHN  = 1,
    STREAM_CHN_LAST
};

#define MASTER MASTER_CHN
#define SLAVE  SLAVE_CHN

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

enum H264Profile
{
    BASELINE_PROFILE = 0,
    MAIN_PROFILE = 1,
    HIGH_PROFILE = 2,
    H264_PROFILE_LAST,
};

enum BitRateType
{
    CONSTANT_BIT_RATE = 0,
    VARIABLE_BIT_RATE = 1,
    BIT_RATE_TYPE_LAST,
};

typedef struct _VideoStreamDescriptor
{
    enum VideoFormat format;
    enum H264Profile profile;
    const gchar *resolution;
    guint image_width;
    guint image_height;
    guint frame_rate;
    guint iframe_ratio;
    enum BitRateType bit_rate_type;
    guint bit_rate;
    gboolean flip;
    gboolean mirror;
    const gchar *path;
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

typedef struct _StreamData
{
    unsigned int magic;
    struct timeval pts;
    unsigned int len;
    gboolean isIFrame;
    char data[0];
} StreamData;

#endif /* __STREAM_DESCRIPTOR_H__ */
