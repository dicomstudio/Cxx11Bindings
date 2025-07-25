#include "cxx11bindings.h"

#include <new>

extern "C" {
c11_stream* c11_stream_create(const read_fn read, const write_fn write,
                              const seek_fn seek, const flush_fn flush,
                              const trunc_fn trunc) {
  try {
    const auto stream =
        new (std::nothrow) c11_stream{read, write, seek, flush, trunc};
    return stream;
  } catch (...) {
    // return reinterpret_cast<c11_stream*>(-1);
    return nullptr;
  }
}

int c11_stream_delete(const c11_stream* c11_stream) {
  try {
    delete c11_stream;
    return 0;
  } catch (...) {
    // FIXME
    return -1;
  }
}
}  // extern "C"
