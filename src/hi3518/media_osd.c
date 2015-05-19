#include <hi_type.h>
#include <hi_defines.h>
#include <hi_comm_region.h>
#include <mpi_region.h>
#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "media_osd.h"
#include "bitmap.h"
#include "stream_descriptor.h"

enum
{
    PROP_0,
    PROP_RGN_HANDLE,
    PROP_VENC_GROUP,
	PROP_FONT_FILE,
    N_PROPERTIES
};

typedef struct _IpcamMediaOsdPrivate
{
	TTF_Font *ttf_font;
	gchar *font_file;	
    IpcamBitmap *bitmap;
    VENC_GRP VencGrp;
    RGN_HANDLE RgnHandle;
    RGN_ATTR_S stRgnAttr;
    RGN_CHN_ATTR_S stChnAttr;
    guint32 image_width;
    guint32 image_height;
    guint32 font_size[IPCAM_OSD_TYPE_LAST];
    Color color[IPCAM_OSD_TYPE_LAST];
    Point position[IPCAM_OSD_TYPE_LAST];
    RECT_S rect[IPCAM_OSD_TYPE_LAST];
    HI_BOOL bShow[IPCAM_OSD_TYPE_LAST];
    gchar *content[IPCAM_OSD_TYPE_LAST];
} IpcamMediaOsdPrivate;

static gint32 ipcam_media_osd_draw_content(IpcamMediaOsd *self, IPCAM_OSD_TYPE type);

G_DEFINE_TYPE(IpcamMediaOsd, ipcam_media_osd, G_TYPE_OBJECT);

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

