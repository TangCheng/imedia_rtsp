#include <hi_type.h>
#include <hi_defines.h>
#include <hi_comm_region.h>
#include <mpi_region.h>
#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "interface/media_osd_interface.h"
#include "media_osd.h"
#include "bitmap.h"
#include "stream_descriptor.h"

typedef struct _IpcamMediaOsdPrivate
{
	TTF_Font *ttf_font;
    IpcamBitmap *bitmap;
    VENC_GRP VencGrp;
    RGN_HANDLE RgnHandle;
    RGN_ATTR_S stRgnAttr;
    RGN_CHN_ATTR_S stChnAttr;
    guint32 font_size[IPCAM_OSD_TYPE_LAST];
    Color color[IPCAM_OSD_TYPE_LAST];
    RECT_S rect[IPCAM_OSD_TYPE_LAST];
    HI_BOOL bShow[IPCAM_OSD_TYPE_LAST];
    gchar *content[IPCAM_OSD_TYPE_LAST];
} IpcamMediaOsdPrivate;

static void ipcam_iosd_interface_init(IpcamIOSDInterface *iface);
static gint32 ipcam_media_osd_set_content(IpcamMediaOsd *self, IPCAM_OSD_TYPE type, const gchar *content);

G_DEFINE_TYPE_WITH_CODE(IpcamMediaOsd, ipcam_media_osd, G_TYPE_OBJECT,
                        G_IMPLEMENT_INTERFACE(IPCAM_TYPE_IOSD,
                                              ipcam_iosd_interface_init));

static void ipcam_media_osd_finalize(GObject *object)
{
    HI_S32 s32Ret = HI_FAILURE;
    MPP_CHN_S stChn;
    IPCAM_OSD_TYPE type;
    IpcamMediaOsdPrivate *priv = IPCAM_MEDIA_OSD_GET_PRIVATE(IPCAM_MEDIA_OSD(object));

    stChn.enModId = HI_ID_GROUP;
    stChn.s32DevId = priv->VencGrp;
    stChn.s32ChnId = 0;

    s32Ret = HI_MPI_RGN_DetachFrmChn(priv->RgnHandle, &stChn);
    if(HI_SUCCESS != s32Ret)
    {
        g_critical("HI_MPI_RGN_DetachFrmChn failed with %#x!\n", s32Ret);
    }
    s32Ret = HI_MPI_RGN_Destroy(priv->RgnHandle);
    if (HI_SUCCESS != s32Ret)
    {
        g_critical("HI_MPI_RGN_Destroy failed with %#x\n", s32Ret);
    }
    for (type = IPCAM_OSD_TYPE_DATETIME; type < IPCAM_OSD_TYPE_LAST; type++)
    {
        g_free(priv->content[type]);
        priv->content[type] = NULL;
    }

	if (priv->ttf_font)
		TTF_CloseFont(priv->ttf_font);
	TTF_Quit();

	g_clear_object(&priv->bitmap);
    G_OBJECT_CLASS(ipcam_media_osd_parent_class)->finalize(object);
}

