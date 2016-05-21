#include <stdlib.h>
#include <hi_type.h>
#include <hi_defines.h>
#include <hi_comm_sys.h>
#include <hi_comm_venc.h>
#include <mpi_sys.h>
#include <mpi_venc.h>
#include <hi_mem.h>

#include <glib.h>

#include "stream_descriptor.h"
#include "imedia.h"
#include "isp.h"
#include "video_input.h"
#include "video_encode.h"
#include "video_process_subsystem.h"
#include "video_detect.h"
#include "media_video.h"

enum
{
    PROP_0,
    PROP_APP,
    PROP_FD,
    N_PROPERTIES
};

typedef struct _IpcamMediaVideoPrivate
{
    IpcamIMedia *imedia;
    IpcamIsp *isp;
    IpcamVideoInput *vi;
    IpcamVideoProcessSubsystem *vpss;
    IpcamVideoEncode *venc;
    IpcamVideoDetect *vda;
} IpcamMediaVideoPrivate;

extern void signalNewFrameData(void *clientData);

#define ENABLE_VIDEO_DETECT

G_DEFINE_TYPE(IpcamMediaVideo, ipcam_media_video, G_TYPE_OBJECT);

static GParamSpec *obj_properties[N_PROPERTIES] = {NULL, };

static void ipcam_media_video_init(IpcamMediaVideo *self)
{
	IpcamMediaVideoPrivate *priv = IPCAM_MEDIA_VIDEO_GET_PRIVATE(self);

    priv->isp = NULL;
    priv->vi = NULL;
    priv->vpss = NULL;
    priv->venc = NULL;
    priv->vda = NULL;
}

static GObject *
ipcam_media_video_constructor(GType                  gtype,
                              guint                  n_properties,
                              GObjectConstructParam *properties)
{
    GObjectClass *klass;
    GObject *object;
    IpcamMediaVideo *self;
	IpcamMediaVideoPrivate *priv;
    IpcamIMedia *imedia;

    /* Always chain up to the parent constructor */
    klass = G_OBJECT_CLASS(ipcam_media_video_parent_class);
    object = klass->constructor(gtype, n_properties, properties);
    self = IPCAM_MEDIA_VIDEO(object);
    priv = IPCAM_MEDIA_VIDEO_GET_PRIVATE(self);

    g_object_get(G_OBJECT(self), "app", &imedia, NULL);

    priv->isp = g_object_new(IPCAM_ISP_TYPE, NULL);
    priv->vi = g_object_new(IPCAM_VIDEO_INPUT_TYPE, NULL);
    priv->vpss = g_object_new(IPCAM_VIDEO_PROCESS_SUBSYSTEM_TYPE, NULL);
    priv->venc = g_object_new(IPCAM_VIDEO_ENCODE_TYPE, NULL);
    priv->vda = g_object_new(IPCAM_VIDEO_DETECT_TYPE, "app", imedia, NULL);

    return object;
}

static void ipcam_media_video_finalize(GObject *object)
{
    IpcamMediaVideo *self = IPCAM_MEDIA_VIDEO(object);
    IpcamMediaVideoPrivate *priv = IPCAM_MEDIA_VIDEO_GET_PRIVATE(self);

    g_clear_object(&priv->vda);
    g_clear_object(&priv->venc);
    g_clear_object(&priv->vpss);
    g_clear_object(&priv->vi);
	g_clear_object(&priv->isp);
    G_OBJECT_CLASS(ipcam_media_video_parent_class)->finalize(object);
}

