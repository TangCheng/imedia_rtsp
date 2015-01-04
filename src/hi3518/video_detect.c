#include <hi_defines.h>
#include <hi_comm_sys.h>
#include <hi_comm_vda.h>
#include <mpi_vda.h>
#include <mpi_sys.h>
#include <stdlib.h>
#include "imedia.h"
#include "video_detect.h"

enum
{
    PROP_0,
    PROP_APP,
    PROP_IMAGE_WIDTH,
    PROP_IMAGE_HEIGHT,
    N_PROPERTIES
};

typedef struct _IpcamVideoDetectRegionState
{
    gboolean Enabled;
    gboolean Occlusion;
    guint Count;
} IpcamVideoDetectRegionState;

typedef struct _IpcamVideoDetectPrivate
{
    IpcamIMedia *imedia;
    guint32 image_width;
    guint32 image_height;

    GThread *thread;
    gboolean terminated;

    IpcamVideoDetectRegionState region_state[VDA_OD_RGN_NUM_MAX];
} IpcamVideoDetectPrivate;

G_DEFINE_TYPE_WITH_PRIVATE(IpcamVideoDetect, ipcam_video_detect, G_TYPE_OBJECT)

static GParamSpec *obj_properties[N_PROPERTIES] = {NULL, };

static gpointer ipcam_video_detect_thread_handler(gpointer data);


static void ipcam_video_detect_init(IpcamVideoDetect *self)
{
	IpcamVideoDetectPrivate *priv = ipcam_video_detect_get_instance_private(self);
    int i;

    priv->imedia = NULL;
    priv->image_width = IMAGE_MAX_WIDTH;
    priv->image_height = IMAGE_MAX_HEIGHT;
    priv->thread = NULL;
    priv->terminated = FALSE;

    for (i = 0; i < ARRAY_SIZE(priv->region_state); i++) {
        priv->region_state[i].Enabled = FALSE;
        priv->region_state[i].Occlusion = FALSE;
    }
}

static void ipcam_video_detect_finalize(GObject *object)
{
    IpcamVideoDetect *self = IPCAM_VIDEO_DETECT(object);
	IpcamVideoDetectPrivate *priv = ipcam_video_detect_get_instance_private(self);

    if (priv->thread) {
        priv->terminated = TRUE;
        g_thread_join(priv->thread);
    }
}

