#include <hi_type.h>
#include <hi_defines.h>
#include <hi_comm_sys.h>
#include <hi_comm_venc.h>
#include <mpi_sys.h>
#include <mpi_venc.h>
//#include <memory.h>
#include <hi_mem.h>
#include <zmq.h>
#include "stream_descriptor.h"
#include "media_video_interface.h"
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
    IpcamMediaVideo *self = IPCAM_MEDIA_VIDEO(object);
    IpcamMediaVideoPrivate *priv = IPCAM_MEDIA_VIDEO_GET_PRIVATE(self);
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
static gint32 ipcam_media_video_start_livestream(IpcamMediaVideo *self)
{
    IpcamMediaVideoPrivate *priv = IPCAM_MEDIA_VIDEO_GET_PRIVATE(self);

    ipcam_isp_start(priv->isp);
    ipcam_video_input_start(priv->vi);
    ipcam_video_process_subsystem_start(priv->vpss);
    ipcam_video_encode_start(priv->venc);

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
#if 0
static gpointer ipcam_media_video_livestream(gpointer data)
{
    IpcamMediaVideoPrivate *priv = (IpcamMediaVideoPrivate *)data;
    HI_S32 i = 0;
    HI_S32 maxfd = 0;
    struct timeval TimeoutVal;
    fd_set read_fds;
    HI_S32 VencFd;
    VENC_CHN_STAT_S stStat;
    VENC_STREAM_S stStream;
    HI_S32 s32Ret;
    
    /******************************************
     step 1:  check & prepare save-file & venc-fd
    ******************************************/
    /* Set Venc Fd. */
    VencFd = HI_MPI_VENC_GetFd(0);
    if (VencFd < 0)
    {
        g_critical("HI_MPI_VENC_GetFd failed with %#x!\n", VencFd);
    }
    if (maxfd <= VencFd)
    {
        maxfd = VencFd;
    }

    /******************************************
     step 2:  Start to get streams of each channel.
    ******************************************/
    while (TRUE == priv->livestream_flag)
    {
        FD_ZERO(&read_fds);
        FD_SET(VencFd, &read_fds);

        TimeoutVal.tv_sec  = 2;
        TimeoutVal.tv_usec = 0;
        s32Ret = select(maxfd + 1, &read_fds, NULL, NULL, &TimeoutVal);
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
                memset(&stStream, 0, sizeof(stStream));
                s32Ret = HI_MPI_VENC_Query(0, &stStat);
                if (HI_SUCCESS != s32Ret)
                {
                    g_print("HI_MPI_VENC_Query chn[%d] failed with %#x!\n", 0, s32Ret);
                    break;
                }

                /*******************************************************
                 step 2.2 : malloc corresponding number of pack nodes.
                *******************************************************/
                stStream.pstPack = g_new(VENC_PACK_S, stStat.u32CurPacks);
                if (NULL == stStream.pstPack)
                {
                    g_critical("malloc stream pack failed!\n");
                    break;
                }
                    
                /*******************************************************
                 step 2.3 : call mpi to get one-frame stream
                *******************************************************/
                stStream.u32PackCount = stStat.u32CurPacks;
                s32Ret = HI_MPI_VENC_GetStream(0, &stStream, HI_TRUE);
                if (HI_SUCCESS != s32Ret)
                {
                    g_free(stStream.pstPack);
                    stStream.pstPack = NULL;
                    g_critical("HI_MPI_VENC_GetStream failed with %#x!\n", s32Ret);
                    break;
                }

                /*******************************************************
                 step 2.4 : send frame to live stream
                *******************************************************/
                unsigned newFrameSize = 0; //%%% TO BE WRITTEN %%%
                HI_U8 *p = NULL;
                for (i = 0; i < stStream.u32PackCount; i++)
                {
                    newFrameSize += (stStream.pstPack[i].u32Len[0]);
                    p = stStream.pstPack[i].pu8Addr[0];
                    if (p[0] == 0x00 && p[1] == 0x00 && p[2] == 0x00 && p[3] == 0x01)
                    {
                        newFrameSize -= 4;
                    }
                    if (stStream.pstPack[i].u32Len[1] > 0)
                    {
                        newFrameSize += (stStream.pstPack[i].u32Len[1]);
                    }
                }
                if (newFrameSize > 0)
                {
                    vsd->pts.tv_sec = stStream.pstPack[0].u64PTS / 1000000;
                    vsd->pts.tv_usec = stStream.pstPack[0].u64PTS % 1000000;
                    // Deliver the data here:
                    vsd->len = newFrameSize;
                    zmq_send(priv->publisher, vsd, sizeof(VideoStreamData), flags);
                    HI_U64 pos = 0;
                    HI_U64 left = 0;
                    for (i = 0; i < stStream.u32PackCount; i++)
                    {
                        left = (vsd->len - pos);
                        p = stStream.pstPack[i].pu8Addr[0];
                        if (p[0] == 0x00 && p[1] == 0x00 && p[2] == 0x00 && p[3] == 0x01)
                        {
                            left = MIN(left, stStream.pstPack[i].u32Len[0] - 4);
                            //left = left < (stStream.pstPack[i].u32Len[0] - 4) ? left : (stStream.pstPack[i].u32Len[0] - 4);
                            //memcpy(vsd->data + pos, p + 4, left);
                            if (pos + left >= vsd->len) flags = 0;
                            zmq_send(priv->publisher, p + 4, left, flags);
                        }
                        else
                        {
                            left = MIN(left, stStream.pstPack[i].u32Len[0]);
                            //memcpy(vsd->data + pos, p, left);
                            if (pos + left >= vsd->len) flags = 0;
                            zmq_send(priv->publisher, p, left, flags);
                        }
                        pos += left;
                        if (pos >= vsd->len) break;
                        if (stStream.pstPack[i].u32Len[1] > 0)
                        {
                            left = (vsd->len - pos);
                            p = stStream.pstPack[i].pu8Addr[1];
                            /*
                            if (p[0] == 0x00 && p[1] == 0x00 && p[2] == 0x00 && p[3] == 0x01)
                            {
                                left = MIN(left, stStream.pstPack[i].u32Len[1] - 4);
                                //memcpy(vsd->data + pos, p + 4, left);
                                if (pos + left >= vsd->len) flags = 0;
                                zmq_send(priv->publisher, p + 4, left, flags);
                            }
                            else
                            */
                            {
                                left = MIN(left, stStream.pstPack[i].u32Len[1]);
                                //memcpy(vsd->data + pos, p, left);
                                if (pos + left >= vsd->len) flags = 0;
                                zmq_send(priv->publisher, p, left, flags);
                            }
                            pos += left;
                            if (pos >= vsd->len) break;
                        }
                    }
                    //s32Ret = zmq_send(priv->publisher, vsd, sizeof(VideoStreamData) + newFrameSize, 0);
                    g_free(vsd);
                }
                
                /*******************************************************
                 step 2.5 : release stream
                *******************************************************/
                s32Ret = HI_MPI_VENC_ReleaseStream(0, &stStream);
                if (HI_SUCCESS != s32Ret)
                {
                    g_free(stStream.pstPack);
                    stStream.pstPack = NULL;
                    break;
                }
                /*******************************************************
                 step 2.6 : free pack nodes
                *******************************************************/
                g_free(stStream.pstPack);
                stStream.pstPack = NULL;
            }
        }
    }

    /*******************************************************
     * step 3 : close save-file
     *******************************************************/
    g_thread_exit(0);
    return NULL;
}
#endif

static void ipcam_ivideo_interface_init(IpcamIVideoInterface *iface)
{
    iface->start = ipcam_media_video_start_livestream;
    iface->stop = ipcam_media_video_stop_livestream;
}
