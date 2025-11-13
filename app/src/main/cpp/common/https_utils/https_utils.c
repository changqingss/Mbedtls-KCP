#include "https_utils.h"

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#ifdef AD102P
#include <uuid/uuid.h>
#endif

#include "../log/log_conf.h"
#include "cjson/cJSON.h"
#include "common/param_config/dev_param.h"
#include "common/wifi/wifi.h"
#include "curl/curl.h"
#include "mbedtls/md.h"
#include "mbedtls/sha256.h"

struct buffer {
  char *memory;
  size_t size;
};

// Helper function to convert binary data to a hexadecimal string
void https_utils_curl_2hex(const unsigned char *input, size_t len, char *output) {
  for (size_t i = 0; i < len; i++) {
    sprintf(output + (i * 2), "%02hhx", input[i]);
  }
}

void https_utils_printHex(const uint8_t *data, uint16_t len) {
  for (uint16_t i = 0; i < len; i++) {
    printf("%02hhx ", data[i]);
  }
  printf("\n");
}

// Function to generate the signature
char *https_utils_generate_signature(const char *body, const unsigned char *secret) {
  // Hex(HMAC-SHA256(secret, timestamp + "|" + nonce), secret
  char stringToSign[1024] = {0};
  snprintf(stringToSign, sizeof(stringToSign), "%s", body);

  // printf("stringToSign:\n%s\n",stringToSign);

  // Step 4: Calculate HMAC-SHA256 of the stringToSign using the secret
  // signature = Hex(HMAC-SHA256(secret, stringToSign)
  unsigned char hmac_output[32] = {0};
  char *hex_hmac_output = malloc(65);
  PARAM_CHECK_STRING(hex_hmac_output, NULL, "Memory allocation failed");
  memset(hex_hmac_output, 0, 65);

  mbedtls_md_context_t hmac_ctx;
  mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;

  mbedtls_md_init(&hmac_ctx);
  mbedtls_md_setup(&hmac_ctx, mbedtls_md_info_from_type(md_type), 1);
  mbedtls_md_hmac_starts(&hmac_ctx, (const unsigned char *)secret, MAX_LOCAL_KEY_SIZE);
  mbedtls_md_hmac_update(&hmac_ctx, (const unsigned char *)stringToSign, strlen(stringToSign));
  mbedtls_md_hmac_finish(&hmac_ctx, hmac_output);
  mbedtls_md_free(&hmac_ctx);

  https_utils_printHex(secret, MAX_LOCAL_KEY_SIZE);
  https_utils_printHex(hmac_output, 32);

  // Step 5: Convert the HMAC result to a hexadecimal string
  https_utils_curl_2hex(hmac_output, 32, hex_hmac_output);

  // printf("hex_hmac_output:%s\n",hex_hmac_output);

  return hex_hmac_output;
}

unsigned char *https_utils_curl_get_local_key() {
  static unsigned char secret[MAX_LOCAL_KEY_SIZE] = {0};
  static int inited = 0;
  if (inited) return secret;
  unsigned char *secret_key = NULL;
  int secret_key_len = 0;
  if (!COMM_API_ReadFileContent(SECRET_KEY_FILE, (char **)&secret_key, &secret_key_len)) {
    char *payloadHexString = COMM_API_PayloadToHexString(secret_key, secret_key_len);
    if (payloadHexString) {
      COMMONLOG_I("%s key:%s", SECRET_KEY_FILE, payloadHexString);
      free(payloadHexString);
      memcpy(secret, secret_key, MAX_LOCAL_KEY_SIZE);
      free(secret_key);
      inited = 1;
    }
  } else {
    COMMONLOG_I("Read secret key failed");
    return NULL;
  }
  return secret;
}

int https_utils_curl_print_header(struct curl_slist *slist) {
  struct curl_slist *temp = slist;
  while (temp) {
    COMMONLOG_I("HTTP Header: %s", temp->data);
    temp = temp->next;
  }
}

size_t https_utils_write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
  size_t realsize = size * nmemb;
  struct buffer *mem = (struct buffer *)userp;

  char *ptr = realloc(mem->memory, mem->size + realsize + 1);
  if (ptr == NULL) {
    /* out of memory! */
    COMMONLOG_E("not enough memory (realloc returned NULL)\n");
    return 0;
  }

  mem->memory = ptr;
  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}
