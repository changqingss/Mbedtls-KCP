
#ifndef __JSON_API_H__
#define __JSON_API_H__

#include <stdbool.h>

#include "cJSON.h"

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#define XJSON_MAX_PATH_LENGTH 128  /// max path length 128byte

typedef cJSON *lds_json;

typedef enum {
  LDS_JSON_ERROR_DATA_TYPE = -8,  ///< data type NOT match
  LDS_JSON_ERROR_VALUE = -7,      ///< Wrong value returned
  LDS_JSON_ERROR_EMPTY = -6,      ///< The path NOT found
  LDS_JSON_ERROR_SUPPORT = -5,    ///< Not support this feature
  LDS_JSON_ERROR_MEM = -4,        ///< Insufficient memory, failed to allocate
  LDS_JSON_ERROR_EXIST = -3,      ///< Already exists
  LDS_JSON_ERROR_FUNC_ARGS = -2,  ///< Parameter error
  LDS_JSON_FAIL = -1,             ///< Other errors
  LDS_JSON_SUCCESS = 0            ///< OK
} LDS_JSON_RET_E;

// 创建JSON对象
#define lds_json_create_object() cJSON_CreateObject()
// 创建JSON数组
#define lds_json_create_array() cJSON_CreateArray()

// 删除JSON对象
#define lds_json_delete_object(obj) cJSON_Delete(obj)

// 转换JSON对象为字符串
#define lds_json_to_string(obj) cJSON_PrintUnformatted(obj)

// JSON字符串解析为JSON对象
#define lds_json_from_string(str) cJSON_Parse(str)

// 复制JSON对象
#define lds_json_duplicate(obj, recurse) cJSON_Duplicate(obj, recurse)

// JSON对象：键值对删除
#define lds_json_delete_item(obj, key) cJSON_DeleteItemFromObject(obj, key)
// JSON对象：分离**出item，分离后此item需要自行调用lds_json_delete_object()释放
#define lds_json_detach_item(obj, key) cJSON_DetachItemFromObject(obj, key)

// 打印JSON obj为字符串
void lds_json_print(lds_json root);

/**
 * @brief lds_json_set_bool
 *
 * @param root     需设置的 JSON 对象
 * @param path     JSON 路径
 * @param value    设置的 bool 值
 * @return LDS_JSON_RET_E
 * @see
 * @warning
 * @note
 */
LDS_JSON_RET_E lds_json_set_bool(lds_json root, const char *path, int value);
/**
 * @brief lds_json_get_bool
 *
 * @param root      所查找的 JSON 对象
 * @param path      JSON 路径
 * @param value     获取的 bool 值
 * @return LDS_JSON_RET_E   LDS_JSON_SUCCESS 成功, LDS_JSON_ERROR_FUNC_ARGS 参数有误, LDS_JSON_ERROR_EXIST 查找到非数值,
 * LDS_JSON_ERROR_EMPTY 未找到路径
 * @see
 * @warning
 * @note
 */
LDS_JSON_RET_E lds_json_get_bool(lds_json root, const char *path, int *value);

/**
 * @brief lds_json_get_int
 *
 * @param root      需设置的JSON对象
 * @param path      JSON路径
 * @param value     查找到的值
 * @return LDS_JSON_RET_E   LDS_JSON_SUCCESS 设置成功, LDS_JSON_ERROR_FUNC_ARGS 参数有误, LDS_JSON_ERROR_EXIST
 * 查找到非数值, LDS_JSON_FAIL 创建路径失败
 * @see
 * @warning
 * @note
 */
LDS_JSON_RET_E lds_json_set_int(lds_json root, const char *path, signed int value);
/**
 * @brief lds_json_set_int
 *
 * @param root      所查找的JSON对象
 * @param path      JSON路径
 * @param value     设置的int值
 * @return LDS_JSON_RET_E   LDS_JSON_SUCCESS 成功, LDS_JSON_ERROR_FUNC_ARGS 参数有误, LDS_JSON_ERROR_EXIST 查找到非数值,
 * LDS_JSON_ERROR_EMPTY 未找到路径
 * @see
 * @warning
 * @note
 */
LDS_JSON_RET_E lds_json_get_int(lds_json root, const char *path, signed int *value);

/**
 * @brief lds_json_get_double
 *
 * @param root      需设置的JSON对象
 * @param path      JSON路径
 * @param value     查找到的值
 * @return LDS_JSON_RET_E   LDS_JSON_SUCCESS 设置成功, LDS_JSON_ERROR_FUNC_ARGS 参数有误, LDS_JSON_ERROR_EXIST
 * 查找到非数值, LDS_JSON_FAIL 创建路径失败
 * @see
 * @warning
 * @note
 */
