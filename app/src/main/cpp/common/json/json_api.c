#include "json_api.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*----------STRUCT----------*/
typedef struct {
  void *ptr;
  int buf_size;
} lds_json_string_config_t;

typedef enum {
  LDS_JSON_TYPE_INVALID = 0,
  LDS_JSON_TYPE_BOOLEAN = 1,
  LDS_JSON_TYPE_INT,
  LDS_JSON_TYPE_DOUBLE,
  LDS_JSON_TYPE_STRING,
  LDS_JSON_TYPE_ARRAY,
  LDS_JSON_TYPE_OBJ,
  LDS_JSON_TYPE_MAX
} LDS_JSON_VAL_TYPE_E;

static char *json_cstrtok(char *s, const char *delim, char **save_ptr) {
  char *token;

  if (s == NULL) {
    s = *save_ptr;
  }

  if (s == NULL) {
    return NULL;
  }

  /* Scan leading delimiters.  */
  s += strspn(s, delim);

  if (*s == '\0') {
    return NULL;
  }

  /* Find the end of the token.  */
  token = s;
  s = (char *)strpbrk(token, delim);
  if (s == NULL) {
    /* This token finishes the string.  */
    *save_ptr = (char *)strchr(token, '\0');
  } else {
    /* Terminate the token and make *SAVE_PTR point past it.  */
    *s = '\0';
    *save_ptr = s + 1;
  }

  return token;
}

lds_json lds_json_get_obj(lds_json root, const char *path) {
  char path_copy[XJSON_MAX_PATH_LENGTH];
  char *p, *q;
  const char *delim = ".";
  lds_json obj, container;
  char *save;
  memset(&obj, 0, sizeof(lds_json));
  snprintf(path_copy, sizeof(path_copy), "%s", path);
  p = path_copy;
  container = root;

  if (NULL == path) {
    return container;
  }
  q = json_cstrtok(p, delim, &save);

  while (q) {
    obj = cJSON_GetObjectItem(container, q);

    if (obj) {
      container = obj;
    } else {
      return NULL;
    }

    q = json_cstrtok(NULL, delim, &save);
  }

  return obj;
}

static lds_json XjsonNewParent(lds_json root, const char *path, char *node, signed int bufsize) {
  char *last_delim;
  char path_copy[XJSON_MAX_PATH_LENGTH];
  char *p, *q;
  lds_json obj, parent;
  unsigned char new_object_start = 0;
  char *save;
  snprintf(path_copy, sizeof(path_copy), "%s", path);
  parent = root;
  p = path_copy;
  last_delim = strrchr(path_copy, '.');

  if (last_delim == NULL) {
    snprintf(node, bufsize, "%s", path_copy);
    return parent;
  }

  q = json_cstrtok(p, ".", &save);

  while (q) {
    // last_delim is points to the '.' delimiter,
    // but q points to the node name
    if (q == last_delim + 1) {  // reach to the last node
      snprintf(node, bufsize, "%s", q);
      return parent;
    }

    if (!new_object_start) {
      obj = cJSON_GetObjectItem(parent, q);
      if (obj == NULL) {
        new_object_start = 1;
        obj = cJSON_CreateObject();

        if (obj == NULL) {
          return NULL;
        }

        cJSON_AddItemToObject(parent, q, obj);
      } else if (obj->type != cJSON_Object) {
        return NULL;
      }

      parent = obj;
    } else {
      obj = cJSON_CreateObject();

      if (obj == NULL) {
        return NULL;
      }

      cJSON_AddItemToObject(parent, q, obj);
      parent = obj;
    }

    q = json_cstrtok(NULL, ".", &save);
  }

  return NULL;
}

static bool lds_json_is_valid_type(LDS_JSON_VAL_TYPE_E val_type) {
  if (LDS_JSON_TYPE_MAX <= val_type || LDS_JSON_TYPE_INVALID >= val_type) {
    return cJSON_False;
  }
  return cJSON_True;
}

