#include <hi_type.h>
#include <hi_defines.h>
#include <hi_comm_region.h>
#include <mpi_region.h>
#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "stream_descriptor.h"
#include "media_osd.h"

enum
{
    PROP_0,
    PROP_RGN_HANDLE,
    PROP_VENC_GROUP,
	PROP_FONT_FILE,
    N_PROPERTIES
};

struct _IpcamOSDItem
{
	IpcamOSD*       osd;
	IpcamOSDStream* stream;
	char*           name;
	RGN_HANDLE      rgn_handle;
	HI_U32          layer;
	gboolean        enabled;

	/* Effect */
	HI_BOOL         invert_color;
	HI_U32          bg_alpha;
	HI_U32          fg_alpha;

	char*           text;
	int	            font_size;
	SDL_Color       fg_color;
	SDL_Color       bg_color;
	POINT_S         position;
	POINT_S         pos_coords;
	SIZE_S          size;

	BITMAP_S        bitmap;
	SDL_Surface*    surface;
};


struct _IpcamOSDStream
{
	IpcamOSD*       osd;
	VENC_GRP        venc_grp;
	uint32_t        image_width;
	uint32_t        image_height;
	GHashTable*     osd_items;         /* [RGN_HANDLE] => [IpcamOSDItem*] */
};

struct _IpcamOSD
{
	TTF_Font*       ttf_font;
	GQueue          rgn_handle_pool;   /* for allocating region handle */
	GHashTable*     streams;           /* [VENC_GRP] => [IpcamOSDStream*] */
};

static gboolean ipcam_osd_alloc_region(IpcamOSD *osd, RGN_HANDLE *rgn_handle);
static void     ipcam_osd_release_region(IpcamOSD *osd, RGN_HANDLE rgn_handle);

static IpcamOSDItem*
ipcam_osd_item_new(IpcamOSDStream *stream, char const *name, RGN_HANDLE handle)
{
	IpcamOSDItem *item;

	item = calloc(1, sizeof(IpcamOSDItem));
	if (item == NULL)
		return NULL;

	item->name = strdup(name);
	item->rgn_handle = handle;
	item->stream = stream;
	item->osd = stream->osd;
	item->size.u32Width = 16;
	item->size.u32Height = 16;
	item->invert_color = HI_TRUE;
	item->bg_alpha = 0;
	item->fg_alpha = 128;
	item->bg_color.r = 0x80;
	item->bg_color.g = 0x80;
	item->bg_color.b = 0x80;
	item->bg_color.a = 0x00;
	item->surface = NULL;

	return item;
}

static void
ipcam_osd_item_destroy(IpcamOSDItem *item)
{
	if (item->enabled)
		ipcam_osd_item_disable(item);
	free((void*)item->name);
	free((void*)item->text);
	free(item);
}

static inline HI_S32
ipcam_osd_item_round_position(IpcamOSDItem *item, HI_S32 value)
{
	return item->invert_color ? ((value + 8) & ~0x0F) : ((value + 2) & ~0x3);
}

static inline HI_U32
ipcam_osd_item_round_size(IpcamOSDItem *item, HI_U32 value)
{
	return item->invert_color ? ((value + 15) & ~0x0F) : ((value + 1) & ~0x1);
}

gboolean
ipcam_osd_item_is_enabled(IpcamOSDItem *item)
{
	return item->enabled;
}

