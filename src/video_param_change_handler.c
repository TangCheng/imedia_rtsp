#include <json-glib/json-glib.h>
#include "imedia.h"
#include "video_param_change_handler.h"
#include "messages.h"


G_DEFINE_TYPE(IpcamVideoParamChangeHandler, ipcam_video_param_change_handler, IPCAM_EVENT_HANDLER_TYPE);

static void ipcam_video_param_change_handler_run_impl(IpcamVideoParamChangeHandler *video_param_change_handler,
                                                      IpcamMessage *message);

static void ipcam_video_param_change_handler_init(IpcamVideoParamChangeHandler *self)
{
}
static void ipcam_video_param_change_handler_class_init(IpcamVideoParamChangeHandlerClass *klass)
{
    IpcamEventHandlerClass *event_handler_class = IPCAM_EVENT_HANDLER_CLASS(klass);
    event_handler_class->run = &ipcam_video_param_change_handler_run_impl;
}
static void ipcam_video_param_change_handler_run_impl(IpcamVideoParamChangeHandler *video_param_change_handler,
                                                      IpcamMessage *message)
{
    gpointer service;
    g_object_get(G_OBJECT(video_param_change_handler), "service", &service, NULL);
    JsonNode *body;
    JsonGenerator *generator = json_generator_new();

    g_object_get(G_OBJECT(message), "body", &body, NULL);

    json_generator_set_root(generator, body);
    json_generator_set_pretty (generator, TRUE);
    gchar *str = json_generator_to_data (generator, NULL);
    
    g_printf("receive notice, body=\n%s\n", str);

    ipcam_imedia_got_video_param(IPCAM_IMEDIA(service), body, TRUE);

    g_free(str);
    g_object_unref(generator);
}
