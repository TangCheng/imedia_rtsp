#include <stdlib.h>
#include <glib.h>
#include <hi_type.h>
#include <hi_defines.h>
#include <hi_comm_sys.h>
#include <hi_comm_venc.h>
#include <mpi_sys.h>
#include <mpi_venc.h>
#include <hi_mem.h>
#include "stream_descriptor.h"
#include "buffer_manager.h"
#include "interface/media_video_interface.h"
#include "media_video.h"
#include "isp.h"
#include "video_input.h"
#include "video_encode.h"
#include "video_process_subsystem.h"

enum
{
    PROP_0,
    PROP_FD,
    N_PROPERTIES
};

typedef struct _IpcamMediaVideoPrivate
{
    IpcamIsp *isp;
    IpcamVideoInput *vi;
    IpcamVideoProcessSubsystem *vpss;
    IpcamVideoEncode *venc;
} IpcamMediaVideoPrivate;

extern void signalNewFrameData(void *clientData);
static void ipcam_ivideo_interface_init(IpcamIVideoInterface *iface);
static gpointer ipcam_media_video_livestream(gpointer data);

G_DEFINE_TYPE_WITH_CODE(IpcamMediaVideo, ipcam_media_video, G_TYPE_OBJECT,
                        G_IMPLEMENT_INTERFACE(IPCAM_TYPE_IVIDEO,
                                              ipcam_ivideo_interface_init));

static GParamSpec *obj_properties[N_PROPERTIES] = {NULL, };

static void ipcam_media_video_finalize(GObject *object)
{
    IpcamMediaVideo *self = IPCAM_MEDIA_VIDEO(object);
    IpcamMediaVideoPrivate *priv = IPCAM_MEDIA_VIDEO_GET_PRIVATE(self);

	g_clear_object(&priv->isp);
    g_clear_object(&priv->vi);
    g_clear_object(&priv->vpss);
    g_clear_object(&priv->venc);
    G_OBJECT_CLASS(ipcam_media_video_parent_class)->finalize(object);
}

static void ipcam_media_video_init(IpcamMediaVideo *self)
{
	IpcamMediaVideoPrivate *priv = IPCAM_MEDIA_VIDEO_GET_PRIVATE(self);
    priv->isp = g_object_new(IPCAM_ISP_TYPE, NULL);
    priv->vi = g_object_new(IPCAM_VIDEO_INPUT_TYPE, NULL);
    priv->vpss = g_object_new(IPCAM_VIDEO_PROCESS_SUBSYSTEM_TYPE, NULL);
    priv->venc = g_object_new(IPCAM_VIDEO_ENCODE_TYPE, NULL);
}

