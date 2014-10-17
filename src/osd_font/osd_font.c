#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <hi_mem.h>
#include "media_osd_interface.h"
#include "osd_font.h"

enum
{
    PROP_0,
    PROP_FONT_SIZE,
    PROP_FONT_COLOR,
    N_PROPERTIES
};

typedef struct _IpcamOsdFontPrivate
{
    TTF_Font *font;    
    guint font_size;
    Color font_color;
} IpcamOsdFontPrivate;

G_DEFINE_TYPE_WITH_PRIVATE(IpcamOsdFont, ipcam_osd_font, G_TYPE_OBJECT)

static GParamSpec *obj_properties[N_PROPERTIES] = {NULL, };
static IpcamOsdFont *osd_font = NULL;

IpcamOsdFont *ipcam_osd_font_new(void)
{
    if (NULL == osd_font)
    {
        osd_font = g_object_new(IPCAM_OSD_FONT_TYPE, NULL);
    }
    
    return osd_font;
}

static void ipcam_osd_font_finalize(GObject *object)
{
    IpcamOsdFontPrivate *priv = ipcam_osd_font_get_instance_private(IPCAM_OSD_FONT(object));
    if (priv->font)
    {
        TTF_CloseFont(priv->font);
        priv->font = NULL;
    }
    TTF_Quit();
    G_OBJECT_CLASS(ipcam_osd_font_parent_class)->finalize(object);
}
static void ipcam_osd_font_init(IpcamOsdFont *self)
{
	IpcamOsdFontPrivate *priv = ipcam_osd_font_get_instance_private(self);
    priv->font_size = 18;
    gint32 s32Ret;

    /* Initialize the TTF library */
    s32Ret = TTF_Init();
	if (s32Ret < 0)
    {
		g_critical("Couldn't initialize TTF: %s\n", SDL_GetError());
		SDL_Quit();
		return;
	}

    priv->font = TTF_OpenFont("/usr/share/fonts/truetype/droid/DroidSansFallback.ttf", priv->font_size);
	if (NULL== priv->font)
    {
		g_critical("Couldn't load %s pt font from %d: %s\n", "ptsize", priv->font_size, SDL_GetError());
        return;
	}

    TTF_SetFontStyle(priv->font, TTF_STYLE_BOLD);
	TTF_SetFontOutline(priv->font, 0);
	TTF_SetFontKerning(priv->font, 0);
	TTF_SetFontHinting(priv->font, TTF_HINTING_LIGHT);
}
static void ipcam_osd_font_get_property(GObject    *object,
                                        guint       property_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
    IpcamOsdFont *self = IPCAM_OSD_FONT(object);
    IpcamOsdFontPrivate *priv = ipcam_osd_font_get_instance_private(self);
    switch(property_id)
    {
    case PROP_FONT_SIZE:
        {
            g_value_set_uint(value, priv->font_size);
        }
        break;
    case PROP_FONT_COLOR:
        {
            g_value_set_uint(value, priv->font_color.value);
        }
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}
static void ipcam_osd_font_set_property(GObject      *object,
                                        guint         property_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
    IpcamOsdFont *self = IPCAM_OSD_FONT(object);
    IpcamOsdFontPrivate *priv = ipcam_osd_font_get_instance_private(self);
    switch(property_id)
    {
    case PROP_FONT_SIZE:
        {
            priv->font_size = g_value_get_uint(value);
            if (NULL != priv->font)
            {
                TTF_SetFontSize(priv->font, priv->font_size);
            }
        }
        break;
    case PROP_FONT_COLOR:
        {
            priv->font_color.value = g_value_get_uint(value);
        }
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}
static void ipcam_osd_font_class_init(IpcamOsdFontClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = &ipcam_osd_font_finalize;
    object_class->get_property = &ipcam_osd_font_get_property;
    object_class->set_property = &ipcam_osd_font_set_property;

    obj_properties[PROP_FONT_SIZE] =
        g_param_spec_uint("font-size",
                          "font size",
                          "font-size.",
                          1,
                          64,
                          18,
                          G_PARAM_READWRITE);
    obj_properties[PROP_FONT_COLOR] =
        g_param_spec_uint("font-color",
                          "font color",
                          "font-color.",
                          0,
                          0xFFFFFFFF,
                          0,
                          G_PARAM_READWRITE);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}
gboolean ipcam_osd_font_render_text(IpcamOsdFont *self,
                                    const gchar *string,
                                    void **data,
                                    guint *width,
                                    guint *height)
{
    g_return_val_if_fail(IPCAM_IS_OSD_FONT(self), FALSE);
    gboolean ret = FALSE;
    SDL_Surface *text = NULL, *text_rgb1555 = NULL;
    IpcamOsdFontPrivate *priv = ipcam_osd_font_get_instance_private(self);
    SDL_Color forecol= {priv->font_color.red,
                        priv->font_color.green,
                        priv->font_color.blue,
                        priv->font_color.alpha};
    g_return_val_if_fail((NULL != priv->font), FALSE);
	text = TTF_RenderUTF8_Solid(priv->font, string, forecol);

    /* Convert to 16 bits per pixel */
    text_rgb1555 = SDL_CreateRGBSurface(SDL_SWSURFACE, 
                                        text->w,
                                        text->h,
                                        16,
                                        0x1F << 10,
                                        0x1F << 5,
                                        0x1F << 0,
                                        0x1 << 15);
    SDL_Rect bounds;
    if (NULL != text_rgb1555 && NULL != text)
    {
    	bounds.x = 0;
    	bounds.y = 0;
    	bounds.w = text->w;
    	bounds.h = text->h;
    	if (SDL_BlitSurface(text, &bounds, text_rgb1555, NULL) < 0)
        {
    		g_critical("Couldn't convert image to 16 bpp");
    	}
        else
        {
            //SDL_SaveBMP(text_rgb1555, content);
            //g_print("%s\n", SDL_GetPixelFormatName(text_rgb1555->format->format));
            *width = text->w;
            *height = text->h;
            *data = g_malloc(text->w * text->h * 2);
            SDL_LockSurface(text_rgb1555);
            memcpy((gchar *)*data, (gchar *)text_rgb1555->pixels, text->w * text->h * 2);
            SDL_UnlockSurface(text_rgb1555);
            ret = TRUE;
        }
    }

    if (NULL != text_rgb1555)
    {
        SDL_FreeSurface(text_rgb1555);
    }
    if (NULL != text)
    {
        SDL_FreeSurface(text);
    }

    return ret;
}
