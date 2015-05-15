#include <unistd.h>
#include <stdlib.h>
#include <mpi_sys.h>
#include <glib.h>

#include "media-ircut.h"

typedef struct HI35XX_ADC_REG
{
	HI_U32 status;
	HI_U32 ctrl;
	HI_U32 powerdown;
	HI_U32 int_status;
	HI_U32 int_mask;
	HI_U32 int_clr;
	HI_U32 int_raw;
	HI_U32 result;
} HI35XX_ADC_REG;

typedef struct HI35XX_GPIO_REG
{
	HI_U32 data[0x100];
	HI_U32 dir;
	HI_U32 is;
	HI_U32 ibe;
	HI_U32 iev;
	HI_U32 ie;
	HI_U32 ris;
	HI_U32 mis;
	HI_U32 ic;
} HI35XX_GPIO_REG;

typedef struct HI35XX_PWM_REG
{
	HI_U32 pwm0_cfg0;
	HI_U32 pwm0_cfg1;
	HI_U32 pwm0_cfg2;
	HI_U32 pwm0_ctrl;
	HI_U32 pwm0_state0;
	HI_U32 pwm0_state1;
	HI_U32 pwm0_state2;
	HI_U32 pwm0_dummy;
	HI_U32 pwm1_cfg0;
	HI_U32 pwm1_cfg1;
	HI_U32 pwm1_cfg2;
	HI_U32 pwm1_ctrl;
	HI_U32 pwm1_state0;
	HI_U32 pwm1_state1;
	HI_U32 pwm1_state2;
} HI35XX_PWM_REG;

struct MediaIrCut
{
	gboolean status;
	gboolean hw_found;
	guint16  sensitivity;
	guint16  hysteresis;
	guint    pwm_duty;
	guint32  count;
	volatile HI35XX_ADC_REG *adc_base;
	volatile HI35XX_GPIO_REG *gpio8_base;
	volatile HI35XX_PWM_REG *pwm_base;
};

static gboolean media_ircut_hw_detect(void);
static void media_ircut_initialize(MediaIrCut *ircut);

MediaIrCut *media_ircut_new(guint16 sensitivity, guint16 hysteresis)
{
	MediaIrCut *ircut = NULL;

	if (!media_ircut_hw_detect())
		return NULL;

	ircut = g_malloc0(sizeof(MediaIrCut));

	g_return_val_if_fail(ircut != NULL, NULL);

	ircut->status = FALSE;
	ircut->sensitivity = sensitivity;
	ircut->hysteresis = hysteresis;
	ircut->pwm_duty = 100;
	ircut->count = 0;

	ircut->adc_base = HI_MPI_SYS_Mmap(0x200B0000, 0x1000);
	ircut->gpio8_base = HI_MPI_SYS_Mmap(0x201C0000, 0x1000);
	ircut->pwm_base = HI_MPI_SYS_Mmap(0x20130000, 0x1000);

	media_ircut_initialize(ircut);

	return ircut;
}

void media_ircut_free(MediaIrCut *ircut)
{
	g_return_if_fail(ircut != NULL);

	HI_MPI_SYS_Munmap((HI_VOID*)ircut->adc_base, 0x1000);
	HI_MPI_SYS_Munmap((HI_VOID*)ircut->gpio8_base, 0x1000);
	HI_MPI_SYS_Munmap((HI_VOID*)ircut->pwm_base, 0x1000);

	g_free(ircut);
}

static gboolean media_ircut_hw_detect(void)
{
	gboolean result;
	volatile HI35XX_GPIO_REG *gpio1;

	gpio1 = HI_MPI_SYS_Mmap(0x20150000, 0x1000);
	if (!gpio1)
		return FALSE;

	gpio1->dir &= ~(1 << 0);

	result = gpio1->data[1 << 0] ? TRUE : FALSE;

	HI_MPI_SYS_Munmap((void*)gpio1, 0x1000);

	return result;
}

static void media_ircut_enable_color_filter(MediaIrCut *ircut)
{
	g_return_if_fail(ircut != NULL);

	ircut->gpio8_base->data[0x60] = 0x40;
}

static void media_ircut_disable_color_filter(MediaIrCut *ircut)
{
	g_return_if_fail(ircut != NULL);

	ircut->gpio8_base->data[0x60] = 0x20;
}

