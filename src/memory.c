#include <cxx11bindings/stream.h>

#include <stdio.h>

#define CHECK(expr)                                                         \
  do {                                                                      \
    if (!(expr)) {                                                          \
      fprintf(stderr, "Check failed at line %d: %s\n", __LINE__, #expr);   \
      return 1;                                                             \
    }                                                                       \
  } while (0)

int main(void) {
  byte data[12] = {'a', 'b', 'c'};
  byte result[12] = {0};
  struct c11_memory_stream memory;
  struct c11_stream* stream = &memory.super;

  CHECK(c11_memory_stream_init(NULL, data, 12, 3) == C11_E_POINTER);
  CHECK(c11_memory_stream_init(&memory, NULL, 1, 0) == C11_E_POINTER);
  CHECK(c11_memory_stream_init(&memory, data, -1, 0) == C11_E_INVALIDARG);
  CHECK(c11_memory_stream_init(&memory, data, 12, -1) == C11_E_INVALIDARG);
  CHECK(c11_memory_stream_init(&memory, data, 12, 13) == C11_E_INVALIDARG);
  CHECK(c11_memory_stream_init(&memory, NULL, 0, 0) == 0);
  CHECK(c11_stream_read(stream, result, 1) == 0);
  CHECK(c11_stream_write(stream, result, 1) == 0);
  CHECK(c11_stream_seek(stream, 0, seek_end) == 0);
  CHECK(c11_stream_trunc(stream, 0) == 0);
  CHECK(c11_stream_flush(stream) == 0);

  CHECK(c11_memory_stream_init(&memory, data, 12, 3) == 0);
  CHECK(c11_stream_read(stream, NULL, 1) == C11_E_POINTER);
  CHECK(c11_stream_write(stream, NULL, 1) == C11_E_POINTER);
  CHECK(c11_stream_read(stream, result, -1) == C11_E_INVALIDARG);
  CHECK(c11_stream_write(stream, result, -1) == C11_E_INVALIDARG);
  CHECK(c11_stream_read(stream, result, 0) == 0);
  CHECK(c11_stream_write(stream, result, 0) == 0);
  CHECK(c11_stream_read(stream, result, 12) == 3);
  CHECK(memcmp(result, "abc", 3) == 0);
  CHECK(c11_stream_read(stream, result, 1) == 0);
  CHECK(c11_stream_seek(stream, -2, seek_cur) == 1);
  CHECK(c11_stream_write(stream, (const byte*)"XY", 2) == 2);
  CHECK(memcmp(data, "aXY", 3) == 0);
  CHECK(c11_stream_seek(stream, 2, seek_end) == 5);
  CHECK(c11_stream_write(stream, (const byte*)"z", 1) == 1);
  CHECK(memory.length == 6 && memory.position == 6);
  CHECK(data[3] == 0 && data[4] == 0 && data[5] == 'z');

  CHECK(c11_stream_trunc(stream, 2) == 2);
  CHECK(memory.position == 6);
  CHECK(c11_stream_read(stream, result, 1) == 0);
  CHECK(c11_stream_trunc(stream, 8) == 8);
  CHECK(memory.position == 6);
  CHECK(memcmp(data, "aX\0\0\0\0\0\0", 8) == 0);
  CHECK(c11_stream_seek(stream, 0, seek_end) == 8);
  CHECK(c11_stream_write(stream, (const byte*)"abcdef", 6) == 4);
  CHECK(memory.length == 12 && memory.position == 12);
  CHECK(c11_stream_write(stream, result, 1) == 0);
  CHECK(c11_stream_seek(stream, INT64_MIN, seek_cur) == C11_E_INVALIDARG);
  CHECK(c11_stream_seek(stream, INT64_MAX, seek_cur) == C11_E_INVALIDARG);
  CHECK(c11_stream_seek(stream, 1, seek_end) == C11_E_INVALIDARG);
  CHECK(c11_stream_seek(stream, 0, -1) == C11_E_INVALIDARG);
  CHECK(c11_stream_trunc(stream, -1) == C11_E_INVALIDARG);
  CHECK(c11_stream_trunc(stream, 13) == C11_E_INVALIDARG);
  CHECK(memory.length == 12 && memory.position == 12);
  CHECK(c11_stream_seek(stream, 0, seek_beg) == 0);
  CHECK(c11_stream_read(stream, result, 12) == 12);
  CHECK(memcmp(result, "aX\0\0\0\0\0\0abcd", 12) == 0);

  CHECK(c11_memory_stream_read(NULL, result, 1) == C11_E_POINTER);
  CHECK(c11_memory_stream_write(NULL, result, 1) == C11_E_POINTER);
  CHECK(c11_memory_stream_seek(NULL, 0, seek_beg) == C11_E_POINTER);
  CHECK(c11_memory_stream_flush(NULL) == C11_E_POINTER);
  CHECK(c11_memory_stream_trunc(NULL, 0) == C11_E_POINTER);

  CHECK(c11_memory_stream_init(&memory, data, 12, 0) == 0);
  memcpy(data, "abcdef", 6);
  CHECK(c11_stream_seek(stream, 4, seek_beg) == 4);
  CHECK(c11_stream_write(stream, data + 1, 3) == 3);
  CHECK(memcmp(data, "\0\0\0\0bcd", 7) == 0);
  return 0;
}
