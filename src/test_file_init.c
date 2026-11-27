#include <cxx11bindings/file.h>

#include <stdio.h>

#ifndef _WIN32
#include <fcntl.h>
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
  CHECK(file != NULL);

  CHECK(c11_file_stream_create1(NULL, "unused", "wb") == C11_E_POINTER);
  CHECK(c11_file_stream_create2(NULL, L"unused", L"wb") == C11_E_POINTER);
  CHECK(c11_file_stream_init(NULL, file) == C11_E_POINTER);

  struct c11_stream* stream = NULL;
  CHECK(c11_file_stream_init(&stream, file) == 0);
  CHECK(stream != NULL);
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
#endif

  return 0;
}