static void ipcam_media_osd_get_property(GObject    *object,
                                         guint       property_id,
                                         GValue     *value,
                                         GParamSpec *pspec)
{
    IpcamMediaOsd *self = IPCAM_MEDIA_OSD(object);
    IpcamMediaOsdPrivate *priv = IPCAM_MEDIA_OSD_GET_PRIVATE(self);
    switch(property_id)
    {
        case PROP_RGN_HANDLE:
            g_value_set_uint(value, priv->RgnHandle);
            break;
        case PROP_VENC_GROUP:
            g_value_set_uint(value, priv->VencGrp);
            break;
		case PROP_FONT_FILE:
			g_value_set_string(value, priv->font_file);
			break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static void ipcam_media_osd_set_property(GObject      *object,
                                         guint         property_id,
                                         const GValue *value,
                                         GParamSpec   *pspec)
{
    IpcamMediaOsd *self = IPCAM_MEDIA_OSD(object);
    IpcamMediaOsdPrivate *priv = IPCAM_MEDIA_OSD_GET_PRIVATE(self);
    switch(property_id)
    {
        case PROP_RGN_HANDLE:
            priv->RgnHandle = g_value_get_uint(value);
            break;
        case PROP_VENC_GROUP:
            priv->VencGrp = g_value_get_uint(value);
            break;
		case PROP_FONT_FILE:
			priv->font_file = g_strdup(g_value_get_string(value));
			break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

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

	g_free(priv->font_file);

	if (priv->ttf_font)
		TTF_CloseFont(priv->ttf_font);
	TTF_Quit();

	g_clear_object(&priv->bitmap);
    G_OBJECT_CLASS(ipcam_media_osd_parent_class)->finalize(object);
}

static GObject *ipcam_media_osd_constructor (GType gtype,
                                             guint n_properties,
                                             GObjectConstructParam *properties)
{
    GObjectClass *klass;
    GObject *object;
    IpcamMediaOsd *osd;
    IpcamMediaOsdPrivate *priv;
    HI_S32 s32Ret = HI_FAILURE;
    MPP_CHN_S stChn;
    IPCAM_OSD_TYPE type;

    /* Always chain up to the parent constructor */
    klass = G_OBJECT_CLASS(ipcam_media_osd_parent_class);
    object = klass->constructor(gtype, n_properties, properties);
    osd = IPCAM_MEDIA_OSD(object);
    priv = IPCAM_MEDIA_OSD_GET_PRIVATE(osd);

    for (type = IPCAM_OSD_TYPE_DATETIME; type < IPCAM_OSD_TYPE_LAST; type++)
    {
        priv->content[type] = NULL;
        priv->bShow[type] = FALSE;

        priv->rect[type].s32X = 0;
        priv->rect[type].s32Y = 0;
        priv->rect[type].u32Width = 0;
        priv->rect[type].u32Height = 0;
    }

    priv->bitmap = g_object_new(IPCAM_BITMAP_TYPE, NULL);

	if (TTF_Init() < 0) {
		g_critical("Couldn't initialize TTF: %s\n", SDL_GetError());
	}

	if (priv->font_file) {
		g_print("loading font %s\n", priv->font_file);
		priv->ttf_font = TTF_OpenFont(priv->font_file, 20);
	}

	/* Use fallback */
	if (!priv->ttf_font)
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
        priv->stChnAttr.unChnAttr.stOverlayChn.u32BgAlpha = 32;
        priv->stChnAttr.unChnAttr.stOverlayChn.u32FgAlpha = 96;
        priv->stChnAttr.unChnAttr.stOverlayChn.u32Layer = 0;

        priv->stChnAttr.unChnAttr.stOverlayChn.stQpInfo.bAbsQp = HI_FALSE;
        priv->stChnAttr.unChnAttr.stOverlayChn.stQpInfo.s32Qp  = 0;

        priv->stChnAttr.unChnAttr.stOverlayChn.stInvertColor.stInvColArea.u32Width = 16;
        priv->stChnAttr.unChnAttr.stOverlayChn.stInvertColor.stInvColArea.u32Height = 16;
        priv->stChnAttr.unChnAttr.stOverlayChn.stInvertColor.u32LumThresh = 70;
        priv->stChnAttr.unChnAttr.stOverlayChn.stInvertColor.enChgMod = LESSTHAN_LUM_THRESH;
        priv->stChnAttr.unChnAttr.stOverlayChn.stInvertColor.bInvColEn = HI_FALSE;

        s32Ret = HI_MPI_RGN_AttachToChn(priv->RgnHandle, &stChn, &priv->stChnAttr);
        if (HI_SUCCESS != s32Ret)
        {
            g_critical("HI_MPI_RGN_AttachToChn failed with %#x!\n", s32Ret);
            break;
        }
    } while (FALSE);

    return object;
}

static void ipcam_media_osd_class_init(IpcamMediaOsdClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->get_property = ipcam_media_osd_get_property;
    object_class->set_property = ipcam_media_osd_set_property;
    object_class->constructor = ipcam_media_osd_constructor;
    object_class->finalize = ipcam_media_osd_finalize;

    g_type_class_add_private(klass, sizeof(IpcamMediaOsdPrivate));

    obj_properties[PROP_RGN_HANDLE] = 
        g_param_spec_uint ("RgnHandle",
                           "Region Handle",
                           "Region Handle",
                           0, RGN_HANDLE_MAX - 1,
                           0,
                           G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
    obj_properties[PROP_VENC_GROUP] = 
        g_param_spec_uint ("VencGroup",
                           "VENC Group Number",
                           "VENC Group Number",
                           0, VENC_MAX_GRP_NUM - 1,
                           0,
                           G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
    obj_properties[PROP_FONT_FILE] = 
        g_param_spec_string ("font",
                           "Font file name",
                           "Font file name",
                           NULL,
                           G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
    g_object_class_install_properties (object_class,
                                       N_PROPERTIES,
                                       obj_properties);
}

static void ipcam_media_osd_init(IpcamMediaOsd *self)
{
    IpcamMediaOsdPrivate *priv = IPCAM_MEDIA_OSD_GET_PRIVATE(self);

    priv->image_width = 1920;
    priv->image_height = 1080;
}

#if 0
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
#endif

gint32 ipcam_media_osd_start(IpcamMediaOsd *self, IPCAM_OSD_TYPE type, IpcamOSDParameter *parameter)
{
    IpcamMediaOsdPrivate *priv = IPCAM_MEDIA_OSD_GET_PRIVATE(self);

    priv->bShow[type] = parameter->is_show;
    priv->font_size[type] = parameter->font_size;
    priv->color[type] = parameter->color;
    priv->position[type].x = parameter->position.x;
    priv->position[type].y = parameter->position.y;
    
    return ipcam_media_osd_draw_content(self, type);
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

gint32 ipcam_media_osd_show(IpcamMediaOsd *self, IPCAM_OSD_TYPE type, const gboolean show)
{
    IpcamMediaOsdPrivate *priv = IPCAM_MEDIA_OSD_GET_PRIVATE(self);
    priv->bShow[type] = show;
    return ipcam_media_osd_set_content(self, type, priv->content[type]);
}

gint32 ipcam_media_osd_set_pos(IpcamMediaOsd *self, IPCAM_OSD_TYPE type,  const Point pos)
{
    IpcamMediaOsdPrivate *priv = IPCAM_MEDIA_OSD_GET_PRIVATE(self);
    priv->position[type].x = pos.x;
    priv->position[type].y = pos.y;
    return ipcam_media_osd_set_region_attr(self, type);
}

gint32 ipcam_media_osd_set_fontsize(IpcamMediaOsd *self, IPCAM_OSD_TYPE type, const guint fsize)
{
    IpcamMediaOsdPrivate *priv = IPCAM_MEDIA_OSD_GET_PRIVATE(self);
    priv->font_size[type] = fsize;
    return ipcam_media_osd_set_content(self, type, priv->content[type]);
}

gint32 ipcam_media_osd_set_color(IpcamMediaOsd *self, IPCAM_OSD_TYPE type, const Color color)
{
    IpcamMediaOsdPrivate *priv = IPCAM_MEDIA_OSD_GET_PRIVATE(self);
    priv->color[type] = color;
    return ipcam_media_osd_set_content(self, type, priv->content[type]);
}

static gint32 ipcam_media_osd_draw_content(IpcamMediaOsd *self, IPCAM_OSD_TYPE type)
{
	SDL_Surface *text_sf, *scrn_sf;
    HI_S32 s32Ret = HI_FAILURE;
    IpcamMediaOsdPrivate *priv = IPCAM_MEDIA_OSD_GET_PRIVATE(self);

    g_return_val_if_fail((type >= 0 && type < IPCAM_OSD_TYPE_LAST), s32Ret);
    g_return_val_if_fail((NULL != priv->ttf_font), s32Ret);

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

    if (priv->bShow[type] && priv->content[type])
    {
		SDL_Color fgclr= {
			priv->color[type].red,
			priv->color[type].green,
			priv->color[type].blue,
			priv->color[type].alpha
		};
        SDL_Color bgclr = {
            128, 128, 128, 1
        };

        TTF_SetFontSize(priv->ttf_font, priv->font_size[type]);
		text_sf = TTF_RenderUTF8_Shaded(priv->ttf_font, priv->content[type], fgclr, bgclr);
        if (text_sf) {
            rect.x = priv->position[type].x * priv->image_width / 1000;
            rect.y = priv->position[type].y * priv->image_height / 1000;
            rect.w = text_sf->w;
            rect.h = text_sf->h;
            if (rect.x + rect.w > priv->image_width)
                rect.x = priv->image_width - rect.w;
            if (rect.y + rect.h > priv->image_height)
                rect.y = priv->image_height - rect.h;
            SDL_BlitSurface(text_sf, NULL, scrn_sf, &rect);

            priv->rect[type].s32X = rect.x;
            priv->rect[type].s32Y = rect.y;
            priv->rect[type].u32Width = rect.w;
            priv->rect[type].u32Height = rect.h;

            SDL_FreeSurface(text_sf);
        }
    }
    else
    {
        s32Ret = HI_SUCCESS;
    }

	SDL_FreeSurface(scrn_sf);

    return s32Ret;
}

gint32 ipcam_media_osd_set_content(IpcamMediaOsd *self, IPCAM_OSD_TYPE type, const gchar *content)
{
    IpcamMediaOsdPrivate *priv = IPCAM_MEDIA_OSD_GET_PRIVATE(self);

    if (priv->content[type])
        g_free(priv->content[type]);

    priv->content[type] = g_strdup(content);
    
    return ipcam_media_osd_draw_content(self, type);
}

gint32 ipcam_media_osd_invalidate(IpcamMediaOsd *self)
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

gint32 ipcam_media_osd_stop(IpcamMediaOsd *self)
{
    HI_S32 s32Ret = HI_SUCCESS;
    
    return s32Ret;
}

void ipcam_media_osd_set_image_size(IpcamMediaOsd *self, guint32 width, guint32 height)
{
    IpcamMediaOsdPrivate *priv = IPCAM_MEDIA_OSD_GET_PRIVATE(self);
    int i;

    priv->image_width = width;
    priv->image_height = height;

    for (i = 0; i < IPCAM_OSD_TYPE_LAST; i++)
        ipcam_media_osd_draw_content(self, i);
}