static void ipcam_media_osd_init(IpcamMediaOsd *self)
{
    HI_S32 s32Ret = HI_FAILURE;
    MPP_CHN_S stChn;
    IpcamMediaOsdPrivate *priv = IPCAM_MEDIA_OSD_GET_PRIVATE(self);
    IPCAM_OSD_TYPE type;
    for (type = IPCAM_OSD_TYPE_DATETIME; type < IPCAM_OSD_TYPE_LAST; type++)
    {
        priv->content[type] = NULL;
    }
    priv->RgnHandle = 0;
    priv->VencGrp = 0;
    priv->bitmap = g_object_new(IPCAM_BITMAP_TYPE, NULL);

	if (TTF_Init() < 0) {
		g_critical("Couldn't initialize TTF: %s\n", SDL_GetError());
	}
	priv->ttf_font = TTF_OpenFont("/usr/share/fonts/truetype/droid/DroidSansFallback.ttf", 20);
	if (!priv->ttf_font) {
		g_critical("Couldn't load %s pt font from %d: %s\n", "ptsize", 20, SDL_GetError());
	}
    TTF_SetFontStyle(priv->ttf_font, TTF_STYLE_BOLD);
	TTF_SetFontOutline(priv->ttf_font, 0);
	TTF_SetFontKerning(priv->ttf_font, 0);
	TTF_SetFontHinting(priv->ttf_font, TTF_HINTING_LIGHT);

	do
    {
        priv->stRgnAttr.enType = OVERLAY_RGN;
        priv->stRgnAttr.unAttr.stOverlay.enPixelFmt = PIXEL_FORMAT_RGB_1555;
        priv->stRgnAttr.unAttr.stOverlay.stSize.u32Width  = (IMAGE_MAX_WIDTH / 16 + 1) * 16;
        priv->stRgnAttr.unAttr.stOverlay.stSize.u32Height = (IMAGE_MAX_HEIGHT / 16 + 1) * 16;
        priv->stRgnAttr.unAttr.stOverlay.u32BgColor = 0x7FFF;

        s32Ret = HI_MPI_RGN_Create(priv->RgnHandle, &priv->stRgnAttr);
        if (HI_SUCCESS != s32Ret)
        {
            g_critical("HI_MPI_RGN_Create failed with %#x!\n", s32Ret);
            break;
        }

        stChn.enModId = HI_ID_GROUP;
        stChn.s32DevId = priv->VencGrp;
        stChn.s32ChnId = 0;

        memset(&priv->stChnAttr, 0, sizeof(priv->stChnAttr));
        priv->stChnAttr.bShow = HI_TRUE;
        priv->stChnAttr.enType = OVERLAY_RGN;
        priv->stChnAttr.unChnAttr.stOverlayChn.stPoint.s32X = 0;
        priv->stChnAttr.unChnAttr.stOverlayChn.stPoint.s32Y = 0;
        priv->stChnAttr.unChnAttr.stOverlayChn.u32BgAlpha = 0;
        priv->stChnAttr.unChnAttr.stOverlayChn.u32FgAlpha = 128;
        priv->stChnAttr.unChnAttr.stOverlayChn.u32Layer = 0;

        priv->stChnAttr.unChnAttr.stOverlayChn.stQpInfo.bAbsQp = HI_FALSE;
        priv->stChnAttr.unChnAttr.stOverlayChn.stQpInfo.s32Qp  = 0;

        priv->stChnAttr.unChnAttr.stOverlayChn.stInvertColor.stInvColArea.u32Width = 16;
        priv->stChnAttr.unChnAttr.stOverlayChn.stInvertColor.stInvColArea.u32Height = 16;
        priv->stChnAttr.unChnAttr.stOverlayChn.stInvertColor.u32LumThresh = 70;
        priv->stChnAttr.unChnAttr.stOverlayChn.stInvertColor.enChgMod = LESSTHAN_LUM_THRESH;
        priv->stChnAttr.unChnAttr.stOverlayChn.stInvertColor.bInvColEn = HI_TRUE;

        s32Ret = HI_MPI_RGN_AttachToChn(priv->RgnHandle, &stChn, &priv->stChnAttr);
        if (HI_SUCCESS != s32Ret)
        {
            g_critical("HI_MPI_RGN_AttachToChn failed with %#x!\n", s32Ret);
            break;
        }
    } while (FALSE);
}
static void ipcam_media_osd_class_init(IpcamMediaOsdClass *klass)
{
    g_type_class_add_private(klass, sizeof(IpcamMediaOsdPrivate));
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = &ipcam_media_osd_finalize;
}

static void ipcam_media_osd_bitmap_clear(IpcamBitmap *bitmap, RECT_S *rect)
{
	SDL_Surface *scrn_sf;
	BITMAP_S *bmp;
	SDL_Rect rc = { rect->s32X, rect->s32Y, rect->u32Width, rect->u32Height	};

	g_return_if_fail(bitmap);

	bmp = ipcam_bitmap_get_data(bitmap);
	scrn_sf = SDL_CreateRGBSurfaceFrom(bmp->pData, 
	                                   bmp->u32Width,
	                                   bmp->u32Height,
	                                   16,
	                                   bmp->u32Width * 2,
	                                   0x1F << 10,
	                                   0x1F << 5,
	                                   0x1F << 0,
	                                   0x1 << 15);
	if (scrn_sf) {
		SDL_FillRect(scrn_sf, &rc, 0);
		SDL_FreeSurface(scrn_sf);
	}
}