void
ipcam_osd_item_enable(IpcamOSDItem *item)
{
    HI_S32 s32Ret = HI_FAILURE;
    MPP_CHN_S stChn;
    RGN_ATTR_S stRgnAttr;
    RGN_CHN_ATTR_S stChnAttr;
	VENC_GRP venc_grp = item->stream->venc_grp;
	HI_U32 bg_color;

	/* Check if already enabled */
	if (item->enabled)
		return;

	/* Prepare ARGB1555 surface */
	item->surface = SDL_CreateRGBSurface(SDL_SWSURFACE,
										 item->size.u32Width,	/* width */
										 item->size.u32Height,  /* height */
										 16,					/* depth */
										 0x1F << 10,			/* Rmask */
										 0x1F << 5,				/* Gmask */
										 0x1F << 0,				/* Bmask */
										 0x01 << 15);			/* Amask */
	item->bitmap.enPixelFormat = PIXEL_FORMAT_RGB_1555;
	item->bitmap.u32Width = item->size.u32Width;
	item->bitmap.u32Height = item->size.u32Height;
	item->bitmap.pData = item->surface->pixels;

	/* Create region and attach to channel */
	stRgnAttr.enType = OVERLAY_RGN;
	stRgnAttr.unAttr.stOverlay.enPixelFmt = PIXEL_FORMAT_RGB_1555;
	stRgnAttr.unAttr.stOverlay.stSize.u32Width  = item->size.u32Width;
	stRgnAttr.unAttr.stOverlay.stSize.u32Height = item->size.u32Height;
	bg_color = SDL_MapRGBA(item->surface->format,
						   item->bg_color.r,
						   item->bg_color.g,
						   item->bg_color.b,
						   item->bg_color.a);
	stRgnAttr.unAttr.stOverlay.u32BgColor = bg_color;

	s32Ret = HI_MPI_RGN_Create(item->rgn_handle, &stRgnAttr);
	if (HI_SUCCESS != s32Ret)
	{
		fprintf(stderr, "Failed to create region: [%#x]\n", s32Ret);
		return;
	}

	stChn.enModId = HI_ID_GROUP;
	stChn.s32DevId = venc_grp;
	stChn.s32ChnId = 0;

	memset(&stChnAttr, 0, sizeof(stChnAttr));
	stChnAttr.bShow = HI_TRUE;
	stChnAttr.enType = OVERLAY_RGN;
	stChnAttr.unChnAttr.stOverlayChn.stPoint.s32X = item->pos_coords.s32X;
	stChnAttr.unChnAttr.stOverlayChn.stPoint.s32Y = item->pos_coords.s32Y;
	stChnAttr.unChnAttr.stOverlayChn.u32BgAlpha = item->bg_alpha;
	stChnAttr.unChnAttr.stOverlayChn.u32FgAlpha = item->fg_alpha;
	stChnAttr.unChnAttr.stOverlayChn.u32Layer = MIN(item->layer, 7);

	stChnAttr.unChnAttr.stOverlayChn.stQpInfo.bAbsQp = HI_FALSE;
	stChnAttr.unChnAttr.stOverlayChn.stQpInfo.s32Qp  = 0;

	stChnAttr.unChnAttr.stOverlayChn.stInvertColor.stInvColArea.u32Width = 16;
	stChnAttr.unChnAttr.stOverlayChn.stInvertColor.stInvColArea.u32Height = 16;
	stChnAttr.unChnAttr.stOverlayChn.stInvertColor.u32LumThresh = 128;
	stChnAttr.unChnAttr.stOverlayChn.stInvertColor.enChgMod = LESSTHAN_LUM_THRESH;
	stChnAttr.unChnAttr.stOverlayChn.stInvertColor.bInvColEn = item->invert_color;

	s32Ret = HI_MPI_RGN_AttachToChn(item->rgn_handle, &stChn, &stChnAttr);
	if (HI_SUCCESS != s32Ret)
	{
		fprintf(stderr, "Failed to attach region%d to channel [%#x]\n", item->rgn_handle, s32Ret);
		HI_MPI_RGN_Destroy(item->rgn_handle);

		return;
	}

	item->enabled = TRUE;
}

void
ipcam_osd_item_disable(IpcamOSDItem *item)
{
	HI_S32 s32Ret = HI_FAILURE;
    MPP_CHN_S stChn;
	VENC_GRP venc_grp = item->stream->venc_grp;

    stChn.enModId = HI_ID_GROUP;
    stChn.s32DevId = venc_grp;
    stChn.s32ChnId = 0;

	if (!item->enabled)
		return;

    s32Ret = HI_MPI_RGN_DetachFrmChn(item->rgn_handle, &stChn);
    if(HI_SUCCESS != s32Ret)
    {
        fprintf(stderr, "Failed to detach region%d from channel%d [%#x]\n",
				item->rgn_handle, venc_grp, s32Ret);
    }
    s32Ret = HI_MPI_RGN_Destroy(item->rgn_handle);
    if (HI_SUCCESS != s32Ret)
    {
        fprintf(stderr, "Failed to destroy region%d [%#x]\n",
				item->rgn_handle, s32Ret);
    }

	item->bitmap.pData = NULL;
	SDL_FreeSurface(item->surface);
	item->surface = NULL;

	item->enabled = FALSE;
}

