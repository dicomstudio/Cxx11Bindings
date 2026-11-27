#ifndef _WIN32
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200112L
#endif
#endif

#include <cxx11bindings/file.h>

#include <stdio.h>

int test_c11_stream_seek_direction(void);

#ifndef _WIN32
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#endif

#define CHECK(expression)                                                     \
  do {                                                                        \
    if (!(expression)) {                                                      \
      fprintf(stderr, "Check failed at line %d: %s\n", __LINE__, #expression); \
      return 1;                                                               \
    }                                                                         \
  } while (0)

int main(void) {
  FILE* file = tmpfile();
  CHECK(test_c11_stream_seek_direction() == 0);
  CHECK(file != NULL);

  CHECK(c11_file_stream_create1(NULL, "unused", "wb") == C11_E_POINTER);
  CHECK(c11_file_stream_create2(NULL, L"unused", L"wb") == C11_E_POINTER);
  CHECK(c11_file_stream_init(NULL, file) == C11_E_POINTER);

  struct c11_stream* stream = NULL;
  CHECK(c11_file_stream_init(&stream, file) == 0);
  CHECK(stream != NULL);
  const byte contents[] = {'a', 'b', 'c', 'd'};
  byte readback[sizeof contents];
  CHECK(c11_stream_write(stream, contents, sizeof contents) == sizeof contents);
  CHECK(c11_stream_seek(stream, 0, seek_beg) == 0);
  CHECK(c11_stream_read(stream, readback, 1) == 1);
  CHECK(readback[0] == 'a');
  CHECK(c11_stream_trunc(stream, 2) == 2);
  CHECK(c11_stream_seek(stream, 0, seek_cur) == 1);
  CHECK(c11_stream_read(stream, readback, sizeof readback) == 1);
  CHECK(readback[0] == 'b');
  CHECK(c11_stream_read(stream, readback, sizeof readback) == 0);
  CHECK(c11_file_stream_destroy(stream) == 0);
  CHECK(fclose(file) == 0);

#ifdef _WIN32
  CHECK(c11_handle_stream_init(NULL, GetCurrentProcess()) == C11_E_POINTER);
#else
  CHECK(c11_fd_stream_init(NULL, 0) == C11_E_POINTER);
  CHECK(c11_fd_stream_init(&stream, -1) == C11_E_INVALIDARG);

  int fds[2];
  CHECK(pipe(fds) == 0);
  if (fds[0] != 0) {
    CHECK(dup2(fds[0], 0) == 0);
    CHECK(close(fds[0]) == 0);
  }
  CHECK(write(fds[1], "x", 1) == 1);
  CHECK(c11_fd_stream_init(&stream, 0) == 0);
  CHECK(stream != NULL);
  byte value = 0;
  CHECK(c11_stream_read(stream, &value, 1) == 1);
  CHECK(value == 'x');
  CHECK(c11_file_stream_destroy(stream) == 0);
  CHECK(fcntl(0, F_GETFD) != -1);
  CHECK(close(0) == 0);
  CHECK(close(fds[1]) == 0);

  CHECK(pipe(fds) == 0);
  const long pipe_buf = fpathconf(fds[1], _PC_PIPE_BUF);
  CHECK(pipe_buf > 0 && pipe_buf <= INT32_MAX / 2);
  byte* data = calloc((size_t)pipe_buf * 2, 1);
  CHECK(data != NULL);
  const int flags = fcntl(fds[1], F_GETFL);
  CHECK(flags != -1);
  CHECK(fcntl(fds[1], F_SETFL, flags | O_NONBLOCK) == 0);
  ssize_t written;
  while ((written = write(fds[1], data, (size_t)pipe_buf)) > 0) {}
  CHECK(written == -1 && (errno == EAGAIN || errno == EWOULDBLOCK));
  CHECK(read(fds[0], data, (size_t)pipe_buf) == pipe_buf);
  CHECK(c11_fd_stream_init(&stream, fds[1]) == 0);
  const buf_size partial =
      c11_stream_write(stream, data, (buf_size)(pipe_buf * 2));
  CHECK(partial > 0 && partial < pipe_buf * 2);
  CHECK(c11_stream_write(stream, data, (buf_size)pipe_buf) == C11_E_IO);
  CHECK(c11_file_stream_destroy(stream) == 0);

  CHECK(read(fds[0], data, (size_t)pipe_buf) == pipe_buf);
  FILE* pipe_file = fdopen(fds[1], "w");
  CHECK(pipe_file != NULL);
  CHECK(setvbuf(pipe_file, NULL, _IONBF, 0) == 0);
  CHECK(c11_file_stream_init(&stream, pipe_file) == 0);
  const buf_size file_partial =
      c11_stream_write(stream, data, (buf_size)(pipe_buf * 2));
  CHECK(file_partial > 0 && file_partial < pipe_buf * 2);
  CHECK(c11_stream_write(stream, data, (buf_size)pipe_buf) == C11_E_IO);
  CHECK(c11_file_stream_destroy(stream) == 0);
  CHECK(fclose(pipe_file) == 0);
  free(data);
  CHECK(close(fds[0]) == 0);
#endif

  return 0;
}
