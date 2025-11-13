#ifndef PROCESS_UTILS_H
#define PROCESS_UTILS_H

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <unistd.h>

static int lock_fd = -1;
static volatile sig_atomic_t g_exit = 0;

static void exit_handle(int sign) { g_exit = 1; }

static void init_signal(void) {
#ifdef USE_PROCESS_SIGNAL
  signal(SIGTERM, exit_handle);
  signal(SIGINT, exit_handle);
#endif
  signal(SIGPIPE, SIG_IGN);
  signal(SIGCHLD, SIG_IGN);
}

static sig_atomic_t get_exit_status(void) { return g_exit; }

static const char *get_basename(const char *path) {
  const char *base = strrchr(path, '/');
  return base ? base + 1 : path;
}

static int update_restart_count(char *buffer, size_t size) {
  int restart_count = 0;
  char *saveptr = NULL;
  char *first_line = strtok_r(buffer, "\n", &saveptr);

  if (first_line == NULL || strlen(first_line) == 0) {
    restart_count = 1;
  } else {
    if (sscanf(first_line, "Restart Count: %d", &restart_count) != 1) {
      restart_count = 1;
    } else {
      restart_count++;
    }
  }

  pid_t pid = getpid();

  snprintf(buffer, size, "Restart Count:%d\nPID:%d\n", restart_count, pid);

  return restart_count;
}

static int process_end(void) {
  if (lock_fd >= 0) {
    close(lock_fd);
    lock_fd = -1;
  }
}

static int process_start(const char *process_name) {
  const char *base_name = get_basename(process_name);
  char lock_file[256];

  snprintf(lock_file, sizeof(lock_file), "/tmp/%s.pid", base_name);

  lock_fd = open(lock_file, O_CREAT | O_RDWR, 0666);
  if (lock_fd < 0) {
    perror("Unable to open lock file");
    exit(1);
  }

#ifdef USE_PROCESS_FLOCK
  int rc = flock(lock_fd, LOCK_EX | LOCK_NB);
  if (rc) {
    if (errno == EWOULDBLOCK) {
      printf("%s: Already Running\n", base_name);
    } else {
      perror("Unable to acquire lock");
    }
    close(lock_fd);
    exit(1);
  }
#endif

  char file_buffer[1024] = {0};
  ssize_t bytes_read = read(lock_fd, file_buffer, sizeof(file_buffer) - 1);
  if (bytes_read == -1) {
    perror("Unable to read lock file");
    close(lock_fd);
    exit(1);
  }

  update_restart_count(file_buffer, sizeof(file_buffer));

  if (lseek(lock_fd, 0, SEEK_SET) == -1) {
    perror("Unable to seek lock file");
    close(lock_fd);
    exit(1);
  }

  if (write(lock_fd, file_buffer, strlen(file_buffer)) == -1) {
    perror("Unable to write updated content to lock file");
    close(lock_fd);
    exit(1);
  }
  return 0;
}

#endif  // PROCESS_UTILS_H
