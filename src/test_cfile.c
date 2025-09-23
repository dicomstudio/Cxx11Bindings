#include <cxx11bindings/file.h>

int main(const int argc, char* argv[]) {
  if (argc < 2) {
    return 1;
  }
  const char* filename = argv[1];
  struct c11_stream* stream;
  int ret = c11_file_stream_create1(&stream, filename, "wb");
  if (ret < 0) return 1;
  const char data[] = "Hello, World!";
  const buf_size size = c11_stream_write(stream, data, sizeof data);
  if (size < 0) return 1;
  const stream_length l = c11_stream_trunc(stream, 1024);
  if (l < 0) return 1;
  ret = c11_file_stream_destroy(stream);
  if (ret < 0) return 1;
  return 0;
}
