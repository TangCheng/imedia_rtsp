#ifndef __MEDIA_IRCUT_H_
#define __MEDIA_IRCUT_H_

struct MediaIrCut;
typedef struct MediaIrCut MediaIrCut;

MediaIrCut *media_ircut_new(guint16 sensitivity, guint16 hysteresis);
void media_ircut_free(MediaIrCut *ircut);
gboolean media_ircut_poll(MediaIrCut *ircut);
void media_ircut_set_sensitivity(MediaIrCut *ircut, guint16 value);
void media_ircut_set_ir_intensity(MediaIrCut *ircut, guint16 value);
gboolean media_ircut_get_status(MediaIrCut *ircut);

#endif /* __MEDIA_IRCUT_H_ */