void
ipcam_osd_item_draw_text(IpcamOSDItem *item)
{
	IpcamOSD *osd = item->osd;
	HI_S32 s32Ret = HI_FAILURE;
	SDL_Surface *text_surface;

	/* Check if already disabled */
	if (!item->enabled || !item->text)
		return;

	TTF_SetFontSize(osd->ttf_font, item->font_size);
	text_surface = TTF_RenderUTF8_Solid(osd->ttf_font, item->text, item->fg_color);
	if (text_surface) {
		HI_BOOL grow_size = HI_FALSE;
		HI_U32 bgcolor = SDL_MapRGBA(item->surface->format,
									 item->bg_color.r,
									 item->bg_color.g,
									 item->bg_color.b,
									 item->bg_color.a);

		/*
		 * If the text surface size is greater than item's size,
		 * we need to grow the osd item's size.
		 */
		if (text_surface->w > item->size.u32Width) {
			item->size.u32Width = ipcam_osd_item_round_size(item, text_surface->w);
			grow_size = TRUE;
		}
		if (text_surface->h > item->size.u32Height) {
			item->size.u32Height = ipcam_osd_item_round_size(item, text_surface->h);
			grow_size = TRUE;
		}
		/* size grow needed, re-enable the item to take effect */
		if (grow_size) {
			ipcam_osd_item_disable(item);
			ipcam_osd_item_set_position(item, item->position.s32X, item->position.s32Y);
			ipcam_osd_item_enable(item);
		}

		SDL_Rect src_rect = {
			.x = 0, .y = 0, .w = text_surface->w, .h = text_surface->h };
		SDL_Rect dst_rect = {
			.x = 0, .y = 0, .w = item->surface->w, .h = item->surface->h };

		SDL_FillRect(item->surface, &dst_rect, bgcolor);
		SDL_BlitSurface(text_surface, &src_rect,
						item->surface, &dst_rect);

		SDL_FreeSurface(text_surface);

		s32Ret = HI_MPI_RGN_SetBitMap(item->rgn_handle, &item->bitmap);
		if(s32Ret != HI_SUCCESS)
		{
			fprintf(stderr, "Failed to set bitmap for osd [%#x]\n", s32Ret);
		}
	}
}

void
ipcam_osd_item_set_text(IpcamOSDItem *item, char const *text)
{
	if (item->text && strcmp(text, item->text) == 0)
		return;

	free(item->text);
	item->text = strdup(text);

	/* Re-draw item */
	ipcam_osd_item_draw_text(item);
}

void
ipcam_osd_item_set_layer(IpcamOSDItem *item, int layer)
{
	item->layer = layer;

	/* Re-enable the item */
	if (item->enabled) {
		ipcam_osd_item_disable(item);
		ipcam_osd_item_enable(item);
		ipcam_osd_item_draw_text(item);
	}
}

void
ipcam_osd_item_set_effect(IpcamOSDItem *item, HI_BOOL invert_color,
						  HI_U32 bg_alpha, HI_U32 fg_alpha)
{
	if ((item->invert_color == invert_color)
		&& (item->bg_alpha == bg_alpha)
		&& (item->fg_alpha == fg_alpha))
		return;

	item->invert_color = !!invert_color;
	item->bg_alpha = bg_alpha;
	item->fg_alpha = fg_alpha;

	/* Re-enable the item to take effect */
	if (item->enabled) {
		ipcam_osd_item_disable(item);
		ipcam_osd_item_enable(item);
	}
}

void
ipcam_osd_item_set_font_size(IpcamOSDItem *item, int font_size)
{
	if (font_size == item->font_size)
		return;

	item->font_size = font_size;

	if (item->enabled && item->text) {
		/* Re-draw item */
		ipcam_osd_item_draw_text(item);
	}
}

void
ipcam_osd_item_set_fgcolor(IpcamOSDItem *item, SDL_Color *color)
{
	item->fg_color = *color;

	if (item->enabled && item->text) {
		ipcam_osd_item_draw_text(item);
	}
}

void
ipcam_osd_item_set_bgcolor(IpcamOSDItem *item, SDL_Color *color)
{
	item->bg_color = *color;

	if (item->enabled) {
		RGN_ATTR_S stRgnAttr;
		HI_S32 s32Ret = HI_SUCCESS;

		s32Ret = HI_MPI_RGN_GetAttr(item->rgn_handle, &stRgnAttr);
		if (s32Ret == HI_SUCCESS) {
			HI_U32 bgcolor = SDL_MapRGBA(item->surface->format,
										 color->r, color->g, color->b, color->a);
			stRgnAttr.unAttr.stOverlay.u32BgColor = bgcolor;
			s32Ret = HI_MPI_RGN_SetAttr(item->rgn_handle, &stRgnAttr);
		}
	}
}