static bool lds_json_check_val_type(lds_json obj, LDS_JSON_VAL_TYPE_E except_val_type) {
  if (!obj || !lds_json_is_valid_type(except_val_type)) {
    return cJSON_False;
  }

  switch (except_val_type) {
    case LDS_JSON_TYPE_INT:
    case LDS_JSON_TYPE_DOUBLE:
      return (cJSON_IsNumber(obj) || cJSON_IsBool(obj));
    case LDS_JSON_TYPE_STRING:
      return cJSON_IsString(obj);
    case LDS_JSON_TYPE_BOOLEAN:
      return cJSON_IsBool(obj);
    case LDS_JSON_TYPE_ARRAY:
      return cJSON_IsArray(obj);
    case LDS_JSON_TYPE_OBJ:
      return cJSON_IsObject(obj);
    default:
      break;
  }

  return cJSON_False;
}

static LDS_JSON_RET_E set_json_value(lds_json obj, LDS_JSON_VAL_TYPE_E type, const void *val) {
  if (lds_json_check_val_type(obj, type)) {
    switch (type) {
      case LDS_JSON_TYPE_BOOLEAN:
        obj->type = (*(int *)val) ? cJSON_True : cJSON_False;
        obj->valueint = *(int *)val;
        break;
      case LDS_JSON_TYPE_INT:
      case LDS_JSON_TYPE_DOUBLE:
        cJSON_SetNumberValue(obj, (*(double *)val));
        break;
      case LDS_JSON_TYPE_STRING:
        return (cJSON_SetValuestring(obj, (char *)val)) ? LDS_JSON_SUCCESS : LDS_JSON_FAIL;
      case LDS_JSON_TYPE_OBJ:
        obj->child = (lds_json)val;
        break;
      case LDS_JSON_TYPE_ARRAY:
        return LDS_JSON_ERROR_SUPPORT;
      default:
        return LDS_JSON_ERROR_DATA_TYPE;
    }
  } else {
    return LDS_JSON_ERROR_DATA_TYPE;
  }
  return LDS_JSON_SUCCESS;
}

static LDS_JSON_RET_E lds_json_set(lds_json root, const char *path, LDS_JSON_VAL_TYPE_E type, const void *val) {
  if (NULL == root || NULL == path || NULL == val || !lds_json_is_valid_type(type)) {
    return LDS_JSON_ERROR_FUNC_ARGS;
  }

  lds_json object = lds_json_get_obj(root, path);

  if (object) {
    return set_json_value(object, type, val);
  } else {
    char node[XJSON_MAX_PATH_LENGTH] = "\0";
    object = XjsonNewParent(root, path, node, sizeof(node));
    if (!lds_json_check_val_type(object, LDS_JSON_TYPE_OBJ)) {
      return LDS_JSON_FAIL;
    }

    switch (type) {
      case LDS_JSON_TYPE_STRING:
        return (cJSON_AddStringToObject(object, node, ((char *)val))) ? LDS_JSON_SUCCESS : LDS_JSON_FAIL;
      case LDS_JSON_TYPE_INT:
      case LDS_JSON_TYPE_DOUBLE:
        return (cJSON_AddNumberToObject(object, node, *((double *)val))) ? LDS_JSON_SUCCESS : LDS_JSON_FAIL;
      case LDS_JSON_TYPE_OBJ:
        return (cJSON_AddItemToObject(object, node, (lds_json)val)) ? LDS_JSON_SUCCESS : LDS_JSON_FAIL;
      case LDS_JSON_TYPE_BOOLEAN:
        return (cJSON_AddBoolToObject(object, node, *((int *)val))) ? LDS_JSON_SUCCESS : LDS_JSON_FAIL;
      default:
        return LDS_JSON_ERROR_FUNC_ARGS;
    }
  }

  return LDS_JSON_FAIL;
}