static gint32 ipcam_media_osd_start(IpcamMediaOsd *self, IPCAM_OSD_TYPE type, IpcamOSDParameter *parameter)
{
    IpcamMediaOsdPrivate *priv = IPCAM_MEDIA_OSD_GET_PRIVATE(self);

	ipcam_media_osd_bitmap_clear(priv->bitmap, &priv->rect[type]);
    priv->bShow[type] = parameter->is_show;
    priv->font_size[type] = parameter->font_size;
    priv->color[type] = parameter->color;
    priv->rect[type].s32X = parameter->position.x;
    priv->rect[type].s32Y = parameter->position.y;
    
    return ipcam_media_osd_set_content(self, type, priv->content[type]);
}
static gint32 ipcam_media_osd_set_region_attr(IpcamMediaOsd *self, IPCAM_OSD_TYPE type)
{
    HI_S32 s32Ret = HI_FAILURE;
    MPP_CHN_S stChn;
    IpcamMediaOsdPrivate *priv = IPCAM_MEDIA_OSD_GET_PRIVATE(self);
    stChn.enModId = HI_ID_GROUP;
    stChn.s32DevId = priv->VencGrp;
    stChn.s32ChnId = 0;
    s32Ret = HI_MPI_RGN_SetDisplayAttr(priv->RgnHandle, &stChn, &priv->stChnAttr);
    if (HI_SUCCESS != s32Ret)
    {
        g_critical("HI_MPI_RGN_SetDisplayAttr failed with %#x!\n", s32Ret);
    }
    return s32Ret;
}
static gint32 ipcam_media_osd_show(IpcamMediaOsd *self, IPCAM_OSD_TYPE type, const gboolean show)
{
    IpcamMediaOsdPrivate *priv = IPCAM_MEDIA_OSD_GET_PRIVATE(self);
    priv->bShow[type] = show;
    return ipcam_media_osd_set_content(self, type, priv->content[type]);
}
static gint32 ipcam_media_osd_set_pos(IpcamMediaOsd *self, IPCAM_OSD_TYPE type,  const Point pos)
{
    IpcamMediaOsdPrivate *priv = IPCAM_MEDIA_OSD_GET_PRIVATE(self);
    ipcam_media_osd_bitmap_clear(priv->bitmap, &priv->rect[type]);
    priv->rect[type].s32X = pos.x;
    priv->rect[type].s32Y = pos.y;
    return ipcam_media_osd_set_region_attr(self, type);
}
static gint32 ipcam_media_osd_set_fontsize(IpcamMediaOsd *self, IPCAM_OSD_TYPE type, const guint fsize)
{
    IpcamMediaOsdPrivate *priv = IPCAM_MEDIA_OSD_GET_PRIVATE(self);
    priv->font_size[type] = fsize;
    return ipcam_media_osd_set_content(self, type, priv->content[type]);
}
static gint32 ipcam_media_osd_set_color(IpcamMediaOsd *self, IPCAM_OSD_TYPE type, const Color color)
{
    IpcamMediaOsdPrivate *priv = IPCAM_MEDIA_OSD_GET_PRIVATE(self);
    priv->color[type] = color;
    return ipcam_media_osd_set_content(self, type, priv->content[type]);
}
static gint32 ipcam_media_osd_set_content(IpcamMediaOsd *self, IPCAM_OSD_TYPE type, const gchar *content)
{
	SDL_Surface *text_sf, *scrn_sf;
    HI_S32 s32Ret = HI_FAILURE;
    IpcamMediaOsdPrivate *priv = IPCAM_MEDIA_OSD_GET_PRIVATE(self);
        
    g_return_val_if_fail((NULL != priv->ttf_font), s32Ret);

    if (priv->content[type] != content && type != IPCAM_OSD_TYPE_DATETIME)
    {
        g_free(priv->content[type]);
        priv->content[type] = g_strdup(content);
    }

	SDL_Rect rect = {
		priv->rect[type].s32X, priv->rect[type].s32Y,
		priv->rect[type].u32Width, priv->rect[type].u32Height
	};
	BITMAP_S *bmp = ipcam_bitmap_get_data(priv->bitmap);
	scrn_sf = SDL_CreateRGBSurfaceFrom(bmp->pData, 
	                                   bmp->u32Width,
	                                   bmp->u32Height,
	                                   16,
	                                   bmp->u32Width * 2,
	                                   0x1F << 10,
	                                   0x1F << 5,
	                                   0x1F << 0,
	                                   0x1 << 15);
    SDL_FillRect(scrn_sf, &rect, 0);
	if (priv->bShow[type] && content)
    {
		SDL_Color foreclr= {
			priv->color[type].red,
			priv->color[type].green,
			priv->color[type].blue,
			priv->color[type].alpha
		};
		TTF_SetFontSize(priv->ttf_font, priv->font_size[type]);
		text_sf = TTF_RenderUTF8_Solid(priv->ttf_font, content, foreclr);
		SDL_BlitSurface(text_sf, NULL, scrn_sf, &rect);

		priv->rect[type].u32Width = text_sf->w;
		priv->rect[type].u32Height = text_sf->h;

		SDL_FreeSurface(text_sf);
    }
    else
    {
        s32Ret = HI_SUCCESS;
    }

	SDL_FreeSurface(scrn_sf);
    
    return s32Ret;
}
static gint32 ipcam_media_osd_invalidate(IpcamMediaOsd *self)
{
    HI_S32 s32Ret = HI_FAILURE;
    IpcamMediaOsdPrivate *priv = IPCAM_MEDIA_OSD_GET_PRIVATE(self);
    s32Ret = HI_MPI_RGN_SetBitMap(priv->RgnHandle, ipcam_bitmap_get_data(priv->bitmap));
    if(s32Ret != HI_SUCCESS)
    {
        g_critical("HI_MPI_RGN_SetBitMap failed with %#x!\n", s32Ret);
    }
    return s32Ret;
}
static gint32 ipcam_media_osd_stop(IpcamMediaOsd *self)
{
    HI_S32 s32Ret = HI_SUCCESS;
    
    return s32Ret;
}
static void ipcam_iosd_interface_init(IpcamIOSDInterface *iface)
{
    iface->start = ipcam_media_osd_start;
    iface->show = ipcam_media_osd_show;
    iface->set_pos = ipcam_media_osd_set_pos;
    iface->set_fontsize = ipcam_media_osd_set_fontsize;
    iface->set_color = ipcam_media_osd_set_color;
    iface->set_content = ipcam_media_osd_set_content;
    iface->invalidate = ipcam_media_osd_invalidate;
    iface->stop = ipcam_media_osd_stop;
}
