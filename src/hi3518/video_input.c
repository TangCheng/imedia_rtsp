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
    guint32 image_width[STREAM_CHN_LAST];
    guint32 image_height[STREAM_CHN_LAST];
} IpcamVideoInputPrivate;

G_DEFINE_TYPE_WITH_PRIVATE(IpcamVideoInput, ipcam_video_input, G_TYPE_OBJECT)

static GParamSpec *obj_properties[N_PROPERTIES] = {NULL, };

static void ipcam_video_input_init(IpcamVideoInput *self)
{
	IpcamVideoInputPrivate *priv = ipcam_video_input_get_instance_private(self);
    priv->sensor_type = getenv("SENSOR_TYPE");
    priv->image_width[MASTER] = IMAGE_MAX_WIDTH;
    priv->image_height[MASTER] = IMAGE_MAX_HEIGHT;
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
            priv->image_width[MASTER] = g_value_get_uint(value);
        }
        break;
    case PROP_IMAGE_HEIGHT:
        {
            priv->image_height[MASTER] = g_value_get_uint(value);
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
    g_return_val_if_fail(IPCAM_IS_VIDEO_INPUT(self), HI_FAILURE);
    HI_S32 i, s32Ret = HI_SUCCESS;
    VI_DEV ViDev;
    VI_CHN ViChn;
    HI_U32 u32DevNum = 1;
    HI_U32 u32ChnNum = 1;
    IpcamVideoInputPrivate *priv = ipcam_video_input_get_instance_private(self);

    priv->image_width[MASTER] = desc[MASTER].v_desc.image_width;
    priv->image_height[MASTER] = desc[MASTER].v_desc.image_height;
    priv->image_width[SLAVE] = desc[SLAVE].v_desc.image_width;
    priv->image_height[SLAVE] = desc[SLAVE].v_desc.image_height;
    
    /******************************************************
     step 3 : config & start vicap dev
    ******************************************************/
    for (i = 0; i < u32DevNum; i++)
    {
        ViDev = i;
        #if 1
        VI_DEV_ATTR_S stViDevAttr =
            {
                VI_MODE_DIGITAL_CAMERA,
                VI_WORK_MODE_1Multiplex,
                {0xFFF00000, 0x00},
                VI_SCAN_PROGRESSIVE,
                {-1, -1, -1, -1},
                VI_INPUT_DATA_YUYV,
                {
                    VI_VSYNC_FIELD,
                    VI_VSYNC_NEG_HIGH,
                    VI_HSYNC_VALID_SINGNAL,
                    VI_HSYNC_NEG_HIGH,
                    VI_VSYNC_VALID_SINGAL,
                    VI_VSYNC_VALID_NEG_HIGH,
                    {
                        0, priv->image_width[MASTER], 0,
                        0, priv->image_height[MASTER], 0,
                        0, 0, 0
                    }
                },
                VI_PATH_ISP,
                VI_DATA_TYPE_RGB,
                HI_FALSE
            };
        /*
        if (g_str_equal(priv->sensor_type, "AR0130") ||
            g_str_equal(priv->sensor_type, "AR0331"))
        {
            // do nothing
        }
        */
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
        #else
        VI_DEV_ATTR_EX_S stViDevAttrEx =
            {
                VI_MODE_DIGITAL_CAMERA,
                VI_WORK_MODE_1Multiplex,
                VI_COMBINE_COMPOSITE,
                VI_COMP_MODE_SINGLE,
                VI_CLK_EDGE_SINGLE_DOWN,
                {0xFFF00000, 0x00},
                VI_SCAN_PROGRESSIVE,
                {-1, -1, -1, -1},
                VI_INPUT_DATA_YUYV,
                {
                    VI_VSYNC_PULSE,
                    VI_VSYNC_NEG_HIGH,
                    VI_HSYNC_VALID_SINGNAL,
                    VI_HSYNC_NEG_HIGH,
                    VI_VSYNC_VALID_SINGAL,
                    VI_VSYNC_VALID_NEG_HIGH,
                    {
                        0, priv->image_width[MASTER], 0,
                        0, priv->image_height[MASTER], 0,
                        0, 0, 0
                    }
                },
                {
                    BT656_FIXCODE_1,
                    BT656_FIELD_POLAR_STD
                },
                VI_PATH_ISP,
                VI_DATA_TYPE_RGB,
                HI_FALSE
            };
        HI_MPI_VI_DisableDev(ViDev);
        s32Ret = HI_MPI_VI_SetDevAttrEx(ViDev, &stViDevAttrEx);
        #endif
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
    }
    
    /******************************************************
     * Step 4: config & start vicap chn (max 1) 
     ******************************************************/
    for (i = 0; i < u32ChnNum; i++)
    {
        ViChn = i;
        VI_CHN_ATTR_S stChnAttr =
            {
                {0, 0, priv->image_width[MASTER], priv->image_height[MASTER]},
                {priv->image_width[MASTER], priv->image_height[MASTER]},
                VI_CAPSEL_BOTH,
                PIXEL_FORMAT_YUV_SEMIPLANAR_422,
                HI_FALSE,
                HI_FALSE,
                HI_FALSE,
                -1,
                -1
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
    }

    return s32Ret;
}
gint32 ipcam_video_input_stop(IpcamVideoInput *self)
{
    g_return_val_if_fail(IPCAM_IS_VIDEO_INPUT(self), HI_FAILURE);
    VI_DEV ViDev;
    VI_CHN ViChn;
    HI_S32 i;
    HI_S32 s32Ret;
    HI_U32 u32DevNum = 1;
    HI_U32 u32ChnNum = 1;

    /*** Stop VI Chn ***/
    for(i = 0; i < u32ChnNum; i++)
    {
        /* Stop vi phy-chn */
        ViChn = i;
        s32Ret = HI_MPI_VI_DisableChn(ViChn);
        if (HI_SUCCESS != s32Ret)
        {
            g_critical("HI_MPI_VI_DisableChn [%d] failed with %#x\n", ViChn, s32Ret);
            return HI_FAILURE;
        }
    }

    /*** Stop VI Dev ***/
    for(i = 0; i < u32DevNum; i++)
    {
        ViDev = i;
        s32Ret = HI_MPI_VI_DisableDev(ViDev);
        if (HI_SUCCESS != s32Ret)
        {
            g_critical("HI_MPI_VI_DisableDev [%d] failed with %#x\n", ViDev, s32Ret);
            return HI_FAILURE;
        }
    }

    return HI_SUCCESS;
}
void ipcam_video_input_param_change(IpcamVideoInput *self, StreamDescriptor desc[])
{
    g_return_if_fail(IPCAM_IS_VIDEO_INPUT(self));

    VI_CHN_ATTR_S stChnAttr;
    HI_MPI_VI_GetChnAttr(0, &stChnAttr);
    gboolean param_changed = FALSE;

    if (desc[MASTER].v_desc.flip != stChnAttr.bFlip)
    {
        param_changed = TRUE;
        stChnAttr.bFlip = desc[MASTER].v_desc.flip;
    }

    if (desc[MASTER].v_desc.mirror != stChnAttr.bMirror)
    {
        param_changed = TRUE;
        stChnAttr.bMirror = desc[MASTER].v_desc.mirror;
    }

    if (param_changed)
    {
        HI_MPI_VI_SetChnAttr(0, &stChnAttr);
    }
}