static LDS_JSON_RET_E get_json_value(lds_json obj, LDS_JSON_VAL_TYPE_E type, void *val) {
  if (NULL == obj || NULL == val) {
    return LDS_JSON_ERROR_FUNC_ARGS;
  }

  if (lds_json_check_val_type(obj, type)) {
    switch (type) {
      case LDS_JSON_TYPE_BOOLEAN:
      case LDS_JSON_TYPE_INT:
        *(int *)val = obj->valueint;
        break;
      case LDS_JSON_TYPE_DOUBLE:
        *(double *)val = obj->valuedouble;
        break;
      case LDS_JSON_TYPE_STRING:
        snprintf((char *)((lds_json_string_config_t *)val)->ptr, ((lds_json_string_config_t *)val)->buf_size, "%s",
                 obj->valuestring);
        break;
      case LDS_JSON_TYPE_OBJ:
        *(lds_json *)val = obj;
        break;
      case LDS_JSON_TYPE_ARRAY:
        return LDS_JSON_ERROR_SUPPORT;
      default:
        return LDS_JSON_ERROR_DATA_TYPE;
    }
  } else {
    return LDS_JSON_ERROR_DATA_TYPE;
  }
  return LDS_JSON_SUCCESS;
}

static LDS_JSON_RET_E lds_json_get(lds_json root, const char *path, LDS_JSON_VAL_TYPE_E type, void *val) {
  if (NULL == root || NULL == path || NULL == val || !lds_json_is_valid_type(type)) {
    return LDS_JSON_ERROR_FUNC_ARGS;
  }

  lds_json object = lds_json_get_obj(root, path);

  if (object) {
    return get_json_value(object, type, val);
  } else {
    return LDS_JSON_ERROR_EMPTY;
  }
}

LDS_JSON_RET_E lds_json_set_obj(lds_json root, const char *path, lds_json json) {
  return lds_json_set(root, path, LDS_JSON_TYPE_OBJ, json);
}

/**
 * @brief lds_json_get_int
 *
 * @param root      查找的JSON对象
 * @param path      JSON路径
 * @param value     查找到的值
 * @return LDS_JSON_RET_E   LDS_JSON_SUCCESS 成功, LDS_JSON_ERROR_EXIST 查找到非数值, LDS_JSON_ERROR_EMPTY 未找到路径
 * @see
 * @warning
 * @note
 */
LDS_JSON_RET_E lds_json_get_int(lds_json root, const char *path, signed int *value) {
  return lds_json_get(root, path, LDS_JSON_TYPE_INT, value);
}

/**
 * @brief lds_json_set_int
 *
 * @param root      需设置的JSON对象
 * @param path      JSON路径
 * @param value     设置的int值
 * @return LDS_JSON_RET_E   LDS_JSON_SUCCESS 设置成功, LDS_JSON_ERROR_EXIST 已存在非数值, LDS_JSON_FAIL 创建节点失败
 * @see
 * @warning
 * @note
 */
LDS_JSON_RET_E lds_json_set_int(lds_json root, const char *path, signed int value) {
  double dint = (double)value;
  return lds_json_set(root, path, LDS_JSON_TYPE_INT, &dint);
}

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
LDS_JSON_RET_E lds_json_get_bool(lds_json root, const char *path, int *value) {
  return lds_json_get(root, path, LDS_JSON_TYPE_BOOLEAN, value);
}

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
LDS_JSON_RET_E lds_json_set_bool(lds_json root, const char *path, int value) {
  int ibool = (int)value;
  return lds_json_set(root, path, LDS_JSON_TYPE_BOOLEAN, &ibool);
}

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
LDS_JSON_RET_E lds_json_get_double(lds_json root, const char *path, double *value) {
  return lds_json_get(root, path, LDS_JSON_TYPE_DOUBLE, value);
}