static void ipcam_media_video_get_property(GObject    *object,
                                           guint       property_id,
                                           GValue     *value,
                                           GParamSpec *pspec)
{
    IpcamMediaVideo *self = IPCAM_MEDIA_VIDEO(object);
    IpcamMediaVideoPrivate *priv = IPCAM_MEDIA_VIDEO_GET_PRIVATE(self);

    switch(property_id)
    {
    case PROP_APP:
        {
            g_value_set_object(value, priv->imedia);
        }
        break;
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

static void ipcam_media_video_set_property(GObject  *object,
                                           guint      property_id,
                                           const GValue     *value,
                                           GParamSpec *pspec)
{
    IpcamMediaVideo *self = IPCAM_MEDIA_VIDEO(object);
    IpcamMediaVideoPrivate *priv = IPCAM_MEDIA_VIDEO_GET_PRIVATE(self);

    switch(property_id)
    {
    case PROP_APP:
        {
            priv->imedia = g_value_get_object(value);
        }
        break;
    case PROP_FD:
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
    object_class->constructor = ipcam_media_video_constructor;
    object_class->finalize = &ipcam_media_video_finalize;
    object_class->get_property = &ipcam_media_video_get_property;
    object_class->set_property = &ipcam_media_video_set_property;

    obj_properties[PROP_APP] =
        g_param_spec_object("app",
                            "IMedia Application",
                            "Imedia Application",
                            IPCAM_IMEDIA_TYPE,
                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
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
    stDestChn.enModId = HI_ID_GROUP;
    stDestChn.s32DevId = 1;
    stDestChn.s32ChnId = 0;
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

	/* Master Channel */
    stDestChn.enModId = HI_ID_GROUP;
    stDestChn.s32DevId = 0;
    stDestChn.s32ChnId = 0;
    s32Ret = HI_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
    if (s32Ret != HI_SUCCESS)
    {
        g_critical("%s: HI_MPI_SYS_UnBind failed with %#x!\n", __FUNCTION__, s32Ret);
    }

	/* Slave Channel */
    stDestChn.enModId = HI_ID_GROUP;
    stDestChn.s32DevId = 1;
    stDestChn.s32ChnId = 0;
    s32Ret = HI_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
    if (s32Ret != HI_SUCCESS)
    {
        g_critical("%s: HI_MPI_SYS_UnBind failed with %#x!\n", __FUNCTION__, s32Ret);
    }

    return s32Ret;
}

gint32 ipcam_media_video_start_livestream(IpcamMediaVideo *self,
                                          StreamDescriptor desc[],
                                          OD_REGION_INFO od_reg_info[])
{
    IpcamMediaVideoPrivate *priv = IPCAM_MEDIA_VIDEO_GET_PRIVATE(self);

    ipcam_isp_start(priv->isp, desc);
    ipcam_video_input_start(priv->vi, desc);
    ipcam_video_process_subsystem_start(priv->vpss, desc);
    ipcam_video_encode_start(priv->venc, desc);

    ipcam_media_video_vpss_bind_vi(self);
    ipcam_media_video_venc_bind_vpss(self);

#if defined(ENABLE_VIDEO_DETECT)
    ipcam_video_detect_start(priv->vda, od_reg_info);
#endif

    return HI_SUCCESS;
}

gint32 ipcam_media_video_stop_livestream(IpcamMediaVideo *self)
{
    IpcamMediaVideoPrivate *priv = IPCAM_MEDIA_VIDEO_GET_PRIVATE(self);

    ipcam_media_video_venc_unbind_vpss(self);
    ipcam_media_video_vpss_unbind_vi(self);

    ipcam_video_encode_stop(priv->venc);
    ipcam_video_process_subsystem_stop(priv->vpss);
    ipcam_video_input_stop(priv->vi);
    ipcam_isp_stop(priv->isp);

#if defined(ENABLE_VIDEO_DETECT)
    ipcam_video_detect_stop(priv->vda);
#endif

    return HI_SUCCESS;
}

gint32 ipcam_media_video_query_vi_stat(IpcamMediaVideo *self, VI_CHN_STAT_S *stat)
{
    IpcamMediaVideoPrivate *priv = IPCAM_MEDIA_VIDEO_GET_PRIVATE(self);
    return ipcam_video_input_query(priv->vi, stat);
}

void ipcam_media_video_param_change(IpcamMediaVideo *self,
                                    StreamDescriptor desc[],
                                    OD_REGION_INFO *od_reg_info)
{
    IpcamMediaVideoPrivate *priv = IPCAM_MEDIA_VIDEO_GET_PRIVATE(self);

#if defined(ENABLE_VIDEO_DETECT)
    ipcam_video_detect_stop(priv->vda);
#endif
    ipcam_video_encode_stop(priv->venc);
    ipcam_video_process_subsystem_stop(priv->vpss);
    ipcam_video_input_stop(priv->vi);

    ipcam_isp_param_change (priv->isp, desc);

    ipcam_video_input_start(priv->vi, desc);
    ipcam_video_process_subsystem_start(priv->vpss, desc);
    ipcam_video_encode_start(priv->venc, desc);
#if defined(ENABLE_VIDEO_DETECT)
    ipcam_video_detect_start(priv->vda, od_reg_info);
#endif
}

void ipcam_media_video_set_image_parameter(IpcamMediaVideo *self, IpcamMediaImageAttr *attr)
{
    IpcamMediaVideoPrivate *priv = IPCAM_MEDIA_VIDEO_GET_PRIVATE(self);

    ipcam_video_input_set_image_parameter(priv->vi, attr);
}

void ipcam_media_video_set_color2grey(IpcamMediaVideo *self, gboolean enabled)
{
	IpcamMediaVideoPrivate *priv = IPCAM_MEDIA_VIDEO_GET_PRIVATE(self);

	if (enabled)
		ipcam_video_encode_enable_color2grey(priv->venc);
	else
		ipcam_video_encode_disable_color2grey(priv->venc);
}

void ipcam_media_video_set_antiflicker(IpcamMediaVideo *self, gboolean enable, guint8 freq)
{
	IpcamMediaVideoPrivate *priv = IPCAM_MEDIA_VIDEO_GET_PRIVATE(self);

	ipcam_isp_set_antiflicker(priv->isp, enable, freq);
}
