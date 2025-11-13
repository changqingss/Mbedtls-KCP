#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

// 错误码定义
#define DM_SUCCESS 0
#define DM_ERROR -1
#define DM_FILE_TOO_LARGE -2
#define DM_NO_SPACE -3
#define DM_SRC_FILE_ERROR -4
#define DM_DST_FILE_ERROR -5

// 文件信息结构体
typedef struct {
  char *path;
  long size;
  time_t mtime;
} FileInfo;

// 动态数组结构
typedef struct {
  FileInfo *files;
  size_t count;
  size_t capacity;
  long total_size;
} FileList;

// 初始化文件列表
static void init_file_list(FileList *list) {
  list->files = NULL;
  list->count = 0;
  list->capacity = 0;
  list->total_size = 0;
}

// 添加文件到列表
static void add_to_file_list(FileList *list, const char *path, long size, time_t mtime) {
  if (list->count >= list->capacity) {
    list->capacity = list->capacity ? list->capacity * 2 : 16;
    list->files = realloc(list->files, list->capacity * sizeof(FileInfo));
  }

  list->files[list->count].path = strdup(path);
  list->files[list->count].size = size;
  list->files[list->count].mtime = mtime;
  list->count++;
  list->total_size += size;
}

// 释放文件列表内存
static void free_file_list(FileList *list) {
  for (size_t i = 0; i < list->count; i++) {
    free(list->files[i].path);
  }
  free(list->files);
}

// 比较函数，按修改时间排序(从旧到新)
static int compare_files_by_age(const void *a, const void *b) {
  const FileInfo *fa = (const FileInfo *)a;
  const FileInfo *fb = (const FileInfo *)b;
  return (fa->mtime > fb->mtime) - (fa->mtime < fb->mtime);
}

// 递归收集文件信息
static int collect_files(const char *path, FileList *list) {
  struct stat statbuf;
  if (lstat(path, &statbuf)) {
    fprintf(stderr, "无法访问 %s: %s\n", path, strerror(errno));
    return DM_ERROR;
  }

  if (S_ISREG(statbuf.st_mode)) {
    add_to_file_list(list, path, statbuf.st_size, statbuf.st_mtime);
    return DM_SUCCESS;
  }

  if (S_ISDIR(statbuf.st_mode)) {
    DIR *dir = opendir(path);
    if (!dir) {
      fprintf(stderr, "无法打开目录 %s: %s\n", path, strerror(errno));
      return DM_ERROR;
    }

    struct dirent *entry;
    while ((entry = readdir(dir))) {
      if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
        continue;
      }

      char full_path[PATH_MAX];
      snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
      if (collect_files(full_path, list) != DM_SUCCESS) {
        closedir(dir);
        return DM_ERROR;
      }
    }
    closedir(dir);
  }

  return DM_SUCCESS;
}

// 删除最旧的文件直到有足够空间
static int free_up_space(FileList *list, long required_space) {
  // 按修改时间排序(从旧到新)
  qsort(list->files, list->count, sizeof(FileInfo), compare_files_by_age);

  size_t i = 0;
  while (list->total_size > required_space && i < list->count) {
    printf("删除 %s (大小: %.2f KB, 修改时间: %s)", list->files[i].path, (double)list->files[i].size / 1024,
           ctime(&list->files[i].mtime));

    if (unlink(list->files[i].path) == 0) {
      list->total_size -= list->files[i].size;
      printf(" - 成功\n");
    } else {
      fprintf(stderr, " - 失败: %s\n", strerror(errno));
      return DM_ERROR;
    }
    i++;
  }

  if (list->total_size > required_space) {
    fprintf(stderr, "错误: 无法释放足够空间\n");
    return DM_NO_SPACE;
  }

  return DM_SUCCESS;
}