static void ipcam_video_detect_get_property(GObject    *object,
                                           guint       property_id,
                                           GValue     *value,
                                           GParamSpec *pspec)
{
    IpcamVideoDetect *self = IPCAM_VIDEO_DETECT(object);
    IpcamVideoDetectPrivate *priv = ipcam_video_detect_get_instance_private(self);
    switch(property_id)
    {
    case PROP_APP:
        {
            g_value_set_object(value, priv->imedia);
        }
        break;
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

static void ipcam_video_detect_set_property(GObject     *object,
                                            guint         property_id,
                                            const GValue *value,
                                            GParamSpec    *pspec)
{
    IpcamVideoDetect *self = IPCAM_VIDEO_DETECT(object);
    IpcamVideoDetectPrivate *priv = ipcam_video_detect_get_instance_private(self);
    switch(property_id)
    {
    case PROP_APP:
        {
            priv->imedia = g_value_get_object(value);
        }
        break;
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

static void ipcam_video_detect_class_init(IpcamVideoDetectClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = ipcam_video_detect_get_property;
    object_class->set_property = ipcam_video_detect_set_property;
    object_class->finalize = ipcam_video_detect_finalize;

    obj_properties[PROP_APP] =
        g_param_spec_object("app",
                            "IMedia Application",
                            "Imedia Application",
                            IPCAM_IMEDIA_TYPE,
                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
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

gint32 ipcam_video_detect_start(IpcamVideoDetect *self, StreamDescriptor desc[])
{
    IpcamVideoDetectPrivate *priv = ipcam_video_detect_get_instance_private(self);
    HI_S32 s32Ret = HI_SUCCESS;
    VDA_CHN VdaChn;
    VDA_CHN_ATTR_S stVdaChnAttr;
    MPP_CHN_S stSrcChn;
    MPP_CHN_S stDestChn;
    int i;

    g_return_val_if_fail(IPCAM_IS_VIDEO_DETECT(self), HI_FAILURE);

    VdaChn = 0;
    stVdaChnAttr.enWorkMode = VDA_WORK_MODE_OD;
    stVdaChnAttr.u32Width = 320;
    stVdaChnAttr.u32Height = 240;

    stVdaChnAttr.unAttr.stOdAttr.enVdaAlg = VDA_ALG_BG;
    stVdaChnAttr.unAttr.stOdAttr.enMbSize = VDA_MB_16PIXEL;
    stVdaChnAttr.unAttr.stOdAttr.enMbSadBits = VDA_MB_SAD_8BIT;
    stVdaChnAttr.unAttr.stOdAttr.enRefMode = VDA_REF_MODE_DYNAMIC;
    stVdaChnAttr.unAttr.stOdAttr.u32VdaIntvl = 2;
    stVdaChnAttr.unAttr.stOdAttr.u32BgUpSrcWgt = 128;

    stVdaChnAttr.unAttr.stOdAttr.u32RgnNum = 1;
    for (i = 0; i < stVdaChnAttr.unAttr.stOdAttr.u32RgnNum; i++) {
        VDA_OD_RGN_ATTR_S *rgn_attr = &stVdaChnAttr.unAttr.stOdAttr.astOdRgnAttr[i];

        rgn_attr->stRect.s32X = 0;
        rgn_attr->stRect.s32Y = 0;
        rgn_attr->stRect.u32Width = 320;
        rgn_attr->stRect.u32Height = 240;
        rgn_attr->u32SadTh = 260;
        rgn_attr->u32AreaTh = 60; /* 60% */
        rgn_attr->u32OccCntTh = 4;
        rgn_attr->u32UnOccCntTh = 1;
    }

    /* Create Channel */
    s32Ret = HI_MPI_VDA_CreateChn(VdaChn, &stVdaChnAttr);
    if(s32Ret != HI_SUCCESS)
    {
        g_critical("HI_MPI_VDA_CreateChn(%d) failed: 0x%08x\n", VdaChn, s32Ret);
        return s32Ret;
    }
    /* Bind VI */
    stSrcChn.enModId = HI_ID_VIU;
    stSrcChn.s32DevId = 0;
    stSrcChn.s32ChnId = 1;
    stDestChn.enModId = HI_ID_VDA;
    stDestChn.s32DevId = 0;
    stDestChn.s32ChnId = VdaChn;
    s32Ret = HI_MPI_SYS_Bind(&stSrcChn, &stDestChn);
    if(s32Ret != HI_SUCCESS)
    {
        g_critical("HI_MPI_SYS_Bind(%d) failed: 0x%08x\n", VdaChn, s32Ret);
        return s32Ret;
    }

    /* Start Receive Imaging */
    s32Ret = HI_MPI_VDA_StartRecvPic(VdaChn);
    if(s32Ret != HI_SUCCESS)
    {
        g_critical("HI_MPI_VDA_StartRecvPic(%d) failed: 0x%08x\n", VdaChn, s32Ret);
        return s32Ret;
    }

    priv->thread = g_thread_new("video_detect", ipcam_video_detect_thread_handler, self);

    return s32Ret;
}

gint32 ipcam_video_detect_stop(IpcamVideoDetect *self)
{
    HI_S32 s32Ret = HI_SUCCESS;
    VDA_CHN VdaChn;
    MPP_CHN_S stSrcChn;
    MPP_CHN_S stDestChn;

    g_return_val_if_fail(IPCAM_IS_VIDEO_DETECT(self), HI_FAILURE);

    VdaChn = 0;

    /*Stop Receive Imaging */
    s32Ret = HI_MPI_VDA_StopRecvPic(VdaChn);
    if(s32Ret != HI_SUCCESS)
    {
        g_critical("HI_MPI_VDA_StopRecvPic(%d) failed: 0x%08x\n", VdaChn, s32Ret);
    }
    /* Unbind VI */
    stSrcChn.enModId = HI_ID_VIU;
    stSrcChn.s32DevId = 0;
    stSrcChn.s32ChnId = 1;
    stDestChn.enModId = HI_ID_VDA;
    stDestChn.s32DevId = 0;
    stDestChn.s32ChnId = VdaChn;
    s32Ret = HI_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
    if(s32Ret != HI_SUCCESS)
    {
        g_critical("HI_MPI_SYS_UnBind(%d) failed: 0x%08x\n", VdaChn, s32Ret);
    }
    /* Destroy Channel */
    s32Ret = HI_MPI_VDA_DestroyChn(VdaChn);
    if(s32Ret != HI_SUCCESS)
    {
        g_critical("HI_MPI_VDA_DestroyChn(%d) failed: 0x%08x\n", VdaChn, s32Ret);
    }

    return HI_SUCCESS;
}

static void
ipcam_video_detect_send_notify(IpcamVideoDetect *detect, guint region, gboolean occ)
{
}

static gpointer ipcam_video_detect_thread_handler(gpointer data)
{
    IpcamVideoDetect *self = IPCAM_VIDEO_DETECT(data);
    IpcamVideoDetectPrivate *priv = ipcam_video_detect_get_instance_private(self);
    IpcamIMedia *imedia;
    HI_S32 s32Ret = HI_SUCCESS;
    VDA_CHN VdaChn = 0;
    HI_S32 i;

    sleep(2);

    g_object_get(self, "app", &imedia, NULL);

    HI_S32 s32VdaFd;
    fd_set read_fds;

    s32VdaFd = HI_MPI_VDA_GetFd(VdaChn);

    while(!priv->terminated) {
        VDA_DATA_S stVdaData;

        FD_ZERO(&read_fds);
        FD_SET(s32VdaFd, &read_fds);

        s32Ret = select(s32VdaFd + 1, &read_fds, NULL, NULL, NULL);
        if (s32Ret < 0) {
            g_critical("select err\n");
            continue;
        }
        else if (s32Ret == 0) {
            g_warning("timeout\n");
            continue;
        }

        s32Ret = HI_MPI_VDA_GetData(VdaChn, &stVdaData, TRUE);
        if (s32Ret != HI_SUCCESS)
        {
            g_critical("HI_MPI_VDA_GetData(%d) failed: 0x%08x\n", VdaChn, s32Ret);
            continue;
        }

        for (i = 0; i < stVdaData.unData.stOdData.u32RgnNum; i++) {
            if (stVdaData.unData.stOdData.abRgnAlarm[i] == HI_TRUE) {
                g_print("VdaChn-%d, Rgn-%d, Occ!\n", VdaChn, i);
                s32Ret = HI_MPI_VDA_ResetOdRegion(VdaChn, i);
                if (s32Ret != HI_SUCCESS) {
                    g_print("HI_MPI_VDA_ResetOdRegion failed with %#x!\n", s32Ret);
                    return NULL;
                }
            }
        }

        s32Ret = HI_MPI_VDA_ReleaseData(VdaChn, &stVdaData);
        if (s32Ret != HI_SUCCESS) {
            g_critical("HI_MPI_VDA_ReleaseData failed with %#x!\n", s32Ret);
            return NULL;
        }

        usleep(200*1000);
    }

    return NULL;
}

void ipcam_video_detect_param_change(IpcamVideoDetect *self, StreamDescriptor desc[])
{
    g_return_if_fail(IPCAM_IS_VIDEO_DETECT(self));

    ipcam_video_detect_stop(self);
    ipcam_video_detect_start(self, desc);
}
