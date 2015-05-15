#include <json-glib/json-glib.h>
#include "imedia.h"
#include "video_param_change_handler.h"
#include "messages.h"


G_DEFINE_TYPE(IpcamVideoParamChangeHandler, ipcam_video_param_change_handler, IPCAM_EVENT_HANDLER_TYPE);

static void ipcam_video_param_change_handler_run_impl(IpcamEventHandler *event_handler,
                                                      IpcamMessage *message);

static void ipcam_video_param_change_handler_init(IpcamVideoParamChangeHandler *self)
{
}

static void ipcam_video_param_change_handler_class_init(IpcamVideoParamChangeHandlerClass *klass)
{
    IpcamEventHandlerClass *event_handler_class = IPCAM_EVENT_HANDLER_CLASS(klass);
    event_handler_class->run = &ipcam_video_param_change_handler_run_impl;
}

static void ipcam_video_param_change_handler_run_impl(IpcamEventHandler *event_handler,
                                                      IpcamMessage *message)
{
    IpcamVideoParamChangeHandler *video_param_change_handler = IPCAM_VIDEO_PARAM_CHANGE_HANDLER(event_handler);
    gpointer service;
	const char *event;
    JsonNode *body;

    g_object_get(G_OBJECT(video_param_change_handler), "service", &service, NULL);
    g_object_get(G_OBJECT(message), "event", &event, "body", &body, NULL);

	/* Debug */
#if 0
	{
		JsonGenerator *generator = json_generator_new();
		json_generator_set_root(generator, body);
		json_generator_set_pretty (generator, TRUE);
		gchar *str = json_generator_to_data (generator, NULL);
		g_printf("receive notice, body=\n%s\n", str);
		g_free(str);
		g_object_unref(generator);
	}
#endif

	if (g_str_equal (event, "set_base_info")) {
		ipcam_imedia_got_baseinfo_parameter(IPCAM_IMEDIA(service), body);
	}
	else if (g_str_equal (event, "set_misc")) {
		ipcam_imedia_got_misc_parameter(IPCAM_IMEDIA(service), body);
	}
	else if (g_str_equal (event, "add_users")) {
		ipcam_imedia_got_set_users_parameter(IPCAM_IMEDIA(service), body);
	}
	else if (g_str_equal (event, "set_users")) {
		ipcam_imedia_got_set_users_parameter(IPCAM_IMEDIA(service), body);
	}
	else if (g_str_equal (event, "del_users")) {
		ipcam_imedia_got_del_users_parameter(IPCAM_IMEDIA(service), body);
	}
	else if (g_str_equal (event, "set_event_cover")) {
		ipcam_imedia_got_od_param(IPCAM_IMEDIA(service), body, TRUE);
	}
	else if (g_str_equal (event, "set_video")) {
		ipcam_imedia_got_video_param(IPCAM_IMEDIA(service), body, TRUE);
	}
    else if (g_str_equal (event, "set_image")) {
        ipcam_imedia_got_image_parameter(IPCAM_IMEDIA(service), body);
    }
	else if (g_str_equal (event, "set_osd")) {
		ipcam_imedia_got_osd_parameter(IPCAM_IMEDIA(service), body);
	}
    else if (g_str_equal (event, "set_szyc")) {
        ipcam_imedia_got_szyc_parameter(IPCAM_IMEDIA(service), body);
    }
	else if (g_str_equal (event, "set_day_night_mode")) {
		ipcam_imedia_got_day_night_mode_parameter(IPCAM_IMEDIA(service), body);
	}
	else {
		g_warning ("unhandled event \"%s\"\n", event);
	}
}