/**
 * @brief lds_json_set_double
 *
 * @param root      查找的JSON对象
 * @param path      JSON路径
 * @param value     查找到的值
 * @return LDS_JSON_RET_E   LDS_JSON_SUCCESS 设置成功, LDS_JSON_ERROR_FUNC_ARGS 参数有误, LDS_JSON_ERROR_EXIST
 * 查找到非数值, LDS_JSON_ERROR_EMPTY 未找到路径
 * @see
 * @warning
 * @note
 */
LDS_JSON_RET_E lds_json_set_double(lds_json root, const char *path, double value) {
  return lds_json_set(root, path, LDS_JSON_TYPE_DOUBLE, &value);
}

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
LDS_JSON_RET_E lds_json_get_string(lds_json root, const char *path, char *value, unsigned int bufsize) {
  lds_json_string_config_t config = {
      .ptr = (void *)value,
      .buf_size = bufsize,
  };
  memset(value, 0, bufsize);
  return lds_json_get(root, path, LDS_JSON_TYPE_STRING, (void *)&config);
}

/**
 * @brief lds_json_set_string
 *
 * @param root      查找的JSON对象
 * @param path      JSON路径
 * @param value     查找到的值
 * @return LDS_JSON_RET_E   LDS_JSON_SUCCESS 设置成功, LDS_JSON_ERROR_FUNC_ARGS 参数有误, LDS_JSON_ERROR_EXIST
 * 查找到非数值, LDS_JSON_ERROR_EMPTY 未找到路径
 * @see
 * @warning
 * @note
 */
LDS_JSON_RET_E lds_json_set_string(lds_json root, const char *path, const char *value) {
  return lds_json_set(root, path, LDS_JSON_TYPE_STRING, value);
}

/**
 * @brief lds_json_array_append_bool
 *
 * @param root      需设置的JSON对象
 * @param path      JSON路径
 * @param value     设置的值
 * @return LDS_JSON_RET_E   LDS_JSON_SUCCESS 设置成功, LDS_JSON_ERROR_FUNC_ARGS 参数有误, LDS_JSON_ERROR_EXIST
 * 查找到非数组, LDS_JSON_FAIL 路径创建失败
 * @see
 * @warning
 * @note
 */
LDS_JSON_RET_E lds_json_array_append_bool(lds_json root, const char *path, int value) {
  if (NULL == root || NULL == path) {
    return LDS_JSON_ERROR_FUNC_ARGS;
  }

  char node[XJSON_MAX_PATH_LENGTH];
  lds_json object = lds_json_get_obj(root, path);

  if (object) {
    if (object->type != cJSON_Array) {
      return LDS_JSON_FAIL;
    } else {
      cJSON_AddItemToArray(object, cJSON_CreateBool(value));
      return LDS_JSON_SUCCESS;
    }
  }

  lds_json parent = XjsonNewParent(root, path, node, sizeof(node));

  if (!parent || parent->type != cJSON_Object) {
    return LDS_JSON_FAIL;
  }

  lds_json array = cJSON_CreateArray();

  if (NULL == array) {
    return LDS_JSON_FAIL;
  }

  cJSON_AddItemToArray(array, cJSON_CreateBool(value));
  cJSON_AddItemToObject(parent, node, array);
  return LDS_JSON_SUCCESS;
}

/**
 * @brief lds_json_array_append_int
 *
 * @param root      需设置的JSON对象
 * @param path      JSON路径
 * @param value     设置的值
 * @return LDS_JSON_RET_E   LDS_JSON_SUCCESS 设置成功, LDS_JSON_ERROR_FUNC_ARGS 参数有误, LDS_JSON_ERROR_EXIST
 * 查找到非数组, LDS_JSON_FAIL 路径创建失败
 * @see
 * @warning
 * @note
 */
