#include <hi_defines.h>
#include <hi_comm_vi.h>
#include <mpi_vi.h>
#include <stdlib.h>
#include "video_input.h"

enum
{
    PROP_0,
    PROP_IMAGE_WIDTH,
    PROP_IMAGE_HEIGHT,
    N_PROPERTIES
};

typedef struct _IpcamVideoInputPrivate
{
    gchar *sensor_type;
    guint32 image_width;
    guint32 image_height;
} IpcamVideoInputPrivate;

G_DEFINE_TYPE_WITH_PRIVATE(IpcamVideoInput, ipcam_video_input, G_TYPE_OBJECT)

static GParamSpec *obj_properties[N_PROPERTIES] = {NULL, };

static void ipcam_video_input_init(IpcamVideoInput *self)
{
	IpcamVideoInputPrivate *priv = ipcam_video_input_get_instance_private(self);
    priv->sensor_type = getenv("SENSOR_TYPE");
    priv->image_width = IMAGE_MAX_WIDTH;
    priv->image_height = IMAGE_MAX_HEIGHT;
}
static void ipcam_video_input_get_property(GObject    *object,
                                           guint       property_id,
                                           GValue     *value,
                                           GParamSpec *pspec)
{
    IpcamVideoInput *self = IPCAM_VIDEO_INPUT(object);
    IpcamVideoInputPrivate *priv = ipcam_video_input_get_instance_private(self);
    switch(property_id)
    {
    case PROP_IMAGE_WIDTH:
        {
            g_value_set_uint(value, priv->image_width);
        }
        break;
    case PROP_IMAGE_HEIGHT:
        {
            g_value_set_uint(value, priv->image_height);
        }
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}
static void ipcam_video_input_set_property(GObject      *object,
                                           guint         property_id,
                                           const GValue *value,
                                           GParamSpec   *pspec)
{
    IpcamVideoInput *self = IPCAM_VIDEO_INPUT(object);
    IpcamVideoInputPrivate *priv = ipcam_video_input_get_instance_private(self);
    switch(property_id)
    {
    case PROP_IMAGE_WIDTH:
        {
            priv->image_width = g_value_get_uint(value);
        }
        break;
    case PROP_IMAGE_HEIGHT:
        {
            priv->image_height = g_value_get_uint(value);
        }
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}
static void ipcam_video_input_class_init(IpcamVideoInputClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = &ipcam_video_input_get_property;
    object_class->set_property = &ipcam_video_input_set_property;

    obj_properties[PROP_IMAGE_WIDTH] =
        g_param_spec_uint("width",
                          "Image width",
                          "set video input unit image width.",
                          352, // min value
                          IMAGE_MAX_WIDTH, // max value
                          IMAGE_MAX_WIDTH, // default value
                          G_PARAM_READWRITE);
    obj_properties[PROP_IMAGE_HEIGHT] =
        g_param_spec_uint("height",
                          "Image height",
                          "set video input unit image height.",
                          288, // min value
                          IMAGE_MAX_HEIGHT, // max value
                          IMAGE_MAX_HEIGHT, // default value
                          G_PARAM_READWRITE);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

gint32 ipcam_video_input_start(IpcamVideoInput *self, StreamDescriptor desc[])
{
    HI_S32 s32Ret = HI_SUCCESS;
    VI_DEV ViDev;
    VI_CHN ViChn;
    VI_CHN ViExtChn;
    guint32 sensor_image_width, sensor_image_height;
    gchar *resolution;
    guint32 input_fps;
    IpcamVideoInputPrivate *priv = ipcam_video_input_get_instance_private(self);

    g_return_val_if_fail(IPCAM_IS_VIDEO_INPUT(self), HI_FAILURE);

    /* default sensor mode: 1920x1200@20Hz */
    sensor_image_width = 1920;
    sensor_image_height = 1200;
    input_fps = 20;

    resolution = desc[MASTER_CHN].v_desc.resolution;
    if (g_str_equal(resolution, "UXGA") ||
        g_str_equal(resolution, "960H"))
    {
#if defined(SENSOR_MODE_AUTO)
        sensor_image_width = 1920;
        sensor_image_height = 1200;
        input_fps = 20;
#endif
        priv->image_width = 1600;
        priv->image_height = 1200;
    }
    else
    {
#if defined(SENSOR_MODE_AUTO)
        sensor_image_width = 1920;
        sensor_image_height = 1080;
        input_fps = 30;
#endif
        priv->image_width = 1920;
        priv->image_height = 1080;
    }

    /******************************************************
     step 3 : config & start vicap dev
     ******************************************************/
    ViDev = 0;
    VI_DEV_ATTR_S stViDevAttr =
    {
        .enIntfMode = VI_MODE_DIGITAL_CAMERA,
        .enWorkMode = VI_WORK_MODE_1Multiplex,
        .au32CompMask = { 0xFFF00000, 0x00 },
        .enScanMode = VI_SCAN_PROGRESSIVE,
        .s32AdChnId = { -1, -1, -1, -1 },
        .enDataSeq = VI_INPUT_DATA_YUYV,
        .stSynCfg = {
            .enVsync = VI_VSYNC_FIELD,
            .enVsyncNeg = VI_VSYNC_NEG_HIGH,
            .enHsync = VI_HSYNC_VALID_SINGNAL,
            .enHsyncNeg = VI_HSYNC_NEG_HIGH,
            .enVsyncValid = VI_VSYNC_VALID_SINGAL,
            .enVsyncValidNeg = VI_VSYNC_VALID_NEG_HIGH,
            .stTimingBlank = {
                .u32HsyncHfb = 0,
                .u32HsyncAct = sensor_image_width,
                .u32HsyncHbb = 0,
                .u32VsyncVfb = 0,
                .u32VsyncVact = sensor_image_height,
                .u32VsyncVbb = 0,
                .u32VsyncVbfb = 0,
                .u32VsyncVbact = 0,
                .u32VsyncVbbb = 0
            }
        },
        .enDataPath = VI_PATH_ISP,
        .enInputDataType = VI_DATA_TYPE_RGB,
        .bDataRev = HI_FALSE
    };
    if (g_str_equal(priv->sensor_type, "IMX222"))
    {
        stViDevAttr.stSynCfg.enVsync = VI_VSYNC_PULSE;
        //stViDevAttr.stSynCfg.enVsyncValid = VI_VSYNC_NORM_PULSE;
    }
    else if (g_str_equal(priv->sensor_type, "NT99141"))
    {
        stViDevAttr.au32CompMask[0] = 0xFF000000;
        stViDevAttr.stSynCfg.stTimingBlank.u32HsyncHfb = 4;
        stViDevAttr.stSynCfg.stTimingBlank.u32HsyncHbb = 544;
        stViDevAttr.stSynCfg.stTimingBlank.u32VsyncVfb = 4;
        stViDevAttr.stSynCfg.stTimingBlank.u32VsyncVbb = 20;
        stViDevAttr.enDataPath = VI_PATH_BYPASS;
        stViDevAttr.enInputDataType = VI_DATA_TYPE_YUV;
    }

    s32Ret = HI_MPI_VI_SetDevAttr(ViDev, &stViDevAttr);
    if (s32Ret != HI_SUCCESS)
    {
        g_critical("HI_MPI_VI_SetDevAttr [%d] failed with %#x!\n", ViDev, s32Ret);
        return HI_FAILURE;
    }

    s32Ret = HI_MPI_VI_EnableDev(ViDev);
    if (s32Ret != HI_SUCCESS)
    {
        g_critical("HI_MPI_VI_EnableDev [%d] failed with %#x!\n", ViDev, s32Ret);
        return HI_FAILURE;
    }

    /******************************************************
     * Step 4: config & start vicap chn (max 1) 
     ******************************************************/
    ViChn = 0;
    VI_CHN_ATTR_S stChnAttr =
    {
        .stCapRect = {
            .s32X = (sensor_image_width - priv->image_width) / 2,
            .s32Y = (sensor_image_height - priv->image_height) / 2,
            .u32Width = priv->image_width,
            .u32Height = priv->image_height
        },
        .stDestSize = {
            priv->image_width,
            priv->image_height
        },
        .enCapSel = VI_CAPSEL_BOTH,
        .enPixFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_422,
        .bMirror = desc[MASTER_CHN].v_desc.mirror,
        .bFlip = desc[MASTER_CHN].v_desc.flip,
        .bChromaResample = HI_FALSE,
        .s32SrcFrameRate = input_fps,
        .s32FrameRate = input_fps
    };

    stChnAttr.bMirror = desc[MASTER].v_desc.mirror;
    stChnAttr.bFlip = desc[MASTER].v_desc.flip;

    s32Ret = HI_MPI_VI_SetChnAttr(ViChn, &stChnAttr);
    if (s32Ret != HI_SUCCESS)
    {
        g_critical("HI_MPI_VI_SetChnAttr [%d] failed with %#x!\n", ViChn, s32Ret);
        return HI_FAILURE;
    }

    s32Ret = HI_MPI_VI_EnableChn(ViChn);
    if (s32Ret != HI_SUCCESS)
    {
        g_critical("HI_MPI_VI_Enable [%d] failed with %#x!\n", ViChn, s32Ret);
        return HI_FAILURE;
    }

    VI_EXT_CHN_ATTR_S stViExtChnAttr;
    stViExtChnAttr.s32BindChn = 0;
    stViExtChnAttr.stDestSize.u32Width = 320;
    stViExtChnAttr.stDestSize.u32Height = 240;
    stViExtChnAttr.s32SrcFrameRate = input_fps;
    stViExtChnAttr.s32FrameRate = input_fps;
    stViExtChnAttr.enPixFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_422;

    ViExtChn = 1;
    s32Ret = HI_MPI_VI_SetExtChnAttr(ViExtChn, &stViExtChnAttr);
    if (s32Ret != HI_SUCCESS) {
        g_critical("HI_MPI_VI_SetExtChnAttr [%d] failed with %#x!\n", ViChn, s32Ret);
        return HI_FAILURE;
    }

    s32Ret = HI_MPI_VI_EnableChn(ViExtChn);
    if (s32Ret != HI_SUCCESS)
    {
        g_critical("HI_MPI_VI_Enable [%d] failed with %#x!\n", ViExtChn, s32Ret);
        return HI_FAILURE;
    }

    return s32Ret;
}

gint32 ipcam_video_input_stop(IpcamVideoInput *self)
{
    g_return_val_if_fail(IPCAM_IS_VIDEO_INPUT(self), HI_FAILURE);
    VI_DEV ViDev;
    VI_CHN ViChn;
    VI_CHN ViExtChn;
    HI_S32 s32Ret;

    /* Stop vi ext-chn */
    ViExtChn = 1;
    s32Ret = HI_MPI_VI_DisableChn(ViExtChn);
    if (HI_SUCCESS != s32Ret)
    {
        g_critical("HI_MPI_VI_DisableChn [%d] failed with %#x\n", ViExtChn, s32Ret);
    }

    /* Stop vi phy-chn */
    ViChn = 0;
    s32Ret = HI_MPI_VI_DisableChn(ViChn);
    if (HI_SUCCESS != s32Ret)
    {
        g_critical("HI_MPI_VI_DisableChn [%d] failed with %#x\n", ViChn, s32Ret);
    }

    /*** Stop VI Dev ***/
    ViDev = 0;
    s32Ret = HI_MPI_VI_DisableDev(ViDev);
    if (HI_SUCCESS != s32Ret)
    {
        g_critical("HI_MPI_VI_DisableDev [%d] failed with %#x\n", ViDev, s32Ret);
    }

    return HI_SUCCESS;
}

void ipcam_video_input_param_change(IpcamVideoInput *self, StreamDescriptor desc[])
{
    g_return_if_fail(IPCAM_IS_VIDEO_INPUT(self));

    ipcam_video_input_stop(self);
    ipcam_video_input_start(self, desc);
}

void ipcam_video_input_set_image_parameter(IpcamVideoInput *self, IpcamMediaImageAttr *attr)
{
    VI_DEV ViDev = 0;
    VI_CSC_ATTR_S csc_attr;
    HI_S32 s32Ret;

    g_return_if_fail(IPCAM_IS_VIDEO_INPUT(self));

    s32Ret = HI_MPI_VI_GetCSCAttr(ViDev, &csc_attr);
    if (HI_SUCCESS != s32Ret) {
        g_critical("HI_MPI_VI_GetCSCAttr [%d] failed with %#x\n", ViDev, s32Ret);
        return;
    }

#define MIN(a, b)   ((a) < (b) ? (a) : (b))
    if (attr->brightness >= 0)
        csc_attr.u32LumaVal = MIN(attr->brightness, 100);
    if (attr->chrominance >= 0)
        csc_attr.u32HueVal = MIN(attr->chrominance, 100);
    if (attr->contrast >= 0)
        csc_attr.u32ContrVal = MIN(attr->contrast, 100);
    if (attr->saturation >= 0)
        csc_attr.u32SatuVal = MIN(attr->saturation, 100);
    s32Ret = HI_MPI_VI_SetCSCAttr(ViDev, &csc_attr);
    if (HI_SUCCESS != s32Ret) {
        g_critical("HI_MPI_VI_SetCSCAttr [%d] failed with %#x\n", ViDev, s32Ret);
    }
}