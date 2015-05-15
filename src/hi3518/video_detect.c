#include <hi_defines.h>
#include <hi_comm_sys.h>
#include <hi_comm_vda.h>
#include <mpi_vda.h>
#include <mpi_sys.h>
#include <stdlib.h>
#include <notice_message.h>
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

    guint32 rgn_num;

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

    priv->rgn_num = 0;
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

gint32 ipcam_video_detect_start(IpcamVideoDetect *self, OD_REGION_INFO od_info[])
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

    stVdaChnAttr.unAttr.stOdAttr.enVdaAlg = VDA_ALG_REF;
    stVdaChnAttr.unAttr.stOdAttr.enMbSize = VDA_MB_8PIXEL;
    stVdaChnAttr.unAttr.stOdAttr.enMbSadBits = VDA_MB_SAD_8BIT;
    stVdaChnAttr.unAttr.stOdAttr.enRefMode = VDA_REF_MODE_DYNAMIC;
    stVdaChnAttr.unAttr.stOdAttr.u32VdaIntvl = 4;   /* fps= 20/(4+1) = 4 */
    stVdaChnAttr.unAttr.stOdAttr.u32BgUpSrcWgt = 128;

    int rgn_num = 0;
    for (i = 0; i < VDA_OD_RGN_NUM_MAX; i++) {
        VDA_OD_RGN_ATTR_S *rgn_attr;

        if (!od_info[i].enable)
            continue;

        rgn_attr = &stVdaChnAttr.unAttr.stOdAttr.astOdRgnAttr[rgn_num];

        rgn_attr->stRect.s32X = od_info[i].rect.left * 320 / 1000;
        rgn_attr->stRect.s32Y = od_info[i].rect.top * 240 / 1000;
        rgn_attr->stRect.u32Width = od_info[i].rect.width * 320 / 1000;
        rgn_attr->stRect.u32Height = od_info[i].rect.height * 240 / 1000;
        rgn_attr->u32SadTh = 512 * (100 - od_info[i].sensitivity) / 100;
        rgn_attr->u32AreaTh = 80; /* 80% */
        rgn_attr->u32OccCntTh = 8;
        rgn_attr->u32UnOccCntTh = 2;

        g_print("VDA: rgn%d rect={%d,%d,%d,%d} u32SadTh=%d\n",
                rgn_num,
                rgn_attr->stRect.s32X,
                rgn_attr->stRect.s32Y,
                rgn_attr->stRect.u32Width,
                rgn_attr->stRect.u32Height,
                rgn_attr->u32SadTh);

        rgn_num++;
    }
    stVdaChnAttr.unAttr.stOdAttr.u32RgnNum = rgn_num;
    priv->rgn_num = rgn_num;

    g_return_val_if_fail(priv->rgn_num > 0, HI_SUCCESS);

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

    priv->terminated = FALSE;
    priv->thread = g_thread_new("video_detect", ipcam_video_detect_thread_handler, self);

    return s32Ret;
}

gint32 ipcam_video_detect_stop(IpcamVideoDetect *self)
{
    IpcamVideoDetectPrivate *priv = ipcam_video_detect_get_instance_private(self);
    HI_S32 s32Ret = HI_SUCCESS;
    VDA_CHN VdaChn;
    MPP_CHN_S stSrcChn;
    MPP_CHN_S stDestChn;

    g_return_val_if_fail(IPCAM_IS_VIDEO_DETECT(self), HI_FAILURE);

    g_return_val_if_fail(priv->rgn_num > 0, HI_SUCCESS);

    VdaChn = 0;

    priv->terminated = TRUE;
    g_thread_join(priv->thread);

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
    IpcamIMedia *imedia;
    JsonBuilder *builder;
    IpcamMessage *notice_msg;
    JsonNode *body;

    g_object_get(detect, "app", &imedia, NULL);

    g_return_if_fail(imedia != NULL);

    builder = json_builder_new();
    json_builder_begin_object(builder);
    json_builder_set_member_name(builder, "event");
    json_builder_begin_object(builder);
    json_builder_set_member_name(builder, "region");
    json_builder_add_int_value(builder, region);
    json_builder_set_member_name(builder, "state");
    json_builder_add_boolean_value(builder, occ);
    json_builder_end_object(builder);
    json_builder_end_object(builder);

    body = json_builder_get_root(builder);

    notice_msg = g_object_new(IPCAM_NOTICE_MESSAGE_TYPE,
                              "event", "video_occlusion_event",
                              "body", body,
                              NULL);
    ipcam_base_app_broadcast_notice_message(IPCAM_BASE_APP(imedia), notice_msg, "imedia_rtsp_token");

    g_object_unref(notice_msg);
    g_object_unref(builder);
}

struct VdaRegionState
{
	gboolean  occ_state;
	guint32   count;
};

static struct VdaRegionState __rgn_stat[VDA_OD_RGN_NUM_MAX];

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

	for (i = 0; i < ARRAY_SIZE(__rgn_stat); i++) {
		__rgn_stat[i].occ_state = FALSE;
		__rgn_stat[i].count = 0;
	}

    while(!priv->terminated) {
        VDA_DATA_S stVdaData;

        s32Ret = HI_MPI_VDA_GetData(VdaChn, &stVdaData, TRUE);
        if (s32Ret != HI_SUCCESS)
        {
            g_critical("HI_MPI_VDA_GetData(%d) failed: 0x%08x\n", VdaChn, s32Ret);
            continue;
        }

        for (i = 0; i < stVdaData.unData.stOdData.u32RgnNum; i++) {
            guint region = i;
            gboolean occ = (stVdaData.unData.stOdData.abRgnAlarm[i] == HI_TRUE);
            if (occ) {
				if (!__rgn_stat[region].occ_state) {
					ipcam_video_detect_send_notify (self, region, occ);
				}

				__rgn_stat[region].occ_state = TRUE;
				__rgn_stat[region].count = 0;

                s32Ret = HI_MPI_VDA_ResetOdRegion(VdaChn, i);
                if (s32Ret != HI_SUCCESS) {
                    g_print("HI_MPI_VDA_ResetOdRegion failed with %#x!\n", s32Ret);
                    return NULL;
                }
            }
			else {
				if (__rgn_stat[region].occ_state) {
					if (__rgn_stat[region].count > 12) {
						ipcam_video_detect_send_notify(self, region, occ);
						__rgn_stat[region].occ_state = FALSE;
						__rgn_stat[region].count = 0;
					}
					else {
						__rgn_stat[region].count++;
					}
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

void ipcam_video_detect_param_change(IpcamVideoDetect *self, OD_REGION_INFO od_info[])
{
    g_return_if_fail(IPCAM_IS_VIDEO_DETECT(self));

    ipcam_video_detect_stop(self);
    ipcam_video_detect_start(self, od_info);
}