static void media_ircut_enable_pwm_output(MediaIrCut *ircut)
{
	int cycles = 3000000 / 3000;

	ircut->pwm_base->pwm1_ctrl = 0;
	ircut->pwm_base->pwm1_cfg0 = cycles;
	ircut->pwm_base->pwm1_cfg1 = cycles * ircut->pwm_duty / 100;
	ircut->pwm_base->pwm1_cfg2 = 1023;
	ircut->pwm_base->pwm1_ctrl = 5;			/* keep outout & start */

	ircut->gpio8_base->data[0x02] = 0x02;
}

static void media_ircut_disable_pwm_output(MediaIrCut *ircut)
{
	ircut->gpio8_base->data[0x02] = 0x00;
}

static guint16 media_ircut_get_adc_value(MediaIrCut *ircut)
{
	int cnt;
	guint16 result = 0;

	ircut->adc_base->ctrl = 0x00010001; /* start covert */
	/* wait for interrupt */
	for (cnt = 0; !(ircut->adc_base->int_status & 0x1) && cnt < 10; cnt++) {
		usleep(100);
	}

	result = ircut->adc_base->result;   /* get result */
	ircut->adc_base->int_clr = 1;		/* clear interrupt */

	return result & 0x3ff;
}

static void media_ircut_initialize(MediaIrCut *ircut)
{
	g_return_if_fail(ircut != NULL);

	/* SAR_ADC initialize */
	ircut->adc_base->powerdown = 0;		/* power on */
	ircut->adc_base->ctrl = 0x00010000; /* select ADC1 */
	ircut->adc_base->int_mask = 0;      /* enable ADC interrupt */

	/* IrCut optical filter initialize */
	/* GPIO8_1/GPIO8_5/GPIO8_6 */
	ircut->gpio8_base->dir |= (1 << 1) | (1 << 5) | (1 << 6);
	/* Enable color filter default */
	media_ircut_enable_color_filter(ircut);
	/* Ir Output enable */
	ircut->gpio8_base->data[0x02] = 0x02;

	/* PWM initialize */
	media_ircut_disable_pwm_output(ircut);
}

#define IRCUT_TRIGGER_COUNT  10

gboolean media_ircut_poll(MediaIrCut *ircut)
{
	gboolean triggered = FALSE;

	g_return_val_if_fail(ircut != NULL, FALSE);

	guint16 value = media_ircut_get_adc_value(ircut);

	if (!ircut->status) {		/* IrCut not enabled */
		guint16 threshold = ircut->sensitivity < ircut->hysteresis ? 0 : 
			ircut->sensitivity - ircut->hysteresis;
		if (value < threshold) {
			if (ircut->count > IRCUT_TRIGGER_COUNT) {
				media_ircut_disable_color_filter(ircut);
				media_ircut_enable_pwm_output(ircut);
				ircut->status = TRUE;
				ircut->count = 0;

				triggered = TRUE;
			}
			else {
				ircut->count++;
			}
		}
		else {
			ircut->count = 0;
		}
	}
	else {						/* IrCut already enabled */
		guint16 threshold = ircut->sensitivity;
		if (value > threshold) {
			if (ircut->count > IRCUT_TRIGGER_COUNT) {
				media_ircut_enable_color_filter(ircut);
				media_ircut_disable_pwm_output(ircut);
				ircut->status = FALSE;
				ircut->count = 0;

				triggered = TRUE;
			}
			else {
				ircut->count++;
			}
		}
		else {
			ircut->count = 0;
		}
	}

	return triggered;
}

void media_ircut_set_sensitivity(MediaIrCut *ircut, guint16 value)
{
	g_return_if_fail(ircut != NULL);

	ircut->sensitivity = value * 1024 / 100;
}

void media_ircut_set_ir_intensity(MediaIrCut *ircut, guint16 value)
{
	g_return_if_fail(ircut != NULL);

	ircut->pwm_duty = value;

	if (ircut->status)
		media_ircut_enable_pwm_output(ircut);
}

gboolean media_ircut_get_status(MediaIrCut *ircut)
{
	g_return_val_if_fail(ircut != NULL, FALSE);

	return ircut->status;
}