void
ipcam_osd_item_set_position(IpcamOSDItem *item, int x, int y)
{
	uint32_t image_width = item->stream->image_width;
	uint32_t image_height = item->stream->image_height;

	item->position.s32X = x;
	item->position.s32Y = y;

	x = (x * image_width) / 1000;
	y = (y * image_height) / 1000;

	if (x + item->size.u32Width > image_width)
		x = image_width - item->size.u32Width;
	if (y + item->size.u32Height > image_height)
		y = image_height - item->size.u32Height;

	item->pos_coords.s32X = ipcam_osd_item_round_position(item, x);
	item->pos_coords.s32Y = ipcam_osd_item_round_position(item, y);

	if (item->enabled) {
		HI_S32 s32Ret;
		RGN_CHN_ATTR_S stChnAttr;
		MPP_CHN_S stChn;

		stChn.enModId = HI_ID_GROUP;
		stChn.s32DevId = item->stream->venc_grp;
		stChn.s32ChnId = 0;

		s32Ret = HI_MPI_RGN_GetDisplayAttr(item->rgn_handle, &stChn, &stChnAttr);
		if (s32Ret != HI_SUCCESS) {
			fprintf(stderr, "HI_MPI_RGN_GetDisplayAttr() failed [%#x]\n", s32Ret);
			return;
		}
		stChnAttr.unChnAttr.stOverlayChn.stPoint.s32X = item->pos_coords.s32X;
		stChnAttr.unChnAttr.stOverlayChn.stPoint.s32Y = item->pos_coords.s32Y;
		s32Ret = HI_MPI_RGN_SetDisplayAttr(item->rgn_handle, &stChn, &stChnAttr);
		if (s32Ret != HI_SUCCESS) {
			fprintf(stderr, "HI_MPI_RGN_SetDisplayAttr() failed [%#x]\n", s32Ret);
			return;
		}
	}
}


/* IpcamOSDStream member functions */

static void
ipcam_osd_stream_free_items(gpointer data)
{
	IpcamOSDItem *item = data;

	ipcam_osd_item_destroy(item);
}

IpcamOSDStream*
ipcam_osd_stream_new(IpcamOSD *osd, VENC_GRP venc_grp)
{
	IpcamOSDStream *stream;

	stream = calloc(1, sizeof(IpcamOSDStream));
	if (stream == NULL)
		return NULL;

	stream->osd = osd;
	stream->venc_grp = venc_grp;
	stream->osd_items = g_hash_table_new_full(g_str_hash,
											  g_str_equal,
											  NULL,
											  ipcam_osd_stream_free_items);

	return stream;
}

void
ipcam_osd_stream_destroy(IpcamOSDStream *stream)
{
	g_hash_table_unref(stream->osd_items);
	free(stream);
}

IpcamOSDItem*
ipcam_osd_stream_add_item(IpcamOSDStream *stream, char const *name)
{
	IpcamOSDItem *item = NULL;
	RGN_HANDLE rgn_handle;
	guint tb_size;

	tb_size = g_hash_table_size(stream->osd_items);
	item = g_hash_table_lookup(stream->osd_items, name);
	if (item == NULL) {
		if (ipcam_osd_alloc_region(stream->osd, &rgn_handle)) {
			item = ipcam_osd_item_new(stream, name, rgn_handle);
			if (item) {
				ipcam_osd_item_set_layer(item, tb_size);
				g_hash_table_replace(stream->osd_items, item->name, item);
			}
		}
	}

	return item;
}

void
ipcam_osd_stream_delete_item(IpcamOSDStream *stream, IpcamOSDItem *item)
{
	RGN_HANDLE rgn_handle;

	rgn_handle = item->rgn_handle;
	g_hash_table_remove(stream->osd_items, item->name);
	ipcam_osd_item_destroy(item);
	ipcam_osd_release_region(stream->osd, rgn_handle);
}

void
ipcam_osd_stream_set_image_size(IpcamOSDStream *stream, int width, int height)
{
	GHashTableIter iter;
	IpcamOSDItem *item;

	stream->image_width = width;
	stream->image_height = height;

	g_hash_table_iter_init(&iter, stream->osd_items);
	while (g_hash_table_iter_next(&iter, NULL, (gpointer *)&item)) {
		ipcam_osd_item_set_position(item,
									item->position.s32X,
									item->position.s32Y);
	}
}