// 添加生成随机 nonce 的函数
char *https_utils_generate_nonce() {
  char *nonce = malloc(37);  // UUID格式: 36字符 + 1个null终止符
  if (!nonce) return NULL;

#ifndef AD102P
  // 生成简单的随机UUID格式字符串
  srand(time(NULL));
  snprintf(nonce, 37, "%08x-%04x-%04x-%04x-%012lx", rand(), rand() & 0xFFFF, rand() & 0xFFFF, rand() & 0xFFFF,
           (long)rand() * rand());
#else
  uuid_t uuid;

  // 生成UUID
  uuid_generate(uuid);

  // 将UUID转换为字符串表示形式
  uuid_unparse_lower(uuid, nonce);
#endif

  // 打印生成的UUID
  COMMONLOG_I("Generated UUID: %s\n", nonce);
  return nonce;
}

int get_https_token(struct https_param_t *param, struct https_token_t *token) {
  CURL *curl = NULL;
  CURLcode res = -1;
  struct buffer chunk;
  chunk.memory = malloc(1);
  chunk.size = 0;

  // 创建一个CURL句柄
  curl = curl_easy_init();
  if (curl) {
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);

    // 设置请求的URL
    // 'https://wonderlabs.us.api.switchbot.net/device/v1/auth'
    curl_easy_setopt(curl, CURLOPT_URL, param->url);

    // 设置为POST请求
    curl_easy_setopt(curl, CURLOPT_POST, 1L);

    // 设置请求头
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // 构建POST数据
    cJSON *json = cJSON_CreateObject();
    cJSON *deviceID = cJSON_CreateString(param->mac);

    // 获取当前时间戳（毫秒）
    struct timeval tv;
    gettimeofday(&tv, NULL);
    long long timestamp_ms = (long long)tv.tv_sec * 1000 + tv.tv_usec / 1000;
    param->timestamp = timestamp_ms;
    cJSON *timestamp = cJSON_CreateNumber(timestamp_ms);

    // 生成随机nonce
    char *nonce_str = https_utils_generate_nonce();
    cJSON *nonce = cJSON_CreateString(nonce_str);

    // 生成签名（使用现有的签名生成逻辑，但需要适配新的格式）
    char timestamp_str[32];
    snprintf(timestamp_str, sizeof(timestamp_str), "%lld", timestamp_ms);

    // 构建用于签名的字符串（根据实际API要求调整）
    char sign_data[512];
    snprintf(sign_data, sizeof(sign_data), "%lld%s%s", timestamp_ms, "|", nonce_str);

    // 这里需要根据实际的签名算法调整
    char *signature = https_utils_generate_signature(sign_data, param->secret);
    cJSON *signature_json = cJSON_CreateString(signature);

    // 添加到JSON对象
    cJSON_AddItemToObject(json, "deviceID", deviceID);
    cJSON_AddItemToObject(json, "timestamp", timestamp);
    cJSON_AddItemToObject(json, "nonce", nonce);
    cJSON_AddItemToObject(json, "signature", signature_json);

    // 转换为字符串
    char *json_string = cJSON_Print(json);

    COMMONLOG_I("POST data: %s", json_string);

    // 设置POST数据
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_string);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(json_string));

    // 设置回调函数
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, https_utils_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);

    // 执行请求
    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
      COMMONLOG_E("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    } else {
      COMMONLOG_I("auth response bufsize:%d buf:%.*s", chunk.size, chunk.size, chunk.memory);
      long response_code = -1;
      curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
      COMMONLOG_I("response code: %ld", response_code);

      if (response_code == 200) {
        cJSON *root = cJSON_ParseWithLength(chunk.memory, chunk.size);
        if (root != NULL) {
          // 处理认证响应
          cJSON *resultCode = cJSON_GetObjectItem(root, "resultCode");
          cJSON *data = cJSON_GetObjectItem(root, "data");
          if (resultCode != NULL) {
            COMMONLOG_I("Auth result code: %d", resultCode->valueint);
            if (resultCode->valueint == 100) {
              COMMONLOG_I("Authentication successful");
              cJSON *token_json = cJSON_GetObjectItem(data, "token");
              cJSON *expiresIn_json = cJSON_GetObjectItem(data, "expiresIn");
              if (token_json != NULL) {
                int token_length = strlen(token_json->valuestring);
                token->token = malloc(token_length + 1);
                if (token->token != NULL) {
                  strncpy(token->token, token_json->valuestring, token_length);
                  token->token[token_length] = '\0';  // 确保字符串以null结尾
                  COMMONLOG_I("Token: %s", token->token);
                } else {
                  COMMONLOG_E("Failed to allocate memory for token");
                  res = -1;
                }
              } else {
                COMMONLOG_E("Token not found or not a string");
                res = -1;
              }

              if (expiresIn_json != NULL) {
                long long expires_in = expiresIn_json->valuedouble;
                COMMONLOG_I("Expires In: %lld seconds", expires_in);
                token->expiresIn = (int)expires_in;
                token->timestamp = timestamp_ms;  // 设置当前时间戳
                                                  // 这里可以处理过期时间，例如存储到param中
              } else {
                COMMONLOG_E("ExpiresIn not found or not a number");
              }
            }
          }
          cJSON_Delete(root);
        }
      } else {
        COMMONLOG_E("Auth failed with response code: %ld", response_code);
      }
    }

    // 清理
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    cJSON_Delete(json);
    free(json_string);
    free(signature);
    free(nonce_str);
  }

  free(chunk.memory);
  return res;
}

