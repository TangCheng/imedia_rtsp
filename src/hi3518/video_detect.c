#include <hi_defines.h>
#include <hi_comm_sys.h>
#include <hi_comm_isp.h>
#include <mpi_sys.h>
#include <mpi_isp.h>
#include <stdlib.h>
#include <notice_message.h>
#include "imedia.h"
#include "video_detect.h"

enum
{
    PROP_0,
    PROP_APP,
    N_PROPERTIES
};

typedef struct _IpcamVideoDetectPrivate
{
    IpcamIMedia *imedia;

    GThread *thread;
    gboolean terminated;

	gboolean occ_state;
} IpcamVideoDetectPrivate;

G_DEFINE_TYPE_WITH_PRIVATE(IpcamVideoDetect, ipcam_video_detect, G_TYPE_OBJECT)

static GParamSpec *obj_properties[N_PROPERTIES] = {NULL, };

static gpointer ipcam_video_detect_thread_handler(gpointer data);


static void ipcam_video_detect_init(IpcamVideoDetect *self)
{
	IpcamVideoDetectPrivate *priv = ipcam_video_detect_get_instance_private(self);

    priv->imedia = NULL;
    priv->thread = NULL;
    priv->terminated = FALSE;
	priv->occ_state = FALSE;
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

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}


gint32 ipcam_video_detect_start(IpcamVideoDetect *self, OD_REGION_INFO od_info[])
{
    IpcamVideoDetectPrivate *priv = ipcam_video_detect_get_instance_private(self);

    g_return_val_if_fail(IPCAM_IS_VIDEO_DETECT(self), HI_FAILURE);

    priv->terminated = FALSE;
    priv->thread = g_thread_new("video_detect", ipcam_video_detect_thread_handler, self);

    return 0;
}

gint32 ipcam_video_detect_stop(IpcamVideoDetect *self)
{
    IpcamVideoDetectPrivate *priv = ipcam_video_detect_get_instance_private(self);

    g_return_val_if_fail(IPCAM_IS_VIDEO_DETECT(self), HI_FAILURE);

    priv->terminated = TRUE;
    g_thread_join(priv->thread);

	return HI_SUCCESS;
}

static void
ipcam_video_detect_send_notify(IpcamVideoDetect *detect, guint region, gboolean occ)
{
    IpcamIMedia *imedia;
    JsonBuilder *builder;
    IpcamMessage *notice_msg;
    JsonNode *body;

	//g_print("VDA: region[%d]=%d\n", region, occ);

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
    ipcam_base_app_send_message(IPCAM_BASE_APP(imedia),
                                notice_msg,
                                "imedia_rtsp_pub",
                                "imedia_rtsp_token",
                                NULL,
                                0);

    g_object_unref(notice_msg);
    g_object_unref(builder);
}

static gpointer ipcam_video_detect_thread_handler(gpointer data)
{
    IpcamVideoDetect *self = IPCAM_VIDEO_DETECT(data);
    IpcamVideoDetectPrivate *priv = ipcam_video_detect_get_instance_private(self);
    IpcamIMedia *imedia;
    HI_S32 s32Ret = HI_SUCCESS;
	int count = 0;

    g_object_get(self, "app", &imedia, NULL);

	sleep(1);

    while(!priv->terminated) {
		ISP_EXP_STA_INFO_S stExpSta;
		s32Ret = HI_MPI_ISP_GetExpStaInfo(&stExpSta);
		if (s32Ret == HI_SUCCESS) {
#if 0
			int i;
			printf("u16Exp_Hist5Value={");
			for (i = 0; i < ARRAY_SIZE(stExpSta.u16Exp_Hist5Value); i++) {
				printf("%s%d", i > 0 ? " " : "",
				       stExpSta.u16Exp_Hist5Value[i]);
			}
			printf("}\n");
#endif
			if (stExpSta.u16Exp_Hist5Value[4] > 1400 && priv->occ_state) {
				count++;
				if (count > 4) {
					priv->occ_state = FALSE;
					count = 0;
					ipcam_video_detect_send_notify(self, 0, priv->occ_state);
					g_print("Occlusion removed\n");
				}
			}
			else if (stExpSta.u16Exp_Hist5Value[4] < 1000 && !priv->occ_state) {
				count++;
				if (count > 4) {
					priv->occ_state = TRUE;
					count = 0;
					ipcam_video_detect_send_notify(self, 0, priv->occ_state);
					g_print("Occlusion detected\n");
				}
			}
			else {
				count = 0;
			}
		}

        usleep(100*1000);
    }

    return NULL;
}

void ipcam_video_detect_param_change(IpcamVideoDetect *self, OD_REGION_INFO od_info[])
{
    g_return_if_fail(IPCAM_IS_VIDEO_DETECT(self));

    ipcam_video_detect_stop(self);
    ipcam_video_detect_start(self, od_info);
}