IpcamOSDItem*
ipcam_osd_stream_lookup_item(IpcamOSDStream *stream, char const *name)
{
	return g_hash_table_lookup(stream->osd_items, name);
}


/* IpcamOSD member functions */

static void free_video_stream_callback(gpointer data)
{
	IpcamOSDStream *stream = (IpcamOSDStream*)data;

	g_hash_table_unref(stream->osd_items);
	free(stream);
}

IpcamOSD *ipcam_osd_new(char const* font_file)
{
	IpcamOSD *osd;
	int i;

	osd = calloc(1, sizeof(IpcamOSD));
	if (osd == NULL)
		return NULL;

	if (TTF_Init() < 0) {
		fprintf(stderr, "Couldn't initialize TTF: %s\n", SDL_GetError());
		free(osd);
		return NULL;
	}

	if (font_file) {
		fprintf(stderr, "loading font %s\n", font_file);
		osd->ttf_font = TTF_OpenFont(font_file, 20);
	}

	/* Use fallback */
	if (!osd->ttf_font) {
		font_file = "/usr/share/fonts/truetype/droid/DroidSansFallback.ttf";
		osd->ttf_font = TTF_OpenFont(font_file, 20);
	}

	if (!osd->ttf_font) {
		fprintf(stderr, "Failed to load font %s [%s]\n", font_file, SDL_GetError());
		free(osd);
		return NULL;
	}

	/* Initialize region handle pool */
	g_queue_init(&osd->rgn_handle_pool);
	for (i = 0; i < RGN_HANDLE_MAX; i++) {
		g_queue_push_tail(&osd->rgn_handle_pool, GINT_TO_POINTER(i + 1));
	}

	/* Initialize the video stream descriptor */
	osd->streams = g_hash_table_new_full(g_direct_hash,
										 g_direct_equal,
										 NULL,
										 free_video_stream_callback);

	return osd;
}

void
ipcam_osd_destroy(IpcamOSD *osd)
{
	g_hash_table_unref(osd->streams);
	g_queue_clear(&osd->rgn_handle_pool);
	TTF_CloseFont(osd->ttf_font);
	TTF_Quit();
}

static gboolean
ipcam_osd_alloc_region(IpcamOSD *osd, RGN_HANDLE *rgn_handle)
{
	gint value;

	value = GPOINTER_TO_INT(g_queue_pop_head(&osd->rgn_handle_pool));
	if (value > 0) {
		*rgn_handle = value - 1;
		return TRUE;
	}
	return FALSE;
}

static void
ipcam_osd_release_region(IpcamOSD *osd, RGN_HANDLE rgn_handle)
{
	g_queue_push_tail(&osd->rgn_handle_pool, GINT_TO_POINTER(rgn_handle));
}

IpcamOSDStream*
ipcam_osd_add_stream(IpcamOSD *osd, VENC_GRP venc_grp)
{
	IpcamOSDStream *stream;

	stream = g_hash_table_lookup(osd->streams, GINT_TO_POINTER(venc_grp));
	if (stream == NULL) {
		stream = ipcam_osd_stream_new(osd, venc_grp);
		g_hash_table_insert(osd->streams, GINT_TO_POINTER(venc_grp), stream);
	}

	return stream;
}

void
ipcam_osd_delete_stream(IpcamOSD *osd, VENC_GRP venc_grp)
{
	g_hash_table_remove(osd->streams, GINT_TO_POINTER(venc_grp));
}

void
ipcam_osd_set_font_file(IpcamOSD *osd, char const *font_file)
{
}

IpcamOSDStream*
ipcam_osd_lookup_stream(IpcamOSD *osd, VENC_GRP venc_grp)
{
	return g_hash_table_lookup(osd->streams, GINT_TO_POINTER(venc_grp));
}

IpcamOSDItem*
ipcam_osd_lookup_item(IpcamOSD *osd, VENC_GRP venc_grp, char const *name)
{
	IpcamOSDStream *stream;

	stream = ipcam_osd_lookup_stream(osd, venc_grp);
	if (stream)
		return ipcam_osd_stream_lookup_item(stream, name);

	return NULL;
}

void
ipcam_osd_set_item_text(IpcamOSD *osd, VENC_GRP venc_grp, char const *name, char const *text)
{
	IpcamOSDItem *item;

	if (osd == NULL)
		return;

	item = ipcam_osd_lookup_item(osd, venc_grp, name);
	if (item) {
		ipcam_osd_item_set_text(item, text);
	}
}
