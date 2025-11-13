#ifndef _AAC_ENCODER_H_
#define _AAC_ENCODER_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>

#include "aacdecoder_lib.h"
#include "aacenc_lib.h"

typedef struct {
  HANDLE_AACENCODER encoder;
  int input_size;
  int out_size;
} AAC_ENCODE_INFO_t, *PAAC_ENCODE_INFO_t;

typedef struct aacEncInfo {
  uint8_t inited;
  uint16_t e_aot;
  uint16_t e_bitRate;
  uint16_t e_vbr;
  uint16_t e_mode;
  char e_transport[16];
  char e_libName[16];
} aacCodecInfo_s;

/* AAC enc */
int media_aacInitEncoder(aacCodecInfo_s* aac_conf, int sample_rate, int channels, PAAC_ENCODE_INFO_t encode_info,
                         int bchn);
void media_aacDeInitEncoder(HANDLE_AACENCODER encoder);
int media_aacEncodeFrame(HANDLE_AACENCODER encoder, const uint8_t* in_buffer, int in_size, uint8_t* out_buffer,
                         int out_size, int* out_bytes);

/* AAC dec */
int media_aacInitDecoder(aacCodecInfo_s* aac_dec_conf);
void media_aacDeInitDecoder();
int media_aacDecodeFrame(unsigned char* pEncodedFrame, unsigned int encodedFrameLen, unsigned char* decFrm,
                         int* decFrmsize);

#ifdef __cplusplus
}
#endif

#endif
