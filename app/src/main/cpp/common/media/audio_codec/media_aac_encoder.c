#include "media_aac_encoder.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// #define AAC_CODEC_DEBUG_ENABLE 1

#ifdef AAC_CODEC_DEBUG_ENABLE

#define AAC_CODEC_DBG(str, args...) APPLOG_DBG(str, ##args)

#define AAC_CODEC_ERR(str, args...) APPLOG_ERR(str, ##args)

#define AAC_CODEC_INFO(str, args...) APPLOG_INFO(str, ##args)

#else
#define AAC_CODEC_DBG(fm, ...) printf(fm, ##__VA_ARGS__)
#define AAC_CODEC_ERR(fm, ...) printf(fm, ##__VA_ARGS__)
#define AAC_CODEC_INFO(fm, ...) printf(fm, ##__VA_ARGS__)
#endif

int media_aacInitEncoder(aacCodecInfo_s* aac_conf, int sample_rate, int channels, PAAC_ENCODE_INFO_t encode_info,
                         int bchn) {
  if (encode_info == NULL) {
    return -1;
  }

  int ret = -1;
  int bitrate = aac_conf->e_bitRate;
  int vbr = aac_conf->e_vbr;
  HANDLE_AACENCODER encoder = NULL;
  int aot = aac_conf->e_aot;  // AOT_AAC_LC;
  int afterburner = 1;
  CHANNEL_MODE mode = aac_conf->e_mode;
  AACENC_InfoStruct info = {0};
  uint16_t transportValue = 0;

  int input_size;

  AAC_CODEC_INFO("bitrate:%d, vbr:%d, mode:%d, bchn=%d", bitrate, vbr, mode, bchn);

  switch (channels) {
    case 1:
      mode = MODE_1;
      break;
    case 2:
      mode = MODE_2;
      break;
    case 3:
      mode = MODE_1_2;
      break;
    case 4:
      mode = MODE_1_2_1;
      break;
    case 5:
      mode = MODE_1_2_2;
      break;
    case 6:
      mode = MODE_1_2_2_1;
      break;
    default:
      AAC_CODEC_ERR("Unsupported WAV channels %d\n", channels);
      goto CleanUp;
  }

  if (aacEncOpen(&encoder, 0, channels) != AACENC_OK) {
    AAC_CODEC_ERR("Unable to open encoder\n");
    goto CleanUp;
  }

  if (aacEncoder_SetParam(encoder, AACENC_AOT, aot) != AACENC_OK) {
    AAC_CODEC_ERR("Unable to set the AOT\n");
    goto CleanUp;
  }

  if (aacEncoder_SetParam(encoder, AACENC_SAMPLERATE, sample_rate) != AACENC_OK) {
    AAC_CODEC_ERR("Unable to set the sample rate\n");
    goto CleanUp;
  }

  if (aacEncoder_SetParam(encoder, AACENC_CHANNELMODE, mode) != AACENC_OK) {
    AAC_CODEC_ERR("Unable to set the channel mode\n");
    goto CleanUp;
  }

  if (aacEncoder_SetParam(encoder, AACENC_CHANNELORDER, 1) != AACENC_OK) {
    AAC_CODEC_ERR("Unable to set the wav channel order\n");
    goto CleanUp;
  }

  if (vbr != 0) {
    if (aacEncoder_SetParam(encoder, AACENC_BITRATEMODE, vbr) != AACENC_OK) {
      AAC_CODEC_ERR("Unable to set the VBR bitrate mode\n");
      goto CleanUp;
    }
  } else {
    if (aacEncoder_SetParam(encoder, AACENC_BITRATE, bitrate) != AACENC_OK) {
      AAC_CODEC_ERR("Unable to set the bitrate\n");
      goto CleanUp;
    }
  }

  // It is usually TT_MP4_ADTS and TT_MP4_RAW
  if (strcmp(aac_conf->e_transport, "RAW") == 0) {
    transportValue = TT_MP4_RAW;
  }
  if (strcmp(aac_conf->e_transport, "ADTS") == 0) {
    transportValue = TT_MP4_ADTS;
  }
  if (aacEncoder_SetParam(encoder, AACENC_TRANSMUX, transportValue) != AACENC_OK) {
    AAC_CODEC_ERR("Unable to set the ADTS transmux\n");
    goto CleanUp;
    ;
  }

  if (aacEncoder_SetParam(encoder, AACENC_AFTERBURNER, afterburner) != AACENC_OK) {
    AAC_CODEC_ERR("Unable to set the afterburner mode\n");
    goto CleanUp;
    ;
  }

  if (aacEncEncode(encoder, NULL, NULL, NULL, NULL) != AACENC_OK) {
    AAC_CODEC_ERR("Unable to initialize the encoder\n");
    goto CleanUp;
    ;
  }

  if (aacEncInfo(encoder, &info) != AACENC_OK) {
    AAC_CODEC_ERR("Unable to get the encoder info\n");
    goto CleanUp;
  }

  ret = 0;
  AAC_CODEC_DBG("info.maxOutBufBytes = %u, info.frameLength = %u\n", info.maxOutBufBytes, info.frameLength);

  // default bit width is 16, 16 / 8 = 2
  input_size = channels * 2 * info.frameLength;
  AAC_CODEC_DBG("------------------------ inputSize:%d\n", input_size);

CleanUp:
  if (ret != 0) {
    if (encoder != NULL) {
      aacEncClose(&encoder);
    }
  } else {
    encode_info->encoder = encoder;
    encode_info->input_size = input_size;
    encode_info->out_size = info.maxOutBufBytes;
  }

  return ret;
}

