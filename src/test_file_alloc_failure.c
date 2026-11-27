#include <cxx11bindings/file.h>

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

static void* fail_malloc(size_t size) {
  (void)size;
  return NULL;
}

#define malloc fail_malloc
#include "file.c"
#undef malloc

#define CHECK(expression)                                                     \
  do {                                                                        \
    if (!(expression)) {                                                      \
      fprintf(stderr, "Check failed at line %d: %s\n", __LINE__, #expression); \
      return 1;                                                               \
    }                                                                         \
  } while (0)

int main(void) {
  struct c11_stream* stream = (struct c11_stream*)1;
  FILE* file = tmpfile();
  CHECK(file != NULL);
  CHECK(c11_file_stream_init(&stream, file) == C11_E_OUTOFMEMORY);
  CHECK(stream == NULL);
  CHECK(fclose(file) == 0);

#ifdef _WIN32
  stream = (struct c11_stream*)1;
  HANDLE handle = CreateFileW(L"NUL", GENERIC_READ,
                              FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  CHECK(handle != INVALID_HANDLE_VALUE);
  CHECK(c11_handle_stream_init(&stream, handle) == C11_E_OUTOFMEMORY);
  CHECK(stream == NULL);
  CHECK(CloseHandle(handle));
#else
  stream = (struct c11_stream*)1;
  CHECK(c11_fd_stream_init(&stream, 0) == C11_E_OUTOFMEMORY);
  CHECK(stream == NULL);
#endif

  return 0;
}
