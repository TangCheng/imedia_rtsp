#include <hi_defines.h>
#include <hi_comm_venc.h>
#include <mpi_venc.h>
#include <memory.h>
#include "video_encode.h"

enum
{
    PROP_0,
    PROP_IMAGE_WIDTH,
    PROP_IMAGE_HEIGHT,
    N_PROPERTIES
};

typedef struct _IpcamVideoEncodePrivate
{
    guint32 image_width;
    guint32 image_height;
} IpcamVideoEncodePrivate;

G_DEFINE_TYPE_WITH_PRIVATE(IpcamVideoEncode, ipcam_video_encode, G_TYPE_OBJECT)

static GParamSpec *obj_properties[N_PROPERTIES] = {NULL, };

static void ipcam_video_encode_init(IpcamVideoEncode *self)
{
    IpcamVideoEncodePrivate *priv = ipcam_video_encode_get_instance_private(self);
    priv->image_width = IMAGE_MAX_WIDTH;
    priv->image_height = IMAGE_MAX_HEIGHT;
}
static void ipcam_video_encode_get_property(GObject    *object,
                                            guint       property_id,
                                            GValue     *value,
                                            GParamSpec *pspec)
{
    IpcamVideoEncode *self = IPCAM_VIDEO_ENCODE(object);
    IpcamVideoEncodePrivate *priv = ipcam_video_encode_get_instance_private(self);
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
static void ipcam_video_encode_set_property(GObject      *object,
                                            guint         property_id,
                                            const GValue *value,
                                            GParamSpec   *pspec)
{
    IpcamVideoEncode *self = IPCAM_VIDEO_ENCODE(object);
    IpcamVideoEncodePrivate *priv = ipcam_video_encode_get_instance_private(self);
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
static void ipcam_video_encode_class_init(IpcamVideoEncodeClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = &ipcam_video_encode_get_property;
    object_class->set_property = &ipcam_video_encode_set_property;

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

gint32 ipcam_video_encode_start(IpcamVideoEncode *self, StreamDescriptor desc[])
{
    g_return_val_if_fail(IPCAM_IS_VIDEO_ENCODE(self), HI_FAILURE);
    HI_S32 s32Ret;
    VENC_GRP VeGroup = 0;
    VENC_CHN VeChn = 0;
    VENC_CHN_ATTR_S stAttr;
    HI_S32 chn;
    IpcamVideoEncodePrivate *priv = ipcam_video_encode_get_instance_private(self);
    guint image_width, image_height;
    guint input_width, input_height;
    guint input_fps;
    guint output_fps;

    gchar *resolution = desc[MASTER_CHN].v_desc.resolution;
    if (g_str_equal(resolution, "UXGA") ||
        g_str_equal(resolution, "960H"))
    {
        input_width = 1600;
        input_height = 1200;
        input_fps = 20;
    }
    else
    {
        input_width = 1920;
        input_height = 1080;
        input_fps = 30;
    }

    for (chn = MASTER_CHN; chn < STREAM_CHN_LAST; chn++)
    {
        /* set h264 chnnel video encode attribute */
        memset(&stAttr, 0, sizeof(VENC_CHN_ATTR_S));
        if (desc[chn].v_desc.format == VIDEO_FORMAT_H264)
        {
            stAttr.stVeAttr.enType = PT_H264;
        }
        else
        {
            g_warn_if_reached();
        }
        image_width = desc[chn].v_desc.image_width;
        image_height = desc[chn].v_desc.image_height;
        stAttr.stVeAttr.stAttrH264e.u32PicWidth = image_width;
        stAttr.stVeAttr.stAttrH264e.u32PicHeight = image_height;
        stAttr.stVeAttr.stAttrH264e.u32MaxPicWidth = IMAGE_MAX_WIDTH;
        stAttr.stVeAttr.stAttrH264e.u32MaxPicHeight = IMAGE_MAX_HEIGHT;
        stAttr.stVeAttr.stAttrH264e.u32Profile = desc[chn].v_desc.profile;
        stAttr.stVeAttr.stAttrH264e.u32BufSize  = IMAGE_MAX_WIDTH * IMAGE_MAX_HEIGHT * 2;/*stream buffer size*/
        stAttr.stVeAttr.stAttrH264e.bByFrame = HI_FALSE;/*get stream mode is slice mode or frame mode?*/
        stAttr.stVeAttr.stAttrH264e.bField = HI_FALSE;  /* surpport frame code only for hi3516, bfield = HI_FALSE */
        stAttr.stVeAttr.stAttrH264e.bMainStream = HI_TRUE; /* surpport main stream only for hi3516, bMainStream = HI_TRUE */
        stAttr.stVeAttr.stAttrH264e.u32Priority = 0; /*channels precedence level. invalidate for hi3516*/
        stAttr.stVeAttr.stAttrH264e.bVIField = HI_FALSE;/*the sign of the VI picture is field or frame. Invalidate for hi3516*/
        // omit other video encode assignments here.
        /* set h264 chnnel rate control attribute */

        output_fps = desc[chn].v_desc.frame_rate > input_fps ? input_fps : desc[chn].v_desc.frame_rate;

        if (desc[chn].v_desc.bit_rate_type == CONSTANT_BIT_RATE)
        {
            stAttr.stRcAttr.enRcMode = VENC_RC_MODE_H264CBR;
            stAttr.stRcAttr.stAttrH264Cbr.u32BitRate = desc[chn].v_desc.bit_rate;
            stAttr.stRcAttr.stAttrH264Cbr.u32ViFrmRate = input_fps;
            stAttr.stRcAttr.stAttrH264Cbr.fr32TargetFrmRate = output_fps;
            stAttr.stRcAttr.stAttrH264Cbr.u32Gop = output_fps;
            stAttr.stRcAttr.stAttrH264Cbr.u32FluctuateLevel = 0;
            stAttr.stRcAttr.stAttrH264Cbr.u32StatTime = 1;
        }
        else if (desc[chn].v_desc.bit_rate_type = VARIABLE_BIT_RATE)
        {
            stAttr.stRcAttr.enRcMode = VENC_RC_MODE_H264VBR;
            stAttr.stRcAttr.stAttrH264Vbr.u32MaxBitRate = desc[chn].v_desc.bit_rate;
            stAttr.stRcAttr.stAttrH264Vbr.u32ViFrmRate = input_fps;
            stAttr.stRcAttr.stAttrH264Vbr.fr32TargetFrmRate = output_fps;
            stAttr.stRcAttr.stAttrH264Vbr.u32Gop = output_fps;
            stAttr.stRcAttr.stAttrH264Vbr.u32MinQp = 0;
            stAttr.stRcAttr.stAttrH264Vbr.u32MaxQp = 51;
            stAttr.stRcAttr.stAttrH264Vbr.u32StatTime = 1;
        }
        else
        {
            g_warn_if_reached();
        }
        // omit other rate control assignments here.
        VeGroup = chn;
        VeChn = chn;
        s32Ret = HI_MPI_VENC_CreateGroup(VeGroup);
        if (HI_SUCCESS != s32Ret)
        {
            g_critical("HI_MPI_VENC_CreateGroup(%d) err 0x%x\n", VeGroup,s32Ret);
            return HI_FAILURE;
        }
        s32Ret = HI_MPI_VENC_CreateChn(VeChn, &stAttr);
        if (HI_SUCCESS != s32Ret)
        {
            g_critical("HI_MPI_VENC_CreateChn(%d) err 0x%x\n", VeChn, s32Ret);
            return HI_FAILURE;
        }
        s32Ret = HI_MPI_VENC_RegisterChn(VeGroup, VeChn);
        if (HI_SUCCESS != s32Ret)
        {
            g_critical("HI_MPI_VENC_RegisterChn(%d,%d) err 0x%x\n", VeGroup, VeChn, s32Ret);
            return HI_FAILURE;
        }
        if (VeGroup != MASTER_CHN)
        {
            guint cropped_width, cropped_height;

            /* calculate the cropped image size */
            /* input_width / input_height > image_width / image_height */
            if (input_width * image_height > input_height * image_width) {
                cropped_height = input_height;
                cropped_width = cropped_height * image_width / image_height;
            }
            else {
                cropped_width = input_width;
                cropped_height = cropped_width * image_height / image_width;
            }

            GROUP_CROP_CFG_S stGrpCropCfg = {
                .bEnable = TRUE,
                .stRect = {
                    .s32X = ((input_width - cropped_width) / 2) & (0xFFFFFFF0),
                    .s32Y = (input_height - cropped_height) / 2,
                    .u32Width = cropped_width & 0xFFFFFFF0,
                    .u32Height = cropped_height & 0xFFFFFFF0
                }
            };

            s32Ret = HI_MPI_VENC_SetGrpCrop(VeGroup, &stGrpCropCfg);
            if (HI_SUCCESS != s32Ret) {
                g_critical("HI_MPI_VENC_SetGrpCrop(%d) err 0x%x\n", VeGroup, s32Ret);
            }
        }

        s32Ret = HI_MPI_VENC_StartRecvPic(VeChn);
        if (s32Ret != HI_SUCCESS)
        {
            g_critical("HI_MPI_VENC_StartRecvPic(%d) err 0x%x\n", VeChn, s32Ret);
            return HI_FAILURE;
        }
    }

    // omit other code here.
    return HI_SUCCESS;
}
gint32 ipcam_video_encode_stop(IpcamVideoEncode *self)
{
    g_return_val_if_fail(IPCAM_IS_VIDEO_ENCODE(self), HI_FAILURE);
    HI_S32 s32Ret;
    HI_S32 chn;
    VENC_GRP VeGrp = 0;
    VENC_CHN VeChn = 0;

    for (chn = MASTER_CHN; chn < STREAM_CHN_LAST; chn++)
    {
        VeGrp = chn;
        VeChn = chn;
        /******************************************
         step 1:  Stop Recv Pictures
         ******************************************/
        s32Ret = HI_MPI_VENC_StopRecvPic(VeChn);
        if (HI_SUCCESS != s32Ret)
        {
            g_critical("HI_MPI_VENC_StopRecvPic(%d) failed with %#x!\n", \
                       VeChn, s32Ret);
        }

        /******************************************
         step 2:  Disable Venc Group Crop
         ******************************************/
        if (VeGrp != MASTER_CHN)
        {
            GROUP_CROP_CFG_S stGrpCropCfg = {
                .bEnable = FALSE,
                .stRect = {
                    .s32X = 0, .s32Y = 0,
                    .u32Width = 0,
                    .u32Height = 0
                }
            };
            s32Ret = HI_MPI_VENC_SetGrpCrop(VeGrp, &stGrpCropCfg);
            if (HI_SUCCESS != s32Ret) {
                g_critical("HI_MPI_VENC_SetGrpCrop(%d) err 0x%x\n", VeGrp, s32Ret);
            }
        }
        /******************************************
         step 2:  UnRegist Venc Channel
         ******************************************/
        s32Ret = HI_MPI_VENC_UnRegisterChn(VeChn);
        if (HI_SUCCESS != s32Ret)
        {
            g_critical("HI_MPI_VENC_UnRegisterChn(%d) failed with %#x!\n", \
                       VeChn, s32Ret);
        }

        /******************************************
         step 3:  Distroy Venc Channel
         ******************************************/
        s32Ret = HI_MPI_VENC_DestroyChn(VeChn);
        if (HI_SUCCESS != s32Ret)
        {
            g_critical("HI_MPI_VENC_DestroyChn(%d) failed with %#x!\n", \
                       VeChn, s32Ret);
        }

        /******************************************
         step 4:  Distroy Venc Group
         ******************************************/
        s32Ret = HI_MPI_VENC_DestroyGroup(VeGrp);
        if (HI_SUCCESS != s32Ret)
        {
            g_critical("HI_MPI_VENC_DestroyGroup(%d) failed with %#x!\n", \
                       VeGrp, s32Ret);
        }
    }

    return HI_SUCCESS;
}
void ipcam_video_encode_param_change(IpcamVideoEncode *self, StreamDescriptor desc[])
{
    g_return_val_if_fail(IPCAM_IS_VIDEO_ENCODE(self), HI_FAILURE);

    ipcam_video_encode_stop(self);
    ipcam_video_encode_start(self, desc);
}