// 复制文件
static int copy_file(const char *src_path, const char *dst_path) {
  int src_fd = open(src_path, O_RDONLY);
  if (src_fd < 0) {
    perror("打开源文件失败");
    return DM_SRC_FILE_ERROR;
  }

  int dst_fd = open(dst_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (dst_fd < 0) {
    perror("创建目标文件失败");
    close(src_fd);
    return DM_DST_FILE_ERROR;
  }

  char buffer[4096];
  ssize_t bytes_read, bytes_written;
  int result = DM_SUCCESS;

  while ((bytes_read = read(src_fd, buffer, sizeof(buffer))) > 0) {
    bytes_written = write(dst_fd, buffer, bytes_read);
    if (bytes_written != bytes_read) {
      perror("写入文件失败");
      result = DM_DST_FILE_ERROR;
      break;
    }
  }

  if (bytes_read < 0) {
    perror("读取文件失败");
    result = DM_SRC_FILE_ERROR;
  }

  close(src_fd);
  close(dst_fd);

  if (result != DM_SUCCESS) {
    unlink(dst_path);  // 删除部分写入的文件
  }

  return result;
}

/**
 * API: 保存文件到目录(带大小限制)
 *
 * @param dir_path 目标目录路径
 * @param src_path 源文件路径
 * @param max_size_kb 目录最大大小(KB)
 * @return 错误码 (DM_SUCCESS 表示成功)
 */
int dm_save_file_to_dir(const char *dir_path, const char *src_path, long max_size_kb) {
  // 检查源文件
  struct stat src_stat;
  if (stat(src_path, &src_stat)) {
    fprintf(stderr, "无法访问源文件 %s: %s\n", src_path, strerror(errno));
    return DM_SRC_FILE_ERROR;
  }

  if (!S_ISREG(src_stat.st_mode)) {
    fprintf(stderr, "%s 不是普通文件\n", src_path);
    return DM_SRC_FILE_ERROR;
  }

  long file_size = src_stat.st_size;
  long max_size_bytes = max_size_kb * 1024;

  // 检查源文件是否超过目录大小限制
  if (file_size > max_size_bytes) {
    fprintf(stderr, "源文件大小 %.2f KB 超过目录限制 %.2f KB\n", (double)file_size / 1024,
            (double)max_size_bytes / 1024);
    return DM_FILE_TOO_LARGE;
  }

  // 获取目标目录当前文件信息
  FileList file_list;
  init_file_list(&file_list);
  if (collect_files(dir_path, &file_list) != DM_SUCCESS) {
    free_file_list(&file_list);
    return DM_ERROR;
  }

  // 检查是否需要释放空间
  if (file_list.total_size + file_size > max_size_bytes) {
    printf("需要释放 %.2f KB 空间\n", (double)(file_list.total_size + file_size - max_size_bytes) / 1024);

    if (free_up_space(&file_list, max_size_bytes - file_size) != DM_SUCCESS) {
      free_file_list(&file_list);
      return DM_NO_SPACE;
    }
  }

  // 构建目标路径
  char dst_path[PATH_MAX];
  const char *filename = strrchr(src_path, '/');
  filename = filename ? filename + 1 : src_path;
  snprintf(dst_path, sizeof(dst_path), "%s/%s", dir_path, filename);

  // 复制文件
  printf("正在保存文件 %s 到 %s...\n", src_path, dst_path);
  int result = copy_file(src_path, dst_path);

  free_file_list(&file_list);
  return result;
}

/**
 * API: 获取目录当前大小
 *
 * @param dir_path 目录路径
 * @param[out] size_kb 返回目录大小(KB)
 * @return 错误码 (DM_SUCCESS 表示成功)
 */
int dm_get_dir_size(const char *dir_path, long *size_kb) {
  FileList file_list;
  init_file_list(&file_list);

  if (collect_files(dir_path, &file_list) != DM_SUCCESS) {
    free_file_list(&file_list);
    return DM_ERROR;
  }

  *size_kb = file_list.total_size / 1024;
  free_file_list(&file_list);
  return DM_SUCCESS;
}

/**
 * API: 清理目录到指定大小
 *
 * @param dir_path 目录路径
 * @param max_size_kb 目标大小(KB)
 * @return 错误码 (DM_SUCCESS 表示成功)
 */
int dm_clean_dir_to_size(const char *dir_path, long max_size_kb) {
  FileList file_list;
  init_file_list(&file_list);

  if (collect_files(dir_path, &file_list) != DM_SUCCESS) {
    free_file_list(&file_list);
    return DM_ERROR;
  }

  long max_size_bytes = max_size_kb * 1024;
  if (file_list.total_size > max_size_bytes) {
    int result = free_up_space(&file_list, max_size_bytes);
    free_file_list(&file_list);
    return result;
  }

  free_file_list(&file_list);
  return DM_SUCCESS;
}