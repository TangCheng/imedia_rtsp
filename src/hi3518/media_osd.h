#ifndef __MEDIA_OSD_H__
#define __MEDIA_OSD_H__

#include <SDL2/SDL.h>
#include <glib.h>

struct _IpcamOSDItem;
typedef struct _IpcamOSDItem IpcamOSDItem;

struct _IpcamOSDStream;
typedef struct _IpcamOSDStream IpcamOSDStream;

struct _IpcamOSD;
typedef struct _IpcamOSD IpcamOSD;

/* IpcamOSDItem member functions */
void            ipcam_osd_item_enable(IpcamOSDItem *item);
void            ipcam_osd_item_disable(IpcamOSDItem *item);
void            ipcam_osd_item_set_text(IpcamOSDItem *item, char const *text);
void            ipcam_osd_item_set_layer(IpcamOSDItem *item, int layer);
void            ipcam_osd_item_set_font_size(IpcamOSDItem *item, int font_size);
void            ipcam_osd_item_set_fgcolor(IpcamOSDItem *item, SDL_Color *color);
void            ipcam_osd_item_set_bgcolor(IpcamOSDItem *item, SDL_Color *color);
void            ipcam_osd_item_set_position(IpcamOSDItem *item, int left, int top);
void            ipcam_osd_item_set_effect(IpcamOSDItem *item, HI_BOOL invert_color,
										  HI_U32 bg_alpha, HI_U32 fg_alpha);
void            ipcam_osd_item_draw_text(IpcamOSDItem *item);

/* IpcamOSDStream member functions */
IpcamOSDItem*   ipcam_osd_stream_add_item(IpcamOSDStream *stream, char const *name);
void            ipcam_osd_stream_delete_item(IpcamOSDStream *stream, IpcamOSDItem *item);
void            ipcam_osd_stream_set_image_size(IpcamOSDStream *stream, int width, int height);
IpcamOSDItem*   ipcam_osd_stream_lookup_item(IpcamOSDStream *stream, char const *name);

/* IpcamOSD member functions */
IpcamOSD*       ipcam_osd_new(char const *font_file);
void            ipcam_osd_destroy(IpcamOSD *osd);
IpcamOSDStream* ipcam_osd_add_stream(IpcamOSD *osd, VENC_GRP venc_grp);
void            ipcam_osd_delete_stream(IpcamOSD *osd, VENC_GRP venc_grp);
void            ipcam_osd_set_font_file(IpcamOSD *osd, char const *font_file);
IpcamOSDStream* ipcam_osd_lookup_stream(IpcamOSD *osd, VENC_GRP venc_grp);
IpcamOSDItem*   ipcam_osd_lookup_item(IpcamOSD *osd, VENC_GRP venc_grp, char const *name);

void            ipcam_osd_set_item_text(IpcamOSD *osd, VENC_GRP venc_grp, char const *name, char const *text);

#endif /* __MEDIA_OSD_H__ */