LDS_JSON_RET_E lds_json_array_append_int(lds_json root, const char *path, signed int value) {
  if (NULL == root || NULL == path) {
    return LDS_JSON_ERROR_FUNC_ARGS;
  }

  char node[XJSON_MAX_PATH_LENGTH];
  lds_json object = lds_json_get_obj(root, path);

  if (object) {
    if (object->type != cJSON_Array) {
      return LDS_JSON_FAIL;
    } else {
      cJSON_AddItemToArray(object, cJSON_CreateNumber(value));
      return LDS_JSON_SUCCESS;
    }
  }

  lds_json parent = XjsonNewParent(root, path, node, sizeof(node));

  if (!parent || parent->type != cJSON_Object) {
    return LDS_JSON_FAIL;
  }

  lds_json array = cJSON_CreateArray();

  if (NULL == array) {
    return LDS_JSON_FAIL;
  }

  cJSON_AddItemToArray(array, cJSON_CreateNumber(value));
  cJSON_AddItemToObject(parent, node, array);
  return LDS_JSON_SUCCESS;
}

/**
 * @brief lds_json_array_get_int_array
 *
 * @param root      查找的JSON对象
 * @param path      JSON路径
 * @param values    可填充获取数据的数组
 * @param max_size  可填充的数组大小
 * @param get_size  填充的数组大小
 * @return LDS_JSON_RET_E   LDS_JSON_SUCCESS 获取成功, LDS_JSON_ERROR_FUNC_ARGS 参数有误, LDS_JSON_ERROR_EXIST
 * 查找到非数组, LDS_JSON_ERROR_EMPTY 未找到路径
 * @see
 * @warning
 * @note
 */
LDS_JSON_RET_E lds_json_array_get_int_array(lds_json root, const char *path, signed int values[], unsigned int max_size,
                                            unsigned int *get_size) {
  if (NULL == root || NULL == path || NULL == values) {
    return LDS_JSON_ERROR_FUNC_ARGS;
  }

  memset((unsigned char *)values, 0, sizeof(values[0]) * max_size);
  lds_json object = lds_json_get_obj(root, path);

  if (object) {
    if (object->type != cJSON_Array) {
      return LDS_JSON_ERROR_DATA_TYPE;
    } else {
      signed int index = 0;
      lds_json cptr = object->child;

      while (cptr && index < max_size) {
        if (cptr->type != cJSON_Number) {
          return LDS_JSON_FAIL;
        }

        values[index] = cptr->valueint;
        cptr = cptr->next;
        index++;
      }
      *get_size = index;
      return LDS_JSON_SUCCESS;
    }
  }

  return LDS_JSON_ERROR_EMPTY;
}

/**
 * @brief lds_json_array_append_double
 *
 * @param root      需设置的JSON对象
 * @param path      JSON路径
 * @param value     设置的值
 * @return LDS_JSON_RET_E   LDS_JSON_SUCCESS 设置成功, LDS_JSON_ERROR_FUNC_ARGS 参数有误, LDS_JSON_ERROR_EXIST
 * 查找到非数组, LDS_JSON_FAIL 路径创建失败
 * @see
 * @warning
 * @note
 */
LDS_JSON_RET_E lds_json_array_append_double(lds_json root, const char *path, double value) {
  if (NULL == root || NULL == path) {
    return LDS_JSON_ERROR_FUNC_ARGS;
  }

  char node[XJSON_MAX_PATH_LENGTH];
  lds_json object = lds_json_get_obj(root, path);

  if (object) {
    if (object->type != cJSON_Array) {
      return LDS_JSON_FAIL;
    } else {
      cJSON_AddItemToArray(object, cJSON_CreateNumber(value));
      return LDS_JSON_SUCCESS;
    }
  }

  lds_json parent = XjsonNewParent(root, path, node, sizeof(node));

  if (!parent || parent->type != cJSON_Object) {
    return LDS_JSON_FAIL;
  }

  lds_json array = cJSON_CreateArray();

  if (NULL == array) {
    return LDS_JSON_FAIL;
  }

  cJSON_AddItemToArray(array, cJSON_CreateNumber(value));
  cJSON_AddItemToObject(parent, node, array);
  return LDS_JSON_SUCCESS;
}

