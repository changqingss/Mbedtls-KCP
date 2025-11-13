#include "alsa_pcm.h"

#include <pthread.h>
#include <stdio.h>

int alsa_volume_get(const char *card_name, const char *ctl_name, long *value) {
  struct alsa_ctl *ctl = alsa_ctl_open(card_name, ctl_name);
  if (ctl == NULL) return -1;

  // long value;
  int ret = alsa_ctl_get_value(ctl, value);
  if (ret == 0) ALSA_PCM_INFO("%ld\n", *value);

  alsa_ctl_close(ctl);

  return ret;
}

int alsa_volume_set(const char *card_name, const char *ctl_name, long value) {
  struct alsa_ctl *ctl = alsa_ctl_open(card_name, ctl_name);
  if (ctl == NULL) return -1;

  int ret = alsa_ctl_set_value(ctl, value);

  alsa_ctl_close(ctl);

  return ret;
}

struct alsa_pcm *alsa = NULL;
struct alsa_pcm *alsa_playback_hand = NULL;
struct alsa_params params;
static int playback_is_used = 0;
pthread_mutex_t g_alsa_spk_init_mutex = PTHREAD_MUTEX_INITIALIZER;

int alsa_capture_pcm_int(const char *device_name, struct alsa_params args) {
  int ret = -1;
  ALSA_PCM_ERR("alsa_pcm_open_capture_device already init!");

  if (alsa) {
    ALSA_PCM_ERR("alsa_pcm_open_capture_device already init!");
    return 0;
  }

  alsa = alsa_pcm_open_capture_device(device_name);
  if (alsa == NULL) {
    ALSA_PCM_ERR("alsa_pcm_open_capture_device failed!");
    return ret;
  }

  params = args;

  ALSA_PCM_INFO("device_name:%s", device_name);
  ALSA_PCM_INFO("buffer_frames:%d buffer_time:%d channel:%d frame_bytes:%d period_frame:%d times:%d rate:%d",
                params.buffer_frames, params.buffer_time, params.channels, params.frame_bytes, params.period_frames,
                params.period_time, params.rate);

  ret = alsa_pcm_set_params(alsa, &params);
  if (ret) {
    ALSA_PCM_ERR("alsa_pcm_set_params failed!");
    return -1;
  }

  ALSA_PCM_INFO("device_name:%s", device_name);
  ALSA_PCM_INFO("buffer_frames:%d buffer_time:%d channel:%d frame_bytes:%d period_frame:%d times:%d rate:%d",
                params.buffer_frames, params.buffer_time, params.channels, params.frame_bytes, params.period_frames,
                params.period_time, params.rate);
}

int alsa_capture_pcm(unsigned char *data, int dataLen) {
  int ret = -1;

  if (alsa == NULL) {
    ALSA_PCM_ERR("alsa is NULL ret:%d", ret);
    return ret;
  }

  int n = dataLen / params.frame_bytes;
  ret = alsa_pcm_read(alsa, data, n);
  if (ret) {
    ALSA_PCM_ERR("alsa_pcm_read fail ret:%d", ret);
  }

  return ret;
}

int alsa_playback_pcm_int(const char *device_name, struct alsa_params args) {
  int ret = -1;

  pthread_mutex_lock(&g_alsa_spk_init_mutex);
  if (alsa_playback_hand) {
    ALSA_PCM_INFO("alsa_pcm_open_playback_device already init\n");
    pthread_mutex_unlock(&g_alsa_spk_init_mutex);
    return 0;
  }

  alsa_playback_hand = alsa_pcm_open_playback_device(device_name);
  if (alsa_playback_hand == NULL) {
    ALSA_PCM_ERR("alsa_pcm_open_playback_device failed!\n");
    pthread_mutex_unlock(&g_alsa_spk_init_mutex);
    return ret;
  }

  params = args;

  ALSA_PCM_INFO("playback buffer_frames:%d buffer_time:%d channel:%d frame_bytes:%d period_frame:%d times:%d rate:%d\n",
                params.buffer_frames, params.buffer_time, params.channels, params.frame_bytes, params.period_frames,
                params.period_time, params.rate);

  ret = alsa_pcm_set_params(alsa_playback_hand, &params);
  if (ret) {
    ALSA_PCM_ERR("alsa_pcm_set_params failed!");
    pthread_mutex_unlock(&g_alsa_spk_init_mutex);
    return -1;
  }
  pthread_mutex_unlock(&g_alsa_spk_init_mutex);

  ALSA_PCM_INFO("playback buffer_frames:%d buffer_time:%d channel:%d frame_bytes:%d period_frame:%d times:%d rate:%d\n",
                params.buffer_frames, params.buffer_time, params.channels, params.frame_bytes, params.period_frames,
                params.period_time, params.rate);
}

