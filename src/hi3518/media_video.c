#include <stdlib.h>
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
    volatile gboolean livestream_flag;
    GThread *livestream;
    IpcamBufferManager *buffer_manager;
    GList *notify_source_list;
    GMutex mutex;
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
    g_clear_object(&priv->buffer_manager);
    g_mutex_lock(&priv->mutex);
    g_list_free(priv->notify_source_list);
    g_mutex_unlock(&priv->mutex);
    g_mutex_clear(&priv->mutex);
    G_OBJECT_CLASS(ipcam_media_video_parent_class)->finalize(object);
}
static void ipcam_media_video_init(IpcamMediaVideo *self)
{
	IpcamMediaVideoPrivate *priv = IPCAM_MEDIA_VIDEO_GET_PRIVATE(self);
    priv->isp = g_object_new(IPCAM_ISP_TYPE, NULL);
    priv->vi = g_object_new(IPCAM_VIDEO_INPUT_TYPE, NULL);
    priv->vpss = g_object_new(IPCAM_VIDEO_PROCESS_SUBSYSTEM_TYPE, NULL);
    priv->venc = g_object_new(IPCAM_VIDEO_ENCODE_TYPE, NULL);
    priv->buffer_manager = g_object_new(IPCAM_BUFFER_MANAGER_TYPE, NULL);
    g_mutex_init(&priv->mutex);
    priv->notify_source_list = NULL;
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
/*
static void ipcam_media_video_set_property(GObject      *object,
                                           guint         property_id,
                                           const GValue *value,
                                           GParamSpec   *pspec)
{
    IpcamMediaVideo *self = IPCAM_MEDIA_VIDEO(object);
    IpcamMediaVideoPrivate *priv = IPCAM_MEDIA_VIDEO_GET_PRIVATE(self);
    switch(property_id)
    {
    case PROP_FD:
        {
            priv->venc_fd = g_value_get_int(value);
        }
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}
*/
static void ipcam_media_video_class_init(IpcamMediaVideoClass *klass)
{
    g_type_class_add_private(klass, sizeof(IpcamMediaVideoPrivate));
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = &ipcam_media_video_finalize;
    object_class->get_property = &ipcam_media_video_get_property;
    //object_class->set_property = &ipcam_media_video_set_property;

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
    ipcam_isp_start(priv->isp);
    ipcam_video_input_start(priv->vi, desc);
    ipcam_video_process_subsystem_start(priv->vpss, desc);
    ipcam_video_encode_start(priv->venc, desc);

    ipcam_media_video_vpss_bind_vi(self);
    ipcam_media_video_venc_bind_vpss(self);

    priv->livestream_flag = TRUE;
    priv->livestream = g_thread_new("livestream",
                                    ipcam_media_video_livestream,
                                    self);

    return HI_SUCCESS;
}
static gint32 ipcam_media_video_stop_livestream(IpcamMediaVideo *self)
{
    IpcamMediaVideoPrivate *priv = IPCAM_MEDIA_VIDEO_GET_PRIVATE(self);

    priv->livestream_flag = FALSE;
    g_thread_join(priv->livestream);

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

    ipcam_video_input_param_change(priv->vi, desc);
    
}
static gpointer ipcam_media_video_get_write_data(IpcamMediaVideo *self)
{
    IpcamMediaVideoPrivate *priv = IPCAM_MEDIA_VIDEO_GET_PRIVATE(self);
    return ipcam_buffer_manager_get_write_data(priv->buffer_manager);
}
static void ipcam_media_video_release_write_data(IpcamMediaVideo *self, gpointer data)
{
    IpcamMediaVideoPrivate *priv = IPCAM_MEDIA_VIDEO_GET_PRIVATE(self);
    ipcam_buffer_manager_release_write_data(priv->buffer_manager, data);
}
static gpointer ipcam_media_video_get_read_data(IpcamMediaVideo *self)
{
    IpcamMediaVideoPrivate *priv = IPCAM_MEDIA_VIDEO_GET_PRIVATE(self);
    return ipcam_buffer_manager_get_read_data(priv->buffer_manager);
}
static void ipcam_media_video_release_read_data(IpcamMediaVideo *self, gpointer data)
{
    IpcamMediaVideoPrivate *priv = IPCAM_MEDIA_VIDEO_GET_PRIVATE(self);
    ipcam_buffer_manager_release_read_data(priv->buffer_manager, data);
}
static gboolean ipcam_media_video_has_video_data(IpcamMediaVideo *self)
{
    IpcamMediaVideoPrivate *priv = IPCAM_MEDIA_VIDEO_GET_PRIVATE(self);
    return ipcam_buffer_manager_has_data(priv->buffer_manager);
}
static void ipcam_media_video_register_rtsp_source(IpcamMediaVideo *self, void *source)
{
    IpcamMediaVideoPrivate *priv = IPCAM_MEDIA_VIDEO_GET_PRIVATE(self);
    g_mutex_lock(&priv->mutex);
    priv->notify_source_list = g_list_append(priv->notify_source_list, source);
    g_mutex_unlock(&priv->mutex);
}
static void ipcam_media_video_unregister_rtsp_source(IpcamMediaVideo *self, void *source)
{
    IpcamMediaVideoPrivate *priv = IPCAM_MEDIA_VIDEO_GET_PRIVATE(self);
    g_mutex_lock(&priv->mutex);
    priv->notify_source_list = g_list_remove(priv->notify_source_list, source);
    g_mutex_unlock(&priv->mutex);
}
static void ipcam_media_video_notify_rtsp_source(IpcamMediaVideo *self)
{
    IpcamMediaVideoPrivate *priv = IPCAM_MEDIA_VIDEO_GET_PRIVATE(self);
    g_mutex_lock(&priv->mutex);
    GList *source = g_list_first(priv->notify_source_list);
    for (; source; source = g_list_next(source))
    {
        if (source && source->data)
        {
            signalNewFrameData(source->data);
        }
    }
    g_mutex_unlock(&priv->mutex);
}

static guint ipcam_media_video_get_framesize(VENC_STREAM_S *pstStream)
{
    guint newFrameSize = 0;
    gint i = 0;
    HI_U8 *p = NULL;
    
    for (i = 0; i < pstStream->u32PackCount; i++)
    {
        newFrameSize += (pstStream->pstPack[i].u32Len[0]);
        p = pstStream->pstPack[i].pu8Addr[0];
        if (p[0] == 0x00 && p[1] == 0x00 && p[2] == 0x00 && p[3] == 0x01)
        {
            newFrameSize -= 4;
        }
        if (pstStream->pstPack[i].u32Len[1] > 0)
        {
            newFrameSize += (pstStream->pstPack[i].u32Len[1]);
        }
    }
    return newFrameSize;
}

static void ipcam_media_video_copy_data(IpcamMediaVideo *self, VENC_STREAM_S *pstStream,
                                        StreamData *video_data, guint32 newFrameSize)
{
    HI_S32 i = 0;
    HI_U32 pos = 0;
    HI_U8 *p = NULL;

    video_data->magic = 0xDEADBEEF;
    video_data->pts.tv_sec = pstStream->pstPack[0].u64PTS / 1000000ULL;
    video_data->pts.tv_usec = pstStream->pstPack[0].u64PTS % 1000000ULL;
    // Deliver the data here:
    video_data->len = newFrameSize;
    video_data->isIFrame =
        /* pstStream->pstPack[0].DataType.enH264EType == H264E_NALU_ISLICE || */
        /* pstStream->pstPack[0].DataType.enH264EType == H264E_NALU_SEI || */
        pstStream->pstPack[0].DataType.enH264EType == H264E_NALU_SPS ||
        pstStream->pstPack[0].DataType.enH264EType == H264E_NALU_PPS;

    for (i = 0; i < pstStream->u32PackCount; i++)
    {
        p = pstStream->pstPack[i].pu8Addr[0];
        if (p[0] == 0x00 && p[1] == 0x00 && p[2] == 0x00 && p[3] == 0x01)
        {
            memcpy(video_data->data + pos, p + 4, pstStream->pstPack[i].u32Len[0] - 4);
        }
        else
        {
            memcpy(video_data->data + pos, p, pstStream->pstPack[i].u32Len[0]);
        }
        pos += pstStream->pstPack[i].u32Len[0];
        if (pstStream->pstPack[i].u32Len[1] > 0)
        {
            p = pstStream->pstPack[i].pu8Addr[1];
            memcpy(video_data->data + pos, p, pstStream->pstPack[i].u32Len[1]);
            pos += pstStream->pstPack[i].u32Len[1];
        }
    }
}

static void ipcam_media_video_process_data(IpcamMediaVideo *self, VENC_STREAM_S *pstStream)
{
    guint newFrameSize = ipcam_media_video_get_framesize(pstStream);
    
    if (newFrameSize > 0)
    {
        StreamData *video_data = ipcam_media_video_get_write_data(self);
        if (video_data)
        {
            ipcam_media_video_copy_data(self, pstStream, video_data,
                                        newFrameSize < ((1024 * 1024) - sizeof(StreamData)) ? newFrameSize : ((1024 * 1024) - sizeof(StreamData)));
            ipcam_media_video_release_write_data(self, video_data);
            ipcam_media_video_notify_rtsp_source(self);
        }
    }
}

#define MAX_FRAME_PACK_NUM  4

static gpointer ipcam_media_video_livestream(gpointer data)
{
    IpcamMediaVideo *media_video = IPCAM_MEDIA_VIDEO(data);
    IpcamMediaVideoPrivate *priv = IPCAM_MEDIA_VIDEO_GET_PRIVATE(media_video);
    struct timeval TimeoutVal;
    fd_set read_fds;
    HI_S32 VencFd;
    VENC_CHN_STAT_S stStat;
    VENC_STREAM_S stStream;
    HI_S32 s32Ret;
    HI_U32 u32PackCount = MAX_FRAME_PACK_NUM;

    memset(&stStream, 0, sizeof(stStream));
    stStream.pstPack = g_new(VENC_PACK_S, u32PackCount);

    /******************************************
     step 1:  check & prepare save-file & venc-fd
    ******************************************/
    /* Set Venc Fd. */
    VencFd = HI_MPI_VENC_GetFd(0);
    if (VencFd < 0)
    {
        g_critical("HI_MPI_VENC_GetFd failed with %#x!\n", VencFd);
    }

    /******************************************
     step 2:  Start to get streams of each channel.
    ******************************************/
    while (TRUE == priv->livestream_flag)
    {
        FD_ZERO(&read_fds);
        FD_SET(VencFd, &read_fds);

        TimeoutVal.tv_sec  = 1;
        TimeoutVal.tv_usec = 0;
        s32Ret = select(VencFd + 1, &read_fds, NULL, NULL, &TimeoutVal);
        if (s32Ret < 0)
        {
            g_critical("select failed with %#x!\n", s32Ret);
            break;
        }
        else if (s32Ret == 0)
        {
            //g_warning("get venc stream time out, continue!\n");
            continue;
        }
        else
        {
            if (FD_ISSET(VencFd, &read_fds))
            {
                /*******************************************************
                 step 2.1 : query how many packs in one-frame stream.
                *******************************************************/
                s32Ret = HI_MPI_VENC_Query(0, &stStat);
                if (HI_SUCCESS != s32Ret)
                {
                    g_print("HI_MPI_VENC_Query chn[%d] failed with %#x!\n", 0, s32Ret);
                    continue;
                }

                /*******************************************************
                 step 2.3 : call mpi to get one-frame stream
                *******************************************************/
                if (stStat.u32CurPacks > u32PackCount)
                {
                    u32PackCount = stStat.u32CurPacks;
                    stStream.pstPack = g_renew(VENC_PACK_S, stStream.pstPack, u32PackCount);
                }
                stStream.u32PackCount = stStat.u32CurPacks;
                stStream.u32Seq = 0;
                memset(&stStream.stH264Info, 0, sizeof(VENC_STREAM_INFO_H264_S));
                s32Ret = HI_MPI_VENC_GetStream(0, &stStream, HI_TRUE);
                if (HI_SUCCESS != s32Ret)
                {
                    g_critical("HI_MPI_VENC_GetStream failed with %#x!\n", s32Ret);
                    continue;
                }

                /*******************************************************
                 step 2.4 : send frame to live stream
                *******************************************************/
                if (g_list_length(priv->notify_source_list) > 0)
                {
                    ipcam_media_video_process_data(media_video, &stStream);
                }
                /*******************************************************
                 step 2.5 : release stream
                *******************************************************/
                s32Ret = HI_MPI_VENC_ReleaseStream(0, &stStream);
                if (HI_SUCCESS != s32Ret)
                {
                    g_critical("HI_MPI_VENC_ReleaseStream failed with %#x!\n", s32Ret);
                }
            }
        }
    }
    g_free(stStream.pstPack);
    stStream.pstPack = NULL;
    /*******************************************************
     * step 3 : close save-file
     *******************************************************/
    g_thread_exit(0);
    return NULL;
}

static void ipcam_ivideo_interface_init(IpcamIVideoInterface *iface)
{
    iface->start = ipcam_media_video_start_livestream;
    iface->stop = ipcam_media_video_stop_livestream;
    iface->param_change = ipcam_media_video_param_change;
    iface->get_video_data = ipcam_media_video_get_read_data;
    iface->release_video_data = ipcam_media_video_release_read_data;
    iface->has_video_data = ipcam_media_video_has_video_data;
    iface->register_rtsp_source = ipcam_media_video_register_rtsp_source;
    iface->unregister_rtsp_source = ipcam_media_video_unregister_rtsp_source;
}
