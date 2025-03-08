#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <unistd.h>

#define PAGE_SIZE (4096)
#define MIN_LEN (32 * PAGE_SIZE)

void slow_cat(int fd) {
  ssize_t wrt_len;
  uint8_t buf[MIN_LEN];
  // TODO: error handling.
  while ((wrt_len = read(fd, buf, MIN_LEN))) {
    write(STDOUT_FILENO, buf, wrt_len);
  }
}

void cat(int fd) {
  struct stat st;
  if (fstat(fd, &st) == -1)
    exit(EXIT_FAILURE);

  if (st.st_size == 0) {
    slow_cat(fd);
    return;
  }

  ssize_t len = st.st_size < MIN_LEN ? MIN_LEN : st.st_size;
  ssize_t wrt_len;
  while ((wrt_len = sendfile(STDOUT_FILENO, fd, NULL, len)) > 0)
    ;

  if (wrt_len == -1) {
    // TODO: ENOSYS sticky.
    if (errno == EINVAL || errno == ENOSYS) {
      errno = 0;
      slow_cat(fd);
      return;
    }

    perror("sendfile");
    exit(EXIT_FAILURE);
  }
}

int main(int argc, char *argv[]) {
  int opt;
  while ((opt = getopt(argc, argv, "u")) != -1) {
    switch (opt) {
    case 'u':
      break;
    case '?':
      fprintf(stderr, "Usage: %s [-u] [file...]", argv[0]);
      exit(EXIT_FAILURE);
    }
  }

  if (optind == argc) {
    cat(STDIN_FILENO);
    exit(EXIT_SUCCESS);
  }

  int exit_status = EXIT_SUCCESS;
  for (int i = optind; i < argc; ++i) {
    if (strcmp(argv[i], "-") == 0) {
      cat(STDIN_FILENO);
    } else {
      int fd = open(argv[i], O_RDONLY);
      if (fd == -1) {
        fprintf(stderr, "%s: %s: No such file or directory\n", argv[0],
                argv[i]);
        exit_status = EXIT_FAILURE;
        continue;
      }
      cat(fd);
      close(fd);
    }
  }

  exit(exit_status);
}