int alsa_playback_is_used(void) { return playback_is_used; }

int alsa_playback_pcm(unsigned char *data, int dataLen) {
  int ret = -1;

  if (alsa_playback_hand == NULL) return ret;
  if (data == NULL || dataLen <= 0) {
    return ret;
  }

  int n = dataLen / params.frame_bytes;
  ret = alsa_pcm_write(alsa_playback_hand, data, n);
  if (ret) {
    ALSA_PCM_ERR("alsa_pcm_read fail ret:%d", ret);
  }

  if (playback_is_used++ >= 90000) playback_is_used = 0;

  return ret;
}

int alsa_playback_drop(void) {
  if (alsa_playback_hand == NULL) {
    ALSA_PCM_ERR("alsa_playback_hand is NULL");
    return -1;
  }

  int ret = alsa_pcm_drop(alsa_playback_hand);
  if (ret) {
    ALSA_PCM_ERR("alsa_pcm_drop failed ret:%d", ret);
    return ret;
  }

  ALSA_PCM_INFO("alsa_pcm_drop success");
  return 0;  // return value of 0 indicates success for
}

void alsa_playback_flush(void) {
  if (alsa_playback_hand == NULL) {
    ALSA_PCM_ERR("alsa_playback_hand is NULL");
    return;
  }

  int ret = alsa_pcm_drain(alsa_playback_hand);
  if (ret) {
    ALSA_PCM_ERR("alsa_pcm_drain failed ret:%d", ret);
    return;
  }
  ALSA_PCM_INFO("alsa_pcm_drain success");

  ret = alsa_pcm_prepare(alsa_playback_hand);
  if (ret) {
    ALSA_PCM_ERR("alsa_pcm_prepare failed ret:%d", ret);
    return;
  }
  ALSA_PCM_INFO("alsa_pcm_drain and prepare success");
  ALSA_PCM_INFO("alsa_playback_flush success");
}

void alsa_pcm_deinit(void) {
  if (alsa) {
    alsa_pcm_close(alsa);
    alsa = NULL;
  }
}

void alsa_pcm_playback_deinit(void) {
  if (alsa_playback_hand) {
    alsa_pcm_close(alsa_playback_hand);
    alsa_playback_hand = NULL;
  }
}

int main_t() {
  int ret = -1;

  struct alsa_params par;
  par.buffer_frames = 8000;
  par.buffer_time = 500000;
  par.frame_bytes = 2;
  par.period_frames = 800;
  par.period_time = 50000;
  par.channels = 1;
  par.rate = 16000;
  par.format = SND_PCM_FORMAT_S16_LE;

  alsa_capture_pcm_int("hw:0,0", par);

  unsigned char data[1280] = {0};
  int dataLen = 1280;

  char *file_name = "/tmp/16k.pcm";
  FILE *fp = fopen(file_name, "w");
  if (fp) {
    int i = 0;
    for (i = 0; i < 100; i++) {
      alsa_capture_pcm(data, dataLen);
      ret = fwrite(data, dataLen, 1, fp);
      if (ret <= 0 && ferror(fp)) {
        fprintf(stderr, "failed to write %s: %s\n", file_name ? file_name : "stdout", strerror(errno));
        ret = -1;
      }
    }
  }
  fclose(fp);
  fp = NULL;
  alsa_pcm_deinit();

  alsa_playback_pcm_int("hw:0,0", params);

  fp = fopen(file_name, "r");
  if (fp) {
    int i = 0;
    for (i = 0; i < 100; i++) {
      fread(data, dataLen, 1, fp);
      alsa_playback_pcm(data, dataLen);
    }
  }
  fclose(fp);
  alsa_pcm_playback_deinit();
}