void media_aacDeInitEncoder(HANDLE_AACENCODER encoder) { aacEncClose(&encoder); }

int media_aacEncodeFrame(HANDLE_AACENCODER encoder, const uint8_t* in_buffer, int in_size, uint8_t* out_buffer,
                         int out_size, int* out_bytes) {
  if (encoder == NULL || in_buffer == NULL || in_size == 0 || out_buffer == NULL || out_size == 0) {
    return -1;
  }

  AACENC_BufDesc in_buf = {0}, out_buf = {0};
  AACENC_InArgs in_args = {0};
  AACENC_OutArgs out_args = {0};
  int in_identifier = IN_AUDIO_DATA;
  int in_elem_size;
  int out_identifier = OUT_BITSTREAM_DATA;
  int out_elem_size;
  AACENC_ERROR err;

  // 2 bytes per sample point
  in_args.numInSamples = in_size / 2;

  in_elem_size = 2;
  in_buf.numBufs = 1;
  in_buf.bufs = (void**)&in_buffer;
  in_buf.bufferIdentifiers = &in_identifier;
  in_buf.bufSizes = &in_size;
  in_buf.bufElSizes = &in_elem_size;

  out_elem_size = 1;
  out_buf.numBufs = 1;
  out_buf.bufs = (void**)&out_buffer;
  out_buf.bufferIdentifiers = &out_identifier;
  out_buf.bufSizes = &out_size;
  out_buf.bufElSizes = &out_elem_size;

  if ((err = aacEncEncode(encoder, &in_buf, &out_buf, &in_args, &out_args)) != AACENC_OK) {
    AAC_CODEC_ERR("Encoding failed\n");
    return -1;
  }

  *out_bytes = out_args.numOutBytes;
  return 0;

  // printf("out_args.numOutBytes = %d, out_args.numInSamples = %d\n",
  // out_args.numOutBytes, out_args.numInSamples);
}

/*  */
#define MB_AAC_IN_BUFF (2048 * sizeof(INT_PCM))

#define CHANNEL_NUM 1

HANDLE_AACDECODER decoder = NULL;

static int aacDecBusy = 0;

FILE* g_PCMFile = NULL;

