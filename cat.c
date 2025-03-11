#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef PAGE_SIZE
#define PAGE_SIZE (4096)
#endif
#define MAX_TRANSFER_BYTES_SLOW_LEN (32 * PAGE_SIZE)

ssize_t transfer_bytes_slow(int in_fd, int out_fd, size_t len) {
  if (len > MAX_TRANSFER_BYTES_SLOW_LEN) {
    return -1;
  }
  uint8_t buf[MAX_TRANSFER_BYTES_SLOW_LEN];
  ssize_t rd_len = read(in_fd, buf, len);
  if (rd_len == -1) {
    return -1;
  }
  return write(out_fd, buf, (size_t)rd_len);
}

int transfer_file_slow(int in_fd, int out_fd) {
  ssize_t len;
  while ((len = transfer_bytes_slow(in_fd, out_fd,
                                    MAX_TRANSFER_BYTES_SLOW_LEN)) > 0)
    ;
  return (int)len;
}

#ifdef __linux__
#include <sys/sendfile.h>
bool g_transfer_bytes_fast = true;
#else
bool g_transfer_bytes_fast = false;
#endif

ssize_t transfer_bytes_fast(int in_fd, int out_fd, size_t len) {
#ifdef __linux__
  ssize_t ret = sendfile(out_fd, in_fd, NULL, len);
  if (errno != 0) {
    g_transfer_bytes_fast &= !(errno == ENOSYS);
    errno = 0;
  }
  return ret;
#else
  (void) in_fd;
  (void) out_fd;
  (void) len;
  return -1;
#endif
}

int transfer_file_fast(int in_fd, int out_fd) {
  if (!g_transfer_bytes_fast) {
    return transfer_file_slow(in_fd, out_fd);
  }

  struct stat st;
  if (fstat(in_fd, &st) == -1) {
    return -1;
  }

  if (st.st_size == 0) {
    return transfer_file_slow(in_fd, out_fd);
  }

  size_t rd_len = (size_t)st.st_size;
  if (rd_len < MAX_TRANSFER_BYTES_SLOW_LEN) {
    rd_len = MAX_TRANSFER_BYTES_SLOW_LEN;
  }
  ssize_t wr_len;
  while ((wr_len = transfer_bytes_fast(in_fd, out_fd, rd_len)) > 0)
    ;

  if (wr_len == -1) {
    return transfer_file_slow(in_fd, out_fd);
  }

  return 0;
}

int main(int argc, char *argv[]) {
  int opt;
  while ((opt = getopt(argc, argv, "+u")) != -1) {
    switch (opt) {
    case 'u':
      break;
    case '?':
      fprintf(stderr, "Usage: %s [-u] [file...]", argv[0]);
      return EXIT_FAILURE;
    }
  }

  if (optind == argc) {
    return transfer_file_slow(STDIN_FILENO, STDOUT_FILENO) ? EXIT_FAILURE
                                                           : EXIT_SUCCESS;
  }

  int status = 0;
  for (int i = optind; i < argc; ++i) {
    if (strcmp(argv[i], "-") == 0) {
      status |= transfer_file_slow(STDIN_FILENO, STDOUT_FILENO);
      continue;
    }

    int in_fd = open(argv[i], O_RDONLY);
    if (in_fd == -1) {
      fprintf(stderr, "%s: %s: No such file or directory\n", argv[0], argv[i]);
      status = -1;
      continue;
    }
    status |= transfer_file_slow(in_fd, STDOUT_FILENO);
    if (close(in_fd) == -1) {
      return EXIT_FAILURE;
    }
  }

  return status ? EXIT_FAILURE : EXIT_SUCCESS;
}
