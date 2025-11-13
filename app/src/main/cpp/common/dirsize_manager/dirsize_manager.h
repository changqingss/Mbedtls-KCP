/**
 * @file dirsize_manager.h
 * @brief 目录大小管理API头文件
 * @details 提供目录大小限制、文件保存和空间管理的接口
 */

#ifndef DIRSIZE_MANAGER_H
#define DIRSIZE_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

/* 错误码定义 */
#define DM_SUCCESS 0          ///< 操作成功
#define DM_ERROR -1           ///< 通用错误
#define DM_FILE_TOO_LARGE -2  ///< 源文件超过目录大小限制
#define DM_NO_SPACE -3        ///< 无法释放足够空间
#define DM_SRC_FILE_ERROR -4  ///< 源文件访问错误
#define DM_DST_FILE_ERROR -5  ///< 目标文件操作错误

/**
 * @brief 保存文件到目录(带大小限制)
 *
 * @param dir_path 目标目录路径(必须是已存在目录)
 * @param src_path 源文件路径(必须是常规文件)
 * @param max_size_kb 目录最大大小限制(KB)
 * @return int 错误码(DM_SUCCESS表示成功)
 *
 * @note 行为规则:
 * 1. 如果源文件大小超过max_size_kb，直接返回DM_FILE_TOO_LARGE
 * 2. 如果目录当前大小+源文件大小超过限制，按修改时间从旧到新删除文件
 * 3. 只有确保有足够空间后才会开始复制文件
 * 4. 复制失败会自动清理部分写入的文件
 */
int dm_save_file_to_dir(const char *dir_path, const char *src_path, long max_size_kb);

/**
 * @brief 获取目录当前大小
 *
 * @param dir_path 要检查的目录路径
 * @param[out] size_kb 返回目录总大小(KB)
 * @return int 错误码(DM_SUCCESS表示成功)
 *
 * @note 会递归计算目录下所有文件的总大小
 */
int dm_get_dir_size(const char *dir_path, long *size_kb);

/**
 * @brief 清理目录到指定大小
 *
 * @param dir_path 要清理的目录路径
 * @param max_size_kb 目标大小限制(KB)
 * @return int 错误码(DM_SUCCESS表示成功)
 *
 * @note 删除策略:
 * 1. 按文件修改时间从旧到新删除
 * 2. 只删除常规文件(不处理子目录)
 * 3. 如果无法释放到目标大小返回DM_NO_SPACE
 */
int dm_clean_dir_to_size(const char *dir_path, long max_size_kb);

#ifdef __cplusplus
}
#endif

#endif /* DIRSIZE_MANAGER_H */