static void ipcam_media_video_get_property(GObject    *object,
                                           guint       property_id,
                                           GValue     *value,
                                           GParamSpec *pspec)
{
    switch(property_id)
    {
    case PROP_FD:
        {
            g_value_set_int(value, HI_MPI_VENC_GetFd(0));
        }
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

static void ipcam_media_video_class_init(IpcamMediaVideoClass *klass)
{
    g_type_class_add_private(klass, sizeof(IpcamMediaVideoPrivate));
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = &ipcam_media_video_finalize;
    object_class->get_property = &ipcam_media_video_get_property;

    obj_properties[PROP_FD] =
        g_param_spec_int("venc_fd",
                         "xxx",
                         "xxx.",
                         G_MININT,
                         G_MAXINT,
                         -1, // default value
                         G_PARAM_READABLE);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static gint32 ipcam_media_video_vpss_bind_vi(IpcamMediaVideo *self)
{
    HI_S32 s32Ret = HI_SUCCESS;
    MPP_CHN_S stSrcChn;
    MPP_CHN_S stDestChn;

    stSrcChn.enModId = HI_ID_VIU;
    stSrcChn.s32DevId = 0;
    stSrcChn.s32ChnId = 0;

    stDestChn.enModId = HI_ID_VPSS;
    stDestChn.s32DevId = 0;
    stDestChn.s32ChnId = 0;

    s32Ret = HI_MPI_SYS_Bind(&stSrcChn, &stDestChn);
    if (s32Ret != HI_SUCCESS)
    {
        g_critical("%s: HI_MPI_SYS_Bind failed with %#x!\n", __FUNCTION__, s32Ret);
    }

    return s32Ret;
}

static gint32 ipcam_media_video_vpss_unbind_vi(IpcamMediaVideo *self)
{
    HI_S32 s32Ret = HI_SUCCESS;
    MPP_CHN_S stSrcChn;
    MPP_CHN_S stDestChn;

    stSrcChn.enModId = HI_ID_VIU;
    stSrcChn.s32DevId = 0;
    stSrcChn.s32ChnId = 0;

    stDestChn.enModId = HI_ID_VPSS;
    stDestChn.s32DevId = 0;
    stDestChn.s32ChnId = 0;

    s32Ret = HI_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
    if (s32Ret != HI_SUCCESS)
    {
        g_critical("%s: HI_MPI_SYS_UnBind failed with %#x!\n", __FUNCTION__, s32Ret);
    }

    return s32Ret;
}

static gint32 ipcam_media_video_venc_bind_vpss(IpcamMediaVideo *self)
{
    HI_S32 s32Ret = HI_SUCCESS;
    MPP_CHN_S stSrcChn;
    MPP_CHN_S stDestChn;

	/* Master channel */
    stSrcChn.enModId = HI_ID_VPSS;
    stSrcChn.s32DevId = 0;
    stSrcChn.s32ChnId = 0;
    stDestChn.enModId = HI_ID_GROUP;
    stDestChn.s32DevId = 0;
    stDestChn.s32ChnId = 0;
    s32Ret = HI_MPI_SYS_Bind(&stSrcChn, &stDestChn);
    if (s32Ret != HI_SUCCESS)
    {
        g_critical("%s: HI_MPI_SYS_Bind failed with %#x!\n", __FUNCTION__, s32Ret);
    }

	/* Slave channel */
    stSrcChn.enModId = HI_ID_VPSS;
    stSrcChn.s32DevId = 0;
    stSrcChn.s32ChnId = 1;
    stDestChn.enModId = HI_ID_GROUP;
    stDestChn.s32DevId = 1;
    stDestChn.s32ChnId = 1;
    s32Ret = HI_MPI_SYS_Bind(&stSrcChn, &stDestChn);
    if (s32Ret != HI_SUCCESS)
    {
        g_critical("%s: HI_MPI_SYS_Bind failed with %#x!\n", __FUNCTION__, s32Ret);
    }

    return s32Ret;
}

static gint32 ipcam_media_video_venc_unbind_vpss(IpcamMediaVideo *self)
{
    HI_S32 s32Ret = HI_SUCCESS;
    MPP_CHN_S stSrcChn;
    MPP_CHN_S stDestChn;

    stSrcChn.enModId = HI_ID_VPSS;
    stSrcChn.s32DevId = 0;
    stSrcChn.s32ChnId = 0;

    stDestChn.enModId = HI_ID_GROUP;
    stDestChn.s32DevId = 0;
    stDestChn.s32ChnId = 0;

    s32Ret = HI_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
    if (s32Ret != HI_SUCCESS)
    {
        g_critical("%s: HI_MPI_SYS_UnBind failed with %#x!\n", __FUNCTION__, s32Ret);
    }

    return s32Ret;
}

static gint32 ipcam_media_video_start_livestream(IpcamMediaVideo *self, StreamDescriptor desc[])
{
    IpcamMediaVideoPrivate *priv = IPCAM_MEDIA_VIDEO_GET_PRIVATE(self);

    ipcam_isp_start(priv->isp, desc);
    ipcam_video_input_start(priv->vi, desc);
    ipcam_video_process_subsystem_start(priv->vpss, desc);
    ipcam_video_encode_start(priv->venc, desc);

    ipcam_media_video_vpss_bind_vi(self);
    ipcam_media_video_venc_bind_vpss(self);

    return HI_SUCCESS;
}

static gint32 ipcam_media_video_stop_livestream(IpcamMediaVideo *self)
{
    IpcamMediaVideoPrivate *priv = IPCAM_MEDIA_VIDEO_GET_PRIVATE(self);

    ipcam_media_video_venc_unbind_vpss(self);
    ipcam_media_video_vpss_unbind_vi(self);

    ipcam_video_encode_stop(priv->venc);
    ipcam_video_process_subsystem_stop(priv->vpss);
    ipcam_video_input_stop(priv->vi);
    ipcam_isp_stop(priv->isp);

    return HI_SUCCESS;
}

static void ipcam_media_video_param_change(IpcamMediaVideo *self, StreamDescriptor desc[])
{
    IpcamMediaVideoPrivate *priv = IPCAM_MEDIA_VIDEO_GET_PRIVATE(self);

#if 0
    ipcam_isp_param_change(priv->isp, desc);
    ipcam_video_input_param_change(priv->vi, desc);
	ipcam_video_process_subsystem_param_change(priv->vpss, desc);
	ipcam_video_encode_param_change(priv->venc, desc);
#endif
    ipcam_video_encode_stop(priv->venc);
    ipcam_video_process_subsystem_stop(priv->vpss);
    ipcam_video_input_stop(priv->vi);

    ipcam_isp_param_change (priv->isp, desc);

    ipcam_video_input_start(priv->vi, desc);
    ipcam_video_process_subsystem_start(priv->vpss, desc);
    ipcam_video_encode_start(priv->venc, desc);
}

void ipcam_media_video_set_image_parameter(IpcamMediaVideo *self,
                                           gint32 brightness,
                                           gint32 chrominance,
                                           gint32 contrast,
                                           gint32 saturation)
{
    IpcamMediaVideoPrivate *priv = IPCAM_MEDIA_VIDEO_GET_PRIVATE(self);

    ipcam_video_input_set_image_parameter(priv->vi, brightness, chrominance, contrast, saturation);
}

static void ipcam_ivideo_interface_init(IpcamIVideoInterface *iface)
{
    iface->start = ipcam_media_video_start_livestream;
    iface->stop = ipcam_media_video_stop_livestream;
    iface->param_change = ipcam_media_video_param_change;
}
