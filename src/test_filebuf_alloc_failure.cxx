#include <cxx11bindings/filebuf.hxx>

#include <cstdio>
#include <new>
#include <sstream>
#include <stdexcept>

namespace {
enum class failure_stage { none, mode, filebuf, open, wrapper };
failure_stage fail_at = failure_stage::none;
bool fail_with_io = false;

void inject_failure(failure_stage stage) {
  if (fail_at == stage) {
    if (fail_with_io) throw std::runtime_error("injected open failure");
    throw std::bad_alloc();
  }
}
}

#define CXX11_FILEBUF_TEST_HOOK(stage) inject_failure(failure_stage::stage)
#define CXX11_FILEBUF_TEST_ALLOC_FAIL() (fail_at == failure_stage::wrapper)
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

  fail_at = failure_stage::wrapper;
  CHECK(cxx11_file_stream_init(&stream, &borrowed) == C11_E_OUTOFMEMORY);
  CHECK(stream == nullptr);
  CHECK(borrowed.sputc('x') != std::char_traits<char>::eof());
  fail_at = failure_stage::none;

  const char* filename = "filebuf_alloc_failure_test.tmp";
  const wchar_t* wide_filename = L"filebuf_alloc_failure_test.tmp";
  const failure_stage stages[] = {failure_stage::mode, failure_stage::filebuf,
                                  failure_stage::open, failure_stage::wrapper};
  for (const auto stage : stages) {
    fail_at = stage;
    stream = &sentinel;
    CHECK(cxx11_file_stream_create1(&stream, filename, "wb") ==
          C11_E_OUTOFMEMORY);
    CHECK(stream == nullptr);
    stream = &sentinel;
    CHECK(cxx11_file_stream_create2(&stream, wide_filename, L"wb") ==
          C11_E_OUTOFMEMORY);
    CHECK(stream == nullptr);
  }

  fail_at = failure_stage::open;
  fail_with_io = true;
  stream = &sentinel;
  CHECK(cxx11_file_stream_create1(&stream, filename, "wb") == C11_E_IO);
  CHECK(stream == nullptr);
  stream = &sentinel;
  CHECK(cxx11_file_stream_create2(&stream, wide_filename, L"wb") == C11_E_IO);
  CHECK(stream == nullptr);
  fail_at = failure_stage::none;
  fail_with_io = false;

  CHECK(cxx11_file_stream_init(&stream, &borrowed) == 0);
  CHECK(cxx11_file_stream_destroy(stream) == 0);
  CHECK(cxx11_file_stream_create1(&stream, filename, "wb") == 0);
  CHECK(cxx11_file_stream_destroy(stream) == 0);
  CHECK(cxx11_file_stream_create2(&stream, wide_filename, L"wb") == 0);
  CHECK(cxx11_file_stream_destroy(stream) == 0);
  CHECK(std::remove(filename) == 0);
  return 0;
}
