#include <cxx11bindings/filebuf.hxx>

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <new>
#include <sstream>

namespace {
long fail_after = -1;
}

void* operator new(std::size_t size) {
  if (fail_after == 0) throw std::bad_alloc();
  if (fail_after > 0) --fail_after;
  if (void* ptr = std::malloc(size)) return ptr;
  throw std::bad_alloc();
}

void operator delete(void* ptr) noexcept { std::free(ptr); }

#include "filebuf.cxx"

#define CHECK(expression)                                                     \
  do {                                                                        \
    if (!(expression)) {                                                      \
      std::fprintf(stderr, "Check failed at line %d: %s\n", __LINE__, #expression); \
      return 1;                                                               \
    }                                                                         \
  } while (0)

int main() {
  c11_stream sentinel = {};
  c11_stream* stream = &sentinel;
  std::stringbuf borrowed;

  fail_after = 0;
  const int init_result = cxx11_file_stream_init(&stream, &borrowed);
  fail_after = -1;
  CHECK(init_result == C11_E_OUTOFMEMORY);
  CHECK(stream == nullptr);
  CHECK(borrowed.sputc('x') != std::char_traits<char>::eof());

  const char* filename = "filebuf_alloc_failure_test.tmp";
  bool saw_failure = false;
  bool saw_success = false;
  long allocation_count = -1;
  for (long index = 0; index < 64; ++index) {
    stream = &sentinel;
    fail_after = index;
    const int result = cxx11_file_stream_create1(&stream, filename, "wb");
    fail_after = -1;
    if (result == 0) {
      CHECK(stream != nullptr);
      CHECK(cxx11_file_stream_destroy(stream) == 0);
      saw_success = true;
      allocation_count = index;
      break;
    }
    CHECK(result == C11_E_OUTOFMEMORY);
    CHECK(stream == nullptr);
    saw_failure = true;
  }
  CHECK(saw_failure && saw_success);
  CHECK(allocation_count > 0);
  {
    std::filebuf probe;
    CHECK(probe.open(filename, std::ios::out | std::ios::binary) != nullptr);
    c11_stream* wrapper = &sentinel;
    fail_after = 0;
    const int wrapper_result = file_stream_init(&wrapper, &probe, false);
    fail_after = -1;
    CHECK(wrapper_result == C11_E_OUTOFMEMORY);
    CHECK(wrapper == nullptr);
    CHECK(probe.close() != nullptr);
  }
  CHECK(std::remove(filename) == 0);

  const std::string long_mode(256, 'x');
  stream = &sentinel;
  fail_after = 0;
  const int mode_result =
      cxx11_file_stream_create1(&stream, "unused", long_mode.c_str());
  fail_after = -1;
  CHECK(mode_result == C11_E_OUTOFMEMORY);
  CHECK(stream == nullptr);

  stream = &sentinel;
  fail_after = 0;
  const int wide_result = cxx11_file_stream_create2(&stream, L"unused", L"r");
  fail_after = -1;
  CHECK(wide_result == C11_E_OUTOFMEMORY);
  CHECK(stream == nullptr);

#ifdef _MSC_VER
  const wchar_t* wide_filename = L"filebuf_alloc_failure_wide_test.tmp";
  saw_failure = false;
  saw_success = false;
  for (long index = 0; index < 64; ++index) {
    stream = &sentinel;
    fail_after = index;
    const int result = cxx11_file_stream_create2(&stream, wide_filename, L"wb");
    fail_after = -1;
    if (result == 0) {
      CHECK(stream != nullptr);
      CHECK(cxx11_file_stream_destroy(stream) == 0);
      saw_success = true;
      break;
    }
    CHECK(result == C11_E_OUTOFMEMORY);
    CHECK(stream == nullptr);
    saw_failure = true;
  }
  CHECK(saw_failure && saw_success);
  CHECK(_wremove(wide_filename) == 0);
#endif
  return 0;
}