/**
 * @brief lds_json_array_get_double_array
 *
 * @param root      查找的JSON对象
 * @param path      JSON路径
 * @param values    可填充获取数据的数组
 * @param max_size  可填充的数组大小
 * @param get_size  填充的数组大小
 * @return LDS_JSON_RET_E   LDS_JSON_SUCCESS 获取成功, LDS_JSON_ERROR_FUNC_ARGS 参数有误, LDS_JSON_ERROR_EXIST
 * 查找到非数组, LDS_JSON_ERROR_EMPTY 未找到路径
 * @see
 * @warning
 * @note
 */
LDS_JSON_RET_E lds_json_array_get_double_array(lds_json root, const char *path, double values[], unsigned int max_size,
                                               unsigned int *get_size) {
  if (NULL == root || NULL == path || NULL == values) {
    return LDS_JSON_ERROR_FUNC_ARGS;
  }

  memset((unsigned char *)values, 0, sizeof(values[0]) * max_size);
  lds_json object = lds_json_get_obj(root, path);

  if (object) {
    if (object->type != cJSON_Array) {
      return LDS_JSON_FAIL;
    } else {
      signed int index = 0;
      lds_json cptr = object->child;

      while (cptr && index < max_size) {
        if (cptr->type != cJSON_Number) {
          return LDS_JSON_FAIL;
        }

        values[index] = cptr->valuedouble;
        cptr = cptr->next;
        index++;
      }
      *get_size = index;
      return LDS_JSON_SUCCESS;
    }
  }

  return LDS_JSON_FAIL;
}

/**
 * @brief lds_json_array_append_string
 *
 * @param root      需设置的JSON对象
 * @param path      JSON路径
 * @param value     设置的值
 * @param filter    数组内是否存在相同的值，TRUE 为相同值并存， FALSE为仅保留一个值
 * @return LDS_JSON_RET_E   LDS_JSON_SUCCESS 设置成功, LDS_JSON_ERROR_FUNC_ARGS 参数有误, LDS_JSON_ERROR_EXIST
 * 查找到非数组, LDS_JSON_FAIL 路径创建失败
 * @see
 * @warning
 * @note
 */
LDS_JSON_RET_E lds_json_array_append_string(lds_json root, const char *path, const char *value) {
  if (NULL == root || NULL == path) {
    return LDS_JSON_ERROR_FUNC_ARGS;
  }

  char node[XJSON_MAX_PATH_LENGTH];
  lds_json object = lds_json_get_obj(root, path);

  if (object) {
    if (object->type != cJSON_Array) {
      return LDS_JSON_FAIL;
    } else {
      cJSON_AddItemToArray(object, cJSON_CreateString(value));
      return LDS_JSON_SUCCESS;
    }
  }

  lds_json parent = XjsonNewParent(root, path, node, sizeof(node));

  if (!parent || parent->type != cJSON_Object) {
    return LDS_JSON_FAIL;
  }

  lds_json array = cJSON_CreateArray();

  if (NULL == array) {
    return LDS_JSON_FAIL;
  }

  cJSON_AddItemToArray(array, cJSON_CreateString(value));
  cJSON_AddItemToObject(parent, node, array);
  return LDS_JSON_SUCCESS;
}
/**
 * @brief lds_json_array_get_string_array
 *
 * @param root      查找的JSON对象
 * @param path      JSON路径
 * @param values    可填充获取数据的数组
 * @param max_size  可填充的数组大小
 * @param get_size  填充的数组大小
 * @return LDS_JSON_RET_E   LDS_JSON_SUCCESS 获取成功, LDS_JSON_ERROR_FUNC_ARGS 参数有误, LDS_JSON_ERROR_EXIST
 * 查找到非数组, LDS_JSON_ERROR_EMPTY 未找到路径
 * @see
 * @warning
 * @note
 */