int get_https_data(struct https_param_t *param, struct https_token_t *token, struct https_data_t *data) {
  CURL *curl = NULL;
  CURLcode res = -1;
  struct buffer chunk;
  chunk.memory = malloc(1);
  chunk.size = 0;

  // 创建一个CURL句柄
  curl = curl_easy_init();
  if (curl) {
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);

    // 设置请求的URL - 使用固定的 Matter 设备 API 端点
    // "https://wonderlabs.us.api.woankeji.cn/device/v1/device/matter"
    curl_easy_setopt(curl, CURLOPT_URL, param->url);

    // 设置为GET请求
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);

    // 设置请求头
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    // 添加Authorization头部，使用Bearer token
    char auth_header[1024];
    snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", token->token);
    headers = curl_slist_append(headers, auth_header);

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // 设置回调函数
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, https_utils_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);

    // 执行请求
    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
      COMMONLOG_E("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    } else {
      COMMONLOG_I("matter daccert response bufsize:%d buf:%.*s", chunk.size, chunk.size, chunk.memory);
      long response_code = -1;
      curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
      COMMONLOG_I("response code: %ld", response_code);

      if (response_code == 200) {
        cJSON *root = cJSON_ParseWithLength(chunk.memory, chunk.size);
        if (root != NULL) {
          cJSON *resultCode = cJSON_GetObjectItem(root, "resultCode");
          cJSON *data_json = cJSON_GetObjectItem(root, "data");

          if (resultCode != NULL && cJSON_IsNumber(resultCode)) {
            COMMONLOG_I("Matter DACCERT result code: %d", resultCode->valueint);

            if (resultCode->valueint == 100 && data_json != NULL) {
              COMMONLOG_I("Matter DACCERT retrieval successful");
              char *data_string = cJSON_Print(data_json);

              if (data_string != NULL) {
                int data_length = strlen(data_string);
                data->data = malloc(data_length + 1);
                if (data->data != NULL) {
                  strncpy(data->data, data_string, data_length);
                  data->data[data_length] = '\0';  // 确保字符串以 null 结尾
                  COMMONLOG_I("Matter DACCERT data: %s", data->data);
                  free(data_string);
                } else {
                  COMMONLOG_E("Failed to allocate memory for Matter DACCERT data");
                  res = CURLE_OUT_OF_MEMORY;
                }
              } else {
                COMMONLOG_E("Matter DACCERT data not found or is empty");
                res = CURLE_HTTP_RETURNED_ERROR;
              }
            } else {
              COMMONLOG_E("Matter DACCERT request failed with result code: %d", resultCode->valueint);
              res = CURLE_HTTP_RETURNED_ERROR;
            }
          } else {
            COMMONLOG_E("ResultCode not found or invalid in response");
            res = CURLE_HTTP_RETURNED_ERROR;
          }
          cJSON_Delete(root);
        } else {
          COMMONLOG_E("Failed to parse JSON response");
          res = CURLE_HTTP_RETURNED_ERROR;
        }
      } else {
        COMMONLOG_E("Matter DACCERT retrieval failed with response code: %ld", response_code);
        res = CURLE_HTTP_RETURNED_ERROR;
      }
    }

    // 清理
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
  } else {
    COMMONLOG_E("Failed to initialize CURL");
    res = CURLE_FAILED_INIT;
  }

  free(chunk.memory);
  return res;
}

