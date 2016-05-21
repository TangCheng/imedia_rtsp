#include <hi_defines.h>
#include <hi_comm_vpss.h>
#include <mpi_vpss.h>
#include "video_process_subsystem.h"

enum
{
    PROP_0,
    PROP_IMAGE_WIDTH,
    PROP_IMAGE_HEIGHT,
    N_PROPERTIES
};

typedef struct _IpcamVideoProcessSubsystemPrivate
{
    guint32 image_width;
    guint32 image_height;
} IpcamVideoProcessSubsystemPrivate;

G_DEFINE_TYPE_WITH_PRIVATE(IpcamVideoProcessSubsystem, ipcam_video_process_subsystem, G_TYPE_OBJECT)

static GParamSpec *obj_properties[N_PROPERTIES] = {NULL, };

static void ipcam_video_process_subsystem_init(IpcamVideoProcessSubsystem *self)
{
	IpcamVideoProcessSubsystemPrivate *priv = ipcam_video_process_subsystem_get_instance_private(self);
    priv->image_width = IMAGE_MAX_WIDTH;
    priv->image_height = IMAGE_MAX_HEIGHT;
}
static void ipcam_video_process_subsystem_get_property(GObject    *object,
                                                       guint       property_id,
                                                       GValue     *value,
                                                       GParamSpec *pspec)
{
    IpcamVideoProcessSubsystem *self = IPCAM_VIDEO_PROCESS_SUBSYSTEM(object);
    IpcamVideoProcessSubsystemPrivate *priv = ipcam_video_process_subsystem_get_instance_private(self);
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
static void ipcam_video_process_subsystem_set_property(GObject      *object,
                                                       guint         property_id,
                                                       const GValue *value,
                                                       GParamSpec   *pspec)
{
    IpcamVideoProcessSubsystem *self = IPCAM_VIDEO_PROCESS_SUBSYSTEM(object);
    IpcamVideoProcessSubsystemPrivate *priv = ipcam_video_process_subsystem_get_instance_private(self);
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
static void ipcam_video_process_subsystem_class_init(IpcamVideoProcessSubsystemClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = &ipcam_video_process_subsystem_get_property;
    object_class->set_property = &ipcam_video_process_subsystem_set_property;

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

gint32 ipcam_video_process_subsystem_start(IpcamVideoProcessSubsystem *self, StreamDescriptor desc[])
{
    g_return_val_if_fail(IPCAM_IS_VIDEO_PROCESS_SUBSYSTEM(self), HI_FAILURE);
    VPSS_GRP VpssGrp = 0;
    VPSS_CHN VpssChn = 0;
    VPSS_GRP_ATTR_S stGrpAttr;
    VPSS_CHN_ATTR_S stChnAttr;
    VPSS_GRP_PARAM_S stVpssParam;
    HI_S32 s32Ret;
    IpcamVideoProcessSubsystemPrivate *priv = ipcam_video_process_subsystem_get_instance_private(self);

    gchar *resolution = (gchar *)desc[MASTER_CHN].v_desc.resolution;
    if (g_str_equal(resolution, "UXGA") ||
        g_str_equal(resolution, "960H"))
    {
        priv->image_width = 1600;
        priv->image_height = 1200;
    }
    else
    {
        priv->image_width = 1920;
        priv->image_height = 1080;
    }

    /*** Set Vpss Grp Attr ***/

    stGrpAttr.u32MaxW = priv->image_width;
    stGrpAttr.u32MaxH = priv->image_height;
    stGrpAttr.bDrEn = HI_FALSE;
    stGrpAttr.bDbEn = HI_FALSE;
    stGrpAttr.bIeEn = HI_FALSE;
    stGrpAttr.bNrEn = HI_TRUE;
    if (desc[MASTER].v_desc.img_attr.b3ddnr >= 0)
        stGrpAttr.bNrEn = desc[MASTER].v_desc.img_attr.b3ddnr;
    stGrpAttr.bHistEn = HI_FALSE;
    stGrpAttr.enDieMode = VPSS_DIE_MODE_AUTO;
    stGrpAttr.enPixFmt = PIXEL_FORMAT_YUV_SEMIPLANAR_422;

    /*** create vpss group ***/
    s32Ret = HI_MPI_VPSS_CreateGrp(VpssGrp, &stGrpAttr);
    if (s32Ret != HI_SUCCESS)
    {
        g_critical("HI_MPI_VPSS_CreateGrp failed with %#x!\n", s32Ret);
        return HI_FAILURE;
    }

    /*** set vpss param ***/
    s32Ret = HI_MPI_VPSS_GetGrpParam(VpssGrp, &stVpssParam);
    if (s32Ret != HI_SUCCESS)
    {
        g_critical("HI_MPI_VPSS_GetGrpParam failed with %#x!\n", s32Ret);
        return HI_FAILURE;
    }
        
    stVpssParam.u32MotionThresh = 0;
        
    s32Ret = HI_MPI_VPSS_SetGrpParam(VpssGrp, &stVpssParam);
    if (s32Ret != HI_SUCCESS)
    {
        g_critical("HI_MPI_VPSS_SetGrpParam failed with %#x!\n", s32Ret);
        return HI_FAILURE;
    }

    /*** enable vpss chn, without frame ***/
    /* Set Vpss Chn attr */
    stChnAttr.bSpEn = HI_TRUE;
    stChnAttr.bFrameEn = HI_FALSE;
    stChnAttr.stFrame.u32Color[VPSS_FRAME_WORK_LEFT] = 0;
    stChnAttr.stFrame.u32Color[VPSS_FRAME_WORK_RIGHT] = 0;
    stChnAttr.stFrame.u32Color[VPSS_FRAME_WORK_BOTTOM] = 0;
    stChnAttr.stFrame.u32Color[VPSS_FRAME_WORK_TOP] = 0;
    stChnAttr.stFrame.u32Width[VPSS_FRAME_WORK_LEFT] = 0;
    stChnAttr.stFrame.u32Width[VPSS_FRAME_WORK_RIGHT] = 0;
    stChnAttr.stFrame.u32Width[VPSS_FRAME_WORK_TOP] = 0;
    stChnAttr.stFrame.u32Width[VPSS_FRAME_WORK_BOTTOM] = 0;
            
    s32Ret = HI_MPI_VPSS_SetChnAttr(VpssGrp, VpssChn, &stChnAttr);
    if (s32Ret != HI_SUCCESS)
    {
        g_critical("HI_MPI_VPSS_SetChnAttr failed with %#x\n", s32Ret);
        return HI_FAILURE;
    }

	VPSS_CHN_MODE_S chn_mode;
	chn_mode.enChnMode = VPSS_CHN_MODE_USER;
	chn_mode.u32Width = priv->image_width;
	chn_mode.u32Height = priv->image_height;
	chn_mode.bDouble = HI_FALSE;
	chn_mode.enPixelFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
	if ((s32Ret = HI_MPI_VPSS_SetChnMode(VpssGrp, VpssChn, &chn_mode)) != HI_SUCCESS) {
		g_critical("HI_MPI_VPSS_SetChnMode %d-%d failed %#x\n",
		           VpssGrp, VpssChn, s32Ret);
		return HI_FAILURE;
	}
    
    s32Ret = HI_MPI_VPSS_EnableChn(VpssGrp, VpssChn);
    if (s32Ret != HI_SUCCESS)
    {
        g_critical("HI_MPI_VPSS_EnableChn failed with %#x\n", s32Ret);
        return HI_FAILURE;
    }

#if 0
	VpssChn = 1;
    /*** enable vpss chn, without frame ***/
    /* Set Vpss Chn attr */
    stChnAttr.bSpEn = HI_FALSE;
    stChnAttr.bFrameEn = HI_FALSE;
    stChnAttr.stFrame.u32Color[VPSS_FRAME_WORK_LEFT] = 0;
    stChnAttr.stFrame.u32Color[VPSS_FRAME_WORK_RIGHT] = 0;
    stChnAttr.stFrame.u32Color[VPSS_FRAME_WORK_BOTTOM] = 0;
    stChnAttr.stFrame.u32Color[VPSS_FRAME_WORK_TOP] = 0;
    stChnAttr.stFrame.u32Width[VPSS_FRAME_WORK_LEFT] = 0;
    stChnAttr.stFrame.u32Width[VPSS_FRAME_WORK_RIGHT] = 0;
    stChnAttr.stFrame.u32Width[VPSS_FRAME_WORK_TOP] = 0;
    stChnAttr.stFrame.u32Width[VPSS_FRAME_WORK_BOTTOM] = 0;
            
    s32Ret = HI_MPI_VPSS_SetChnAttr(VpssGrp, VpssChn, &stChnAttr);
    if (s32Ret != HI_SUCCESS)
    {
        g_critical("HI_MPI_VPSS_SetChnAttr failed with %#x\n", s32Ret);
        return HI_FAILURE;
    }
    
    s32Ret = HI_MPI_VPSS_EnableChn(VpssGrp, VpssChn);
    if (s32Ret != HI_SUCCESS)
    {
        g_critical("HI_MPI_VPSS_EnableChn failed with %#x\n", s32Ret);
        return HI_FAILURE;
    }
#endif

    /*** start vpss group ***/
    s32Ret = HI_MPI_VPSS_StartGrp(VpssGrp);
    if (s32Ret != HI_SUCCESS)
    {
        g_critical("HI_MPI_VPSS_StartGrp failed with %#x\n", s32Ret);
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}
gint32 ipcam_video_process_subsystem_stop(IpcamVideoProcessSubsystem *self)
{
    g_return_val_if_fail(IPCAM_IS_VIDEO_PROCESS_SUBSYSTEM(self), HI_FAILURE);
    HI_S32 s32Ret = HI_SUCCESS;
    VPSS_GRP VpssGrp = 0;
    VPSS_CHN VpssChn = 0;

    s32Ret = HI_MPI_VPSS_StopGrp(VpssGrp);
    if (s32Ret != HI_SUCCESS)
    {
        g_critical("HI_MPI_VPSS_StopGrp failed with %#x!\n", s32Ret);
    }
    s32Ret = HI_MPI_VPSS_DisableChn(VpssGrp, VpssChn);
    if (s32Ret != HI_SUCCESS)
    {
        g_critical("HI_MPI_VPSS_DisableChn failed with %#x!\n", s32Ret);
    }
#if 0
    VpssChn = 1;
    s32Ret = HI_MPI_VPSS_DisableChn(VpssGrp, VpssChn);
    if (s32Ret != HI_SUCCESS)
    {
        g_critical("HI_MPI_VPSS_DisableChn failed with %#x!\n", s32Ret);
    }
#endif
    
    s32Ret = HI_MPI_VPSS_DestroyGrp(VpssGrp);
    if (s32Ret != HI_SUCCESS)
    {
        g_critical("HI_MPI_VPSS_DestroyGrp failed with %#x!\n", s32Ret);
    }

    return s32Ret;
}
void ipcam_video_process_subsystem_param_change(IpcamVideoProcessSubsystem *self, StreamDescriptor desc[])
{
    g_return_if_fail(IPCAM_IS_VIDEO_PROCESS_SUBSYSTEM(self));

    ipcam_video_process_subsystem_stop(self);
    ipcam_video_process_subsystem_start(self, desc);
}