LDS_JSON_RET_E lds_json_array_get_string_array(lds_json root, const char *path, char *values[], signed int max_size,
                                               unsigned int *get_size) {
  if (NULL == root || NULL == path || NULL == values) {
    return LDS_JSON_ERROR_FUNC_ARGS;
  }

  lds_json object = lds_json_get_obj(root, path);

  if (object) {
    if (object->type != cJSON_Array) {
      return LDS_JSON_ERROR_EXIST;
    } else {
      signed int index = 0;
      lds_json cptr = object->child;

      while (cptr && index < max_size) {
        if (cptr->type != cJSON_String) {
          return LDS_JSON_ERROR_EXIST;
        }

        values[index] = cptr->valuestring;
        cptr = cptr->next;
        index++;
      }
      *get_size = index;
      return LDS_JSON_SUCCESS;
    }
  }

  return LDS_JSON_FAIL;
}

LDS_JSON_RET_E lds_json_array_append_obj(lds_json root, const char *path, lds_json value) {
  if (NULL == root || NULL == path) {
    return LDS_JSON_ERROR_FUNC_ARGS;
  }

  char node[XJSON_MAX_PATH_LENGTH];
  lds_json object = lds_json_get_obj(root, path);

  if (object) {
    if (object->type != cJSON_Array) {
      return LDS_JSON_FAIL;
    } else {
      cJSON_AddItemToArray(object, value);
      return LDS_JSON_SUCCESS;
    }
  }

  lds_json parent = XjsonNewParent(root, path, node, sizeof(node));

  if (!parent || parent->type != cJSON_Object) {
    return LDS_JSON_FAIL;
  }

  lds_json array = cJSON_CreateArray();

  if (NULL == array) {
    return LDS_JSON_FAIL;
  }

  cJSON_AddItemToArray(array, value);
  cJSON_AddItemToObject(parent, node, array);
  return LDS_JSON_SUCCESS;
}

LDS_JSON_RET_E lds_json_array_get_obj_array(lds_json root, const char *path, lds_json values[], signed int max_size,
                                            unsigned int *get_size) {
  if (NULL == root || NULL == path || NULL == values) {
    return LDS_JSON_ERROR_FUNC_ARGS;
  }

  lds_json object = lds_json_get_obj(root, path);

  if (object) {
    if (object->type != cJSON_Array) {
      return LDS_JSON_FAIL;
    } else {
      signed int index = 0;
      lds_json cptr = object->child;

      while (cptr && index < max_size) {
        if (cptr->type != cJSON_Object) {
          return LDS_JSON_FAIL;
        }

        values[index] = cptr;
        cptr = cptr->next;
        index++;
      }
      *get_size = index;
      return LDS_JSON_SUCCESS;
    }
  }

  return LDS_JSON_FAIL;
}

LDS_JSON_RET_E lds_json_get_int_from_array(lds_json array, int index, int *value) {
  if (NULL == array || index < 0 || NULL == value || cJSON_Array != array->type) {
    return LDS_JSON_ERROR_FUNC_ARGS;
  }

  lds_json item = cJSON_GetArrayItem(array, index);
  if (NULL == item || !cJSON_IsNumber(item)) {
    return LDS_JSON_FAIL;
  }

  *value = item->valueint;
  return LDS_JSON_SUCCESS;
}

LDS_JSON_RET_E lds_json_get_string_from_array(lds_json array, int index, char *value, unsigned int bufsize) {
  if (NULL == array || index < 0 || NULL == value || cJSON_Array != array->type) {
    return LDS_JSON_ERROR_FUNC_ARGS;
  }

  lds_json item = cJSON_GetArrayItem(array, index);
  if (NULL == item || !cJSON_IsString(item)) {
    return LDS_JSON_FAIL;
  }

  memset(value, 0, bufsize);
  strncpy(value, item->valuestring, bufsize);
  return LDS_JSON_SUCCESS;
}

void lds_json_print(lds_json root) {
  char *print_str = cJSON_Print(root);
  printf("json: \n%s\n", print_str);
  free(print_str);
  return;
}