int https_utils_read_region(char *region, int region_size, bool *isOnline) {
  int ret = -1;

  FILE *fp = fopen(KVS_PROVIDER_FILE, "rb");
  if (fp == NULL) {
    COMMONLOG_E("fopen:%s(%d)", KVS_PROVIDER_FILE, errno);
    return -1;
  }

  fseek(fp, 0, SEEK_END);
  int len = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  char *pContent = (char *)malloc(len + 1);
  if (pContent == NULL) {
    fclose(fp);
    goto free_malloc;
  }

  int read_ret = fread(pContent, 1, len, fp);
  if (read_ret != len) {
    COMMONLOG_E("fread failed: expected %d bytes, got %d bytes", len, read_ret);
    goto free_malloc;
  }
  pContent[len] = '\0';

  fclose(fp);

  cJSON *root = cJSON_Parse(pContent);
  if (root == NULL) {
    goto free_malloc;
  }

  cJSON *region_json = cJSON_GetObjectItem(root, "region");
  if (region_json != NULL) {
    if (cJSON_IsString(region_json)) {
      COMMONLOG_I("Region: %s", region_json->valuestring);
    } else {
      COMMONLOG_E("Region value is not a string");
      goto delete_json;
    }
  } else {
    COMMONLOG_E("Region not found in JSON");
    goto delete_json;
  }

  cJSON *env_json = cJSON_GetObjectItem(root, "env");
  if (env_json != NULL) {
    if (cJSON_IsString(env_json)) {
      COMMONLOG_I("Env: %s", env_json->valuestring);
    } else {
      COMMONLOG_E("Env value is not a string");
      goto delete_json;
    }
  } else {
    COMMONLOG_E("Env not found in JSON");
    goto delete_json;
  }

  snprintf(region, region_size, "%s", region_json->valuestring);
  *isOnline = (strncmp(env_json->valuestring, "test", 4) != 0);

delete_json:
  cJSON_Delete(root);
free_malloc:
  free(pContent);
  return ret;
}

int https_utils_read_param(struct https_param_t *param) {
  int ret = -1;

  FILE *fp = fopen(KVS_PROVIDER_FILE, "rb");
  if (fp == NULL) {
    COMMONLOG_E("fopen:%s(%d)", KVS_PROVIDER_FILE, errno);
    return -1;
  }

  fseek(fp, 0, SEEK_END);
  int len = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  char *pContent = (char *)malloc(len + 1);
  if (pContent == NULL) {
    fclose(fp);
    goto free_malloc;
  }

  int read_ret = fread(pContent, 1, len, fp);
  if (read_ret != len) {
    COMMONLOG_E("fread failed: expected %d bytes, got %d bytes", len, read_ret);
    goto free_malloc;
  }
  pContent[len] = '\0';

  fclose(fp);

  cJSON *root = cJSON_Parse(pContent);
  if (root == NULL) {
    goto free_malloc;
  }

  cJSON *region_json = cJSON_GetObjectItem(root, "region");
  if (region_json != NULL) {
    if (cJSON_IsString(region_json)) {
      COMMONLOG_I("Region: %s", region_json->valuestring);
    } else {
      COMMONLOG_E("Region value is not a string");
      goto delete_json;
    }
  } else {
    COMMONLOG_E("Region not found in JSON");
    goto delete_json;
  }

  cJSON *env_json = cJSON_GetObjectItem(root, "env");
  if (env_json != NULL) {
    if (cJSON_IsString(env_json)) {
      COMMONLOG_I("Env: %s", env_json->valuestring);
    } else {
      COMMONLOG_E("Env value is not a string");
      goto delete_json;
    }
  } else {
    COMMONLOG_E("Env not found in JSON");
    goto delete_json;
  }

  char *url_suffix = NULL;

  if (strncmp(env_json->valuestring, "test", 4) == 0) {
    url_suffix = HTTPS_OFFLINE_URL_SUFFIX;
  } else {
    url_suffix = HTTPS_ONLINE_URL_SUFFIX;
  }

  sprintf(param->url, "%s%s%s", HTTPS_URL_PREFIX, region_json->valuestring, url_suffix);
  COMMONLOG_I("URL: %s", param->url);

  WIFI_GetBleMac(param->mac);

  unsigned char *secret = https_utils_curl_get_local_key();
  if (secret) {
    memcpy(param->secret, secret, MAX_LOCAL_KEY_SIZE);
    ret = 0;
  }

delete_json:
  cJSON_Delete(root);
free_malloc:
  free(pContent);
  return ret;
}
