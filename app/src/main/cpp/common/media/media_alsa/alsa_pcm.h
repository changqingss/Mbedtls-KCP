#ifndef _ALSA_PCM_H_
#define _ALSA_PCM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#include "../../log/log_conf.h"
#include "libhardware2/alsa.h"
#define ALSA_PCM_DBG(fmt, ...) LOG_D("ALSA", fmt, ##__VA_ARGS__)
#define ALSA_PCM_ERR(fmt, ...) LOG_E("ALSA", fmt, ##__VA_ARGS__)
#define ALSA_PCM_INFO(fmt, ...) LOG_I("ALSA", fmt, ##__VA_ARGS__)

#define CARD_NAME "hw:0"
#define CARD_DEVICE_NAME "hw:0,0"
#define CTRL_NAME_MIC_VOL "Mic Volume"
#define CTRL_NAME_SPK_VOL "Master Playback Volume"

int alsa_volume_get(const char *card_name, const char *ctl_name, long *value);
int alsa_volume_set(const char *card_name, const char *ctl_name, long value);

int alsa_capture_pcm_int(const char *device_name, struct alsa_params args);
int alsa_capture_pcm(unsigned char *data, int dataLen);

int alsa_playback_pcm_int(const char *device_name, struct alsa_params args);
int alsa_playback_pcm(unsigned char *data, int dataLen);

void alsa_pcm_deinit(void);
void alsa_pcm_playback_deinit(void);

int alsa_playback_is_used(void);
int alsa_playback_drop(void);
void alsa_playback_flush(void);

#ifdef __cplusplus
}
#endif

#endif