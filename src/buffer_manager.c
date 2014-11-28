#include <stdlib.h>
#include "buffer_manager.h"
#include "stream_descriptor.h"

#define BUFFER_MAX_LENGTH  10

typedef struct _IpcamBufferManagerPrivate
{
    GQueue *clean_buffer_queue;
    GQueue *dirty_buffer_queue;
    GMutex clean_mutex;
    GMutex dirty_mutex;
} IpcamBufferManagerPrivate;

G_DEFINE_TYPE_WITH_PRIVATE(IpcamBufferManager, ipcam_buffer_manager, G_TYPE_OBJECT);

static void ipcam_buffer_manager_dispose(GObject *self)
{
    static gboolean first_run = TRUE;
    if (first_run)
    {
        first_run = FALSE;
        G_OBJECT_CLASS(ipcam_buffer_manager_parent_class)->dispose(self);
    }
}
static void ipcam_buffer_manager_finalize(GObject *self)
{
    IpcamBufferManagerPrivate *priv = ipcam_buffer_manager_get_instance_private(IPCAM_BUFFER_MANAGER(self));
    g_mutex_lock(&priv->clean_mutex);
    g_queue_free_full(priv->clean_buffer_queue, g_free);
    g_mutex_unlock(&priv->clean_mutex);
    g_mutex_lock(&priv->dirty_mutex);
    g_queue_free_full(priv->dirty_buffer_queue, g_free);
    g_mutex_unlock(&priv->dirty_mutex);
    g_mutex_clear(&priv->clean_mutex);
    g_mutex_clear(&priv->dirty_mutex);
    G_OBJECT_CLASS(ipcam_buffer_manager_parent_class)->finalize(self);
}
static void ipcam_buffer_manager_init(IpcamBufferManager *self)
{
    IpcamBufferManagerPrivate *priv = ipcam_buffer_manager_get_instance_private(self);
    gint i = 0;
    
    g_mutex_init(&priv->clean_mutex);
    g_mutex_init(&priv->dirty_mutex);
    priv->clean_buffer_queue = g_queue_new();
    priv->dirty_buffer_queue = g_queue_new();
    for (i = 0; i < BUFFER_MAX_LENGTH; i++)
    {
        gpointer data = malloc(1024 * 1024);
        g_queue_push_tail(priv->clean_buffer_queue, data);
    }
}
static void ipcam_buffer_manager_class_init(IpcamBufferManagerClass *klass)
{
    GObjectClass *this_class = G_OBJECT_CLASS(klass);
    this_class->dispose = &ipcam_buffer_manager_dispose;
    this_class->finalize = &ipcam_buffer_manager_finalize;
}

gpointer ipcam_buffer_manager_get_write_data(IpcamBufferManager *buffer_manager)
{
    g_return_if_fail(IPCAM_IS_BUFFER_MANAGER(buffer_manager));
    IpcamBufferManagerPrivate *priv = ipcam_buffer_manager_get_instance_private(buffer_manager);
    gpointer data = NULL;
    
    g_mutex_lock(&priv->clean_mutex);
    data = g_queue_pop_head(priv->clean_buffer_queue);
    g_mutex_unlock(&priv->clean_mutex);
    
    return data;
}

void ipcam_buffer_manager_release_write_data(IpcamBufferManager *buffer_manager, gpointer data)
{
    g_return_if_fail(IPCAM_IS_BUFFER_MANAGER(buffer_manager));
    g_return_if_fail(data);
    IpcamBufferManagerPrivate *priv = ipcam_buffer_manager_get_instance_private(buffer_manager);
    
    g_mutex_lock(&priv->dirty_mutex);
    g_queue_push_tail(priv->dirty_buffer_queue, data);
    g_mutex_unlock(&priv->dirty_mutex);
}

gpointer ipcam_buffer_manager_get_read_data(IpcamBufferManager *buffer_manager)
{
    g_return_val_if_fail(IPCAM_IS_BUFFER_MANAGER(buffer_manager), NULL);
    IpcamBufferManagerPrivate *priv = ipcam_buffer_manager_get_instance_private(buffer_manager);
    gpointer data = NULL;
    
    g_mutex_lock(&priv->dirty_mutex);
    data = g_queue_pop_head(priv->dirty_buffer_queue);
    g_mutex_unlock(&priv->dirty_mutex);

    return data;
}

void ipcam_buffer_manager_release_read_data(IpcamBufferManager *buffer_manager, gpointer data)
{
    g_return_if_fail(IPCAM_IS_BUFFER_MANAGER(buffer_manager));
    g_return_if_fail(data);
    IpcamBufferManagerPrivate *priv = ipcam_buffer_manager_get_instance_private(buffer_manager);
    
    g_mutex_lock(&priv->clean_mutex);
    g_queue_push_tail(priv->clean_buffer_queue, data);
    g_mutex_unlock(&priv->clean_mutex);
}

gboolean ipcam_buffer_manager_has_data(IpcamBufferManager *buffer_manager)
{
    g_return_val_if_fail(IPCAM_IS_BUFFER_MANAGER(buffer_manager), FALSE);
    IpcamBufferManagerPrivate *priv = ipcam_buffer_manager_get_instance_private(buffer_manager);
    gboolean ret = FALSE;

    g_mutex_lock(&priv->dirty_mutex);
    ret = g_queue_is_empty(priv->dirty_buffer_queue);
    g_mutex_unlock(&priv->dirty_mutex);

    return ret;
}