LDS_JSON_RET_E lds_json_set_double(lds_json root, const char *path, double value);
/**
 * @brief lds_json_get_double
 *
 * @param root      查找的JSON对象
 * @param path      JSON路径
 * @param value     查找到的值
 * @return LDS_JSON_RET_E   LDS_JSON_SUCCESS 成功, LDS_JSON_ERROR_FUNC_ARGS 参数有误, LDS_JSON_ERROR_EXIST 查找到非数值,
 * LDS_JSON_ERROR_EMPTY 未找到路径
 * @see
 * @warning
 * @note
 */
LDS_JSON_RET_E lds_json_get_double(lds_json root, const char *path, double *value);

/**
 * @brief lds_json_set_string
 *
 * @param root      需设置的JSON对象
 * @param path      JSON路径
 * @param value     查找到的值
 * @return LDS_JSON_RET_E   LDS_JSON_SUCCESS 设置成功, LDS_JSON_ERROR_FUNC_ARGS 参数有误, LDS_JSON_ERROR_EXIST
 * 查找到非数值, LDS_JSON_ERROR_EMPTY 未找到路径
 * @see
 * @warning
 * @note
 */
LDS_JSON_RET_E lds_json_set_string(lds_json root, const char *path, const char *value);
/**
 * @brief lds_json_get_string
 *
 * @param root      查找的JSON对象
 * @param path      JSON路径
 * @param value     查找到的值
 * @return LDS_JSON_RET_E   LDS_JSON_SUCCESS 成功, LDS_JSON_ERROR_FUNC_ARGS 参数有误, LDS_JSON_ERROR_EXIST 查找到非数值,
 * LDS_JSON_ERROR_EMPTY 未找到路径
 * @see
 * @warning
 * @note
 */
LDS_JSON_RET_E lds_json_get_string(lds_json root, const char *path, char *value, unsigned int bufsize);

// 数组操作
// 获取数组大小
#define lds_json_get_array_size(obj) cJSON_GetArraySize(obj)
// 获取数组内指定对象
#define lds_json_get_array_item(obj, index) cJSON_GetArrayItem(obj, index)
// JSON对象：添加json item至array对象
#define lds_json_add_item_to_array(array, item) cJSON_AddItemToArray(array, item)

LDS_JSON_RET_E lds_json_array_append_bool(lds_json root, const char *path, int value);

LDS_JSON_RET_E lds_json_array_append_int(lds_json root, const char *path, signed int value);
LDS_JSON_RET_E lds_json_array_get_int_array(lds_json root, const char *path, signed int values[], unsigned int max_size,
                                            unsigned int *get_size);
LDS_JSON_RET_E lds_json_array_append_double(lds_json root, const char *path, double value);
LDS_JSON_RET_E lds_json_array_get_double_array(lds_json root, const char *path, double values[], unsigned int max_size,
                                               unsigned int *get_size);

LDS_JSON_RET_E lds_json_array_append_string(lds_json root, const char *path, const char *value);
LDS_JSON_RET_E lds_json_array_get_string_array(lds_json root, const char *path, char *values[], signed int max_size,
                                               unsigned int *get_size);

LDS_JSON_RET_E lds_json_array_append_obj(lds_json root, const char *path, lds_json value);
LDS_JSON_RET_E lds_json_array_get_obj_array(lds_json root, const char *path, lds_json values[], signed int max_size,
                                            unsigned int *get_size);

/**
 * @brief Get the last key name in the path
 * eg. json: { "payload": {"attr": 123} }, path = payload.attr
 * @param root
 * @param path
 * @return lds_json
 * @see
 * @warning
 * @note
 */
lds_json lds_json_get_obj(lds_json root, const char *path);

/**
 * @brief Add the corresponding json object in the path under root
 * eg. json: { "payload": {"attr": 123} }
 *     path: payload.time
 *     target json: { "payload": {"attr": 123, "time": 123456} }
 * @param root
 * @param path
 * @param json
 * @return signed int
 * @see
 * @warning
 * @note
 */
LDS_JSON_RET_E lds_json_set_obj(lds_json root, const char *path, lds_json json);

/**
 * @brief get int value from array by index
 *
 * @param array json array
 * @param index
 * @param value[out]
 * @return LDS_JSON_RET_E LDS_JSON_SUCCESS success, others failure
 * @see
 * @warning
 * @note
 */
LDS_JSON_RET_E lds_json_get_int_from_array(lds_json array, int index, int *value);

/**
 * @brief get string value from array by index
 *
 * @param array
 * @param index
 * @param value[out]
 * @param bufsize
 * @return LDS_JSON_RET_E LDS_JSON_SUCCESS success, others failure
 * @see
 * @warning
 * @note
 */
LDS_JSON_RET_E lds_json_get_string_from_array(lds_json array, int index, char *value, unsigned int bufsize);

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif  // __XJSON_H__