int media_aacInitDecoder(aacCodecInfo_s* aac_dec_conf) {
  int ret = 0;
  int aot = AOT_AAC_LC;
  int conceal_method = 2;  // 2;//0 muting 1 noise 2 interpolation
  uint16_t transportValue = 0;
  UCHAR lc_conf[] = {0x14, 0x08};
  UCHAR* conf[] = {lc_conf};
  static UINT conf_len = sizeof(lc_conf);

  if (strcmp(aac_dec_conf->e_transport, "RAW") == 0) {
    transportValue = TT_MP4_RAW;
  }
  if (strcmp(aac_dec_conf->e_transport, "ADTS") == 0) {
    transportValue = TT_MP4_ADTS;
  }

  decoder = aacDecoder_Open(transportValue, 1);
  if (!decoder) {
    AAC_CODEC_ERR("aacDecoder_Open failed\n");
    return -1;
  }

  AAC_DECODER_ERROR err = aacDecoder_ConfigRaw(decoder, conf, &conf_len);
  if (err > 0) {
    AAC_CODEC_ERR("aacDecoder_ConfigRaw failed, err(%#)\n", err);
    return -1;
  }

  aacDecoder_SetParam(decoder, AAC_CONCEAL_METHOD, conceal_method);
  aacDecoder_SetParam(decoder, AAC_PCM_MAX_OUTPUT_CHANNELS, 1);  // MONO:1
  aacDecoder_SetParam(decoder, AAC_PCM_MIN_OUTPUT_CHANNELS, 1);  // MONO:1

  // 获取解码后码流的信息
  CStreamInfo* info = aacDecoder_GetStreamInfo(decoder);
  if (!info) {
    return -1;
  }

  AAC_CODEC_INFO("info->frameSize:%d\n", info->frameSize);
  AAC_CODEC_INFO("info->sampleRate:%d\n", info->sampleRate);
  AAC_CODEC_INFO("info->numTotalBytes:%d\n", info->numTotalBytes);
  AAC_CODEC_INFO("info->aacNumChannels:%d\n", info->aacNumChannels);
  AAC_CODEC_INFO("info->bitRate:%d\n", info->bitRate);
  AAC_CODEC_INFO("info->channelConfig:%d\n", info->channelConfig);
  AAC_CODEC_INFO("info->aacSampleRate:%d\n", info->aacSampleRate);
  AAC_CODEC_INFO("info->numChannels:%d\n", info->numChannels);
  AAC_CODEC_INFO("info->aot:%d\n", info->aot);
  AAC_CODEC_INFO("info->profile:%d\n", info->profile);
  AAC_CODEC_INFO("info->aacSamplesPerFrame:%d\n", info->aacSamplesPerFrame);
  AAC_CODEC_INFO("info->flags:%d\n", info->flags);
  AAC_CODEC_INFO("info->numBadBytes:%d\n", info->numBadBytes);
  AAC_CODEC_INFO("info->numTotalAccessUnits:%d\n", info->numTotalAccessUnits);
  AAC_CODEC_INFO("info->numBadAccessUnits:%d\n", info->numBadAccessUnits);

  return ret;
}

void media_aacDeInitDecoder(void) {
  if (decoder) {
    while (aacDecBusy) {
      usleep(1000 * 20);
    }
    aacDecoder_Close(decoder);
    decoder = NULL;
  }
}

int media_aacDecodeFrame(unsigned char* pEncodedFrame, unsigned int encodedFrameLen, unsigned char* decFrm,
                         int* decFrmsize) {
  unsigned int uBufferSize = MB_AAC_IN_BUFF;
  unsigned int valid = encodedFrameLen;

  unsigned int iEncodedFrameLen = encodedFrameLen;
  AAC_DECODER_ERROR err;

  aacDecBusy = 1;

  if (decoder == NULL) {
    AAC_CODEC_ERR(">>>>>>>>>>> decoder is NULL <<<<<<<<<<<<<<<<\n");
    goto t_exit;
  }

  if (pEncodedFrame == NULL || encodedFrameLen == 0) {
    AAC_CODEC_ERR("input Frame param err\n");
    goto t_exit;
  }

  if (decFrm == NULL || decFrmsize == NULL) {
    AAC_CODEC_ERR("input dest param err\n");
    goto t_exit;
  }

  // AAC_CODEC_DBG("encodedFrameLen:%d\n", encodedFrameLen);
  err = aacDecoder_Fill(decoder, &pEncodedFrame, &iEncodedFrameLen, &valid);
  if (err != AAC_DEC_OK) {
    AAC_CODEC_ERR("aacDecoder_Fill failed :%d\n", err);
    goto t_exit;
  } else {
    // AAC_CODEC_INFO("aacDecoder_Fill succ:%d, valid:%d\n", err, valid);
  }

  err = aacDecoder_DecodeFrame(decoder, (INT_PCM*)decFrm, uBufferSize / sizeof(INT_PCM), 0);
  if (err != AAC_DEC_OK) {
    AAC_CODEC_ERR("aacDecoder_DecodeFrame fail:%d\n", err);
    goto t_exit;
  } else {
    // AAC_CODEC_INFO("aacDecoder_DecodeFrame succ:%d\n", err);
  }

  // 获取解码后码流的信息
  CStreamInfo* info = aacDecoder_GetStreamInfo(decoder);
  if (!info) {
    goto t_exit;
  }

  if (info->frameSize) {
    *decFrmsize = info->frameSize * 2;
  }

  aacDecBusy = 0;
  return info->frameSize;
t_exit:
  aacDecBusy = 0;
  return -1;
}
