#include "stream_interface_impl.hxx"

#include <cxx11bindings/filebuf.hxx>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>  // std::filebuf
#include <gtest/gtest.h>
#include <limits>
#include <sstream>

TEST(FilebufStreamCreate, ClearsOutputOnFailure) {
  c11_stream sentinel = {};
  c11_stream* stream = &sentinel;

  EXPECT_EQ(cxx11_file_stream_create1(&stream, nullptr, "r"), C11_E_INVALIDARG);
  EXPECT_EQ(stream, nullptr);
  stream = &sentinel;
  EXPECT_EQ(cxx11_file_stream_create1(&stream, "unused", nullptr),
            C11_E_INVALIDARG);
  EXPECT_EQ(stream, nullptr);
  stream = &sentinel;
  EXPECT_EQ(cxx11_file_stream_create1(&stream, "", "r"), C11_E_IO);
  EXPECT_EQ(stream, nullptr);

  stream = &sentinel;
  EXPECT_EQ(cxx11_file_stream_create2(&stream, nullptr, L"r"), C11_E_INVALIDARG);
  EXPECT_EQ(stream, nullptr);
  stream = &sentinel;
  EXPECT_EQ(cxx11_file_stream_create2(&stream, L"unused", nullptr),
            C11_E_INVALIDARG);
  EXPECT_EQ(stream, nullptr);
  stream = &sentinel;
  EXPECT_EQ(cxx11_file_stream_create2(&stream, L"", L"r"), C11_E_IO);
  EXPECT_EQ(stream, nullptr);
}

TEST(FilebufStreamCreate, RejectsNullOutput) {
  EXPECT_EQ(cxx11_file_stream_create1(nullptr, "", "r"), C11_E_POINTER);
  EXPECT_EQ(cxx11_file_stream_create2(nullptr, L"", L"r"), C11_E_POINTER);
  EXPECT_EQ(cxx11_file_stream_create1(nullptr, nullptr, nullptr), C11_E_POINTER);
  EXPECT_EQ(cxx11_file_stream_create2(nullptr, nullptr, nullptr), C11_E_POINTER);
}

TEST(FilebufStreamCreate, NullOutputDoesNotTruncateFile) {
  const char* filename = "filebuf_null_output_test.tmp";
  {
    std::ofstream output(filename, std::ios::binary | std::ios::trunc);
    ASSERT_TRUE(output.is_open());
    output << "original";
    output.close();
    ASSERT_TRUE(output.good());
  }

  EXPECT_EQ(cxx11_file_stream_create1(nullptr, filename, "wb"), C11_E_POINTER);
  EXPECT_EQ(cxx11_file_stream_create2(nullptr, L"filebuf_null_output_test.tmp",
                                     L"wb"),
            C11_E_POINTER);

  std::ifstream input(filename, std::ios::binary);
  ASSERT_TRUE(input.is_open());
  std::string contents;
  std::getline(input, contents);
  EXPECT_EQ(contents, "original");
  input.close();
  EXPECT_EQ(std::remove(filename), 0);
}

TEST(FilebufStreamCreate, OpensUtf8FilenameForWritingAndReading) {
  const char* filename = "filebuf_utf8_\xc3\xa9_\xf0\x9f\x98\x80.tmp";
  c11_stream* stream = nullptr;
  ASSERT_EQ(cxx11_file_stream_create1(&stream, filename, "wb"), 0);
  const byte data[] = {'u', 0, '8'};
  EXPECT_EQ(c11_stream_write(stream, data, sizeof data), sizeof data);
  EXPECT_EQ(cxx11_file_stream_destroy(stream), 0);

  stream = nullptr;
  ASSERT_EQ(cxx11_file_stream_create1(&stream, filename, "rb"), 0);
  byte readback[sizeof data] = {};
  EXPECT_EQ(c11_stream_read(stream, readback, sizeof readback), sizeof data);
  EXPECT_EQ(std::memcmp(readback, data, sizeof data), 0);
  EXPECT_EQ(cxx11_file_stream_destroy(stream), 0);
#ifdef _WIN32
  EXPECT_EQ(_wremove(L"filebuf_utf8_\u00e9_\U0001f600.tmp"), 0);
#else
  EXPECT_EQ(std::remove(filename), 0);
#endif
}

#ifdef _WIN32
TEST(FilebufStreamCreate, RejectsInvalidUtf8Filename) {
  c11_stream sentinel = {};
  c11_stream* stream = &sentinel;
  EXPECT_EQ(cxx11_file_stream_create1(&stream, "\xc3\x28", "wb"),
            C11_E_INVALIDARG);
  EXPECT_EQ(stream, nullptr);
}
#endif

TEST(FilebufStreamCreate, OpensWideFilenameForWritingAndReading) {
  const wchar_t* filename = L"filebuf_wide_\u00e9_\U0001f600.tmp";
  c11_stream* stream = nullptr;
  ASSERT_EQ(cxx11_file_stream_create2(&stream, filename, L"wb"), 0);
  const byte data[] = {'a', 0, 'b'};
  EXPECT_EQ(c11_stream_write(stream, data, sizeof data), sizeof data);
  EXPECT_EQ(cxx11_file_stream_destroy(stream), 0);

  stream = nullptr;
  ASSERT_EQ(cxx11_file_stream_create2(&stream, filename, L"rb"), 0);
  byte readback[sizeof data] = {};
  EXPECT_EQ(c11_stream_read(stream, readback, sizeof readback), sizeof data);
  EXPECT_EQ(std::memcmp(readback, data, sizeof data), 0);
  EXPECT_EQ(cxx11_file_stream_destroy(stream), 0);
#ifdef _WIN32
  EXPECT_EQ(_wremove(filename), 0);
#else
  EXPECT_EQ(std::remove("filebuf_wide_\xc3\xa9_\xf0\x9f\x98\x80.tmp"), 0);
#endif
}

#ifndef _WIN32
TEST(FilebufStreamCreate, RejectsInvalidWideFilename) {
  const wchar_t invalid[] = {static_cast<wchar_t>(0xd800), 0};
  c11_stream sentinel = {};
  c11_stream* stream = &sentinel;
  EXPECT_EQ(cxx11_file_stream_create2(&stream, invalid, L"rb"),
            C11_E_INVALIDARG);
  EXPECT_EQ(stream, nullptr);
}
#endif

TEST(FilebufStreamDestroy, RejectsNull) {
  EXPECT_EQ(cxx11_file_stream_destroy(nullptr), C11_E_INVALIDARG);
}

TEST(FilebufStreamDestroy, LeavesBorrowedBufferUntouched) {
  std::stringbuf buffer;
  c11_stream* stream = nullptr;
  ASSERT_EQ(cxx11_file_stream_init(&stream, &buffer), 0);
  const byte data[] = {'a', 'b'};
  ASSERT_EQ(c11_stream_write(stream, data, sizeof data), sizeof data);
  EXPECT_EQ(cxx11_file_stream_destroy(stream), 0);
  EXPECT_EQ(buffer.str(), "ab");
  EXPECT_NE(buffer.sputc('c'), std::char_traits<char>::eof());
  EXPECT_EQ(buffer.str(), "abc");
}

TEST(FilebufStreamDestroy, ClosesOwnedFileAndFlushesOutput) {
  const char* filename = "filebuf_destroy_test.tmp";
  c11_stream* stream = nullptr;
  ASSERT_EQ(cxx11_file_stream_create1(&stream, filename, "wb"), 0);
  const byte data[] = {'a', 'b', 'c'};
  EXPECT_EQ(c11_stream_write(stream, data, sizeof data), sizeof data);
  EXPECT_EQ(cxx11_file_stream_destroy(stream), 0);
  std::ifstream input(filename, std::ios::binary);
  ASSERT_TRUE(input.is_open());
  char readback[sizeof data] = {};
  input.read(readback, sizeof readback);
  EXPECT_EQ(input.gcount(), sizeof readback);
  EXPECT_EQ(std::string(readback, sizeof readback), "abc");
  input.close();
  EXPECT_EQ(std::remove(filename), 0);
}

#ifdef __linux__
TEST(FilebufStreamDestroy, ReportsOwnedFileCloseFailure) {
  std::ifstream available("/dev/full");
  if (!available.is_open()) {
    GTEST_SKIP() << "/dev/full is unavailable";
  }
  available.close();
  c11_stream* stream = nullptr;
  ASSERT_EQ(cxx11_file_stream_create1(&stream, "/dev/full", "wb"), 0);
  const byte value = 'x';
  ASSERT_EQ(c11_stream_write(stream, &value, 1), 1);
  EXPECT_EQ(cxx11_file_stream_destroy(stream), C11_E_IO);
}
#endif

TEST(ErrorMapping, ExtendedCallbackErrorsRetainTheirCodes) {
  const cxx11::ErrorCode codes[] = {
      cxx11::ErrorCode::InvalidOperation, cxx11::ErrorCode::Unauthorized,
      cxx11::ErrorCode::OutOfMemory, cxx11::ErrorCode::Timeout,
      cxx11::ErrorCode::Canceled, cxx11::ErrorCode::FileNotFound,
      cxx11::ErrorCode::DirectoryNotFound, cxx11::ErrorCode::PathTooLong,
      cxx11::ErrorCode::EndOfStream, cxx11::ErrorCode::ArgumentNull,
      cxx11::ErrorCode::NotImplemented, cxx11::ErrorCode::Unmapped};
  for (const auto code : codes) {
    try {
      cxx11::throw_exception_from_negative_value(static_cast<int>(code));
      FAIL() << "Expected callback exception";
    } catch (const cxx11::callback_exception& ex) {
      EXPECT_EQ(ex.code(), code);
    }
  }
}

class short_write_streambuf : public std::streambuf {
 public:
  explicit short_write_streambuf(std::streamsize limit) : limit_(limit) {}
  std::string written;

 protected:
  std::streamsize xsputn(const char* data, std::streamsize count) override {
    const std::streamsize n = (std::min)(count, limit_);
    written.append(data, static_cast<std::size_t>(n));
    return n;
  }

 private:
  std::streamsize limit_;
};

TEST(ShortWrite, StreambufAdapterReturnsProgress) {
  short_write_streambuf target(2);
  c11_stream* stream = nullptr;
  ASSERT_EQ(cxx11_file_stream_init(&stream, &target), 0);
  const byte data[] = {'a', 'b', 'c', 'd'};
  EXPECT_EQ(c11_stream_write(stream, data, 4), 2);
  EXPECT_EQ(target.written, "ab");
  EXPECT_EQ(cxx11_file_stream_destroy(stream), 0);
}

TEST(ShortWrite, StreambufAdapterReturnsZeroWithoutProgress) {
  short_write_streambuf target(0);
  c11_stream* stream = nullptr;
  ASSERT_EQ(cxx11_file_stream_init(&stream, &target), 0);
  const byte data[] = {'a'};
  EXPECT_EQ(c11_stream_write(stream, data, 1), 0);
  EXPECT_TRUE(target.written.empty());
  EXPECT_EQ(cxx11_file_stream_destroy(stream), 0);
}

class partial_stream : public cxx11::stream_interface {
 public:
  std::string written;
  int calls = 0;
  bool stall = false;
  bool fail = false;

  buf_size read(byte*, buf_size) override { return 0; }
  buf_size write(const byte* data, buf_size count) override {
    ++calls;
    if (calls == 2 && fail) throw cxx11::io_exception();
    if (calls == 2 && stall) return 0;
    const buf_size n = std::min<buf_size>(count, 2);
    written.append(reinterpret_cast<const char*>(data), n);
    return n;
  }
  stream_offset seek(stream_offset, seek_dir) override { return 0; }
  int flush() override { return 0; }
  stream_length trunc(stream_length size) override { return size; }
};

class memory_stream : public cxx11::stream_interface {
 public:
  std::string input;
  std::string output;
  stream_offset position = 0;
  buf_size max_read = (std::numeric_limits<buf_size>::max)();
  buf_size max_write = (std::numeric_limits<buf_size>::max)();
  bool throw_on_read = false;
  bool overreport_write = false;
  bool fail_seek = false;
  bool throw_on_seek = false;
  int read_calls = 0;
  int write_calls = 0;
  int seek_calls = 0;
  stream_offset last_seek_offset = 0;
  seek_dir last_seek_direction = seek_dirs::seek_beg;

  buf_size read(byte* buffer, buf_size count) override {
    ++read_calls;
    if (throw_on_read) throw cxx11::io_exception();
    if (count <= 0 || position < 0 ||
        static_cast<std::size_t>(position) >= input.size()) {
      return 0;
    }
    const buf_size available = static_cast<buf_size>(
        input.size() - static_cast<std::size_t>(position));
    const buf_size n = (std::min)((std::min)(count, available), max_read);
    std::memcpy(buffer, input.data() + position, static_cast<std::size_t>(n));
    position += n;
    return n;
  }

  buf_size write(const byte* buffer, buf_size count) override {
    ++write_calls;
    if (overreport_write) return count + 1;
    const buf_size n = (std::min)(count, max_write);
    output.append(reinterpret_cast<const char*>(buffer),
                  static_cast<std::size_t>(n));
    return n;
  }

  stream_offset seek(stream_offset off, seek_dir direction) override {
    ++seek_calls;
    last_seek_offset = off;
    last_seek_direction = direction;
    if (throw_on_seek) throw cxx11::io_exception();
    if (fail_seek) return -1;

    stream_offset base = 0;
    if (direction == seek_dirs::seek_cur) {
      base = position;
    } else if (direction == seek_dirs::seek_end) {
      base = static_cast<stream_offset>(input.size());
    } else if (direction != seek_dirs::seek_beg) {
      return -1;
    }
    const stream_offset target = base + off;
    if (target < 0 ||
        target > static_cast<stream_offset>(input.size())) {
      return -1;
    }
    position = target;
    return position;
  }

  int flush() override { return 0; }
  stream_length trunc(stream_length size) override { return size; }
};

TEST(DefaultStreamBuf, ReadsShortChunksAndReportsEndOfInput) {
  memory_stream target;
  target.input = "abcdefgh";
  target.max_read = 2;
  cxx11::basic_streambuf buffer(target, 4);

  char result[8] = {};
  EXPECT_EQ(buffer.sgetn(result, sizeof result), sizeof result);
  EXPECT_EQ(std::string(result, sizeof result), target.input);
  EXPECT_EQ(target.read_calls, 4);
  EXPECT_EQ(buffer.sgetc(), std::char_traits<char>::eof());
}

TEST(DefaultStreamBuf, PropagatesReadException) {
  memory_stream target;
  target.throw_on_read = true;
  cxx11::basic_streambuf buffer(target);

  EXPECT_THROW(buffer.sgetc(), cxx11::io_exception);
}

TEST(DefaultStreamBuf, PreservesPutbackCharactersAcrossRefill) {
  memory_stream target;
  target.input = "abcdefghijklmnop";
  cxx11::basic_streambuf buffer(target, 4);

  char result[12] = {};
  ASSERT_EQ(buffer.sgetn(result, sizeof result), sizeof result);
  EXPECT_EQ(std::string(result, sizeof result), "abcdefghijkl");
  EXPECT_EQ(buffer.sgetc(), 'm');

  const std::string putback = "lkjihgfe";
  for (const char expected : putback) {
    ASSERT_NE(buffer.sungetc(), std::char_traits<char>::eof());
    EXPECT_EQ(buffer.sgetc(), expected);
  }
  EXPECT_EQ(buffer.sungetc(), std::char_traits<char>::eof());
}

TEST(DefaultStreamBuf, FlushesOutputWhenBufferFillsAndOnSync) {
  memory_stream target;
  cxx11::basic_streambuf buffer(target, 2);

  EXPECT_EQ(buffer.sputn("abcdefghijklm", 13), 13);
  EXPECT_EQ(target.output, "abcdefghij");
  EXPECT_EQ(buffer.pubsync(), 0);
  EXPECT_EQ(target.output, "abcdefghijklm");
  EXPECT_EQ(buffer.pubsync(), 0);
  EXPECT_EQ(target.output, "abcdefghijklm");
}

TEST(DefaultStreamBuf, RetainsOutputAfterInvalidWriteCount) {
  memory_stream target;
  target.overreport_write = true;
  cxx11::basic_streambuf buffer(target);

  ASSERT_EQ(buffer.sputn("pending", 7), 7);
  EXPECT_EQ(buffer.pubsync(), -1);
  EXPECT_TRUE(target.output.empty());

  target.overreport_write = false;
  EXPECT_EQ(buffer.pubsync(), 0);
  EXPECT_EQ(target.output, "pending");
}

TEST(DefaultStreamBuf, OverflowReportsWriteFailureAndCanBeRetried) {
  memory_stream target;
  cxx11::basic_streambuf buffer(target, 1);
  ASSERT_EQ(buffer.sputn("123456789", 9), 9);

  target.max_write = 0;
  EXPECT_EQ(buffer.sputc('x'), std::char_traits<char>::eof());
  EXPECT_TRUE(target.output.empty());

  target.max_write = (std::numeric_limits<buf_size>::max)();
  EXPECT_EQ(buffer.pubsync(), 0);
  EXPECT_EQ(target.output, "123456789");
}

TEST(DefaultStreamBuf, SeekCurrentAdjustsForUnreadInput) {
  memory_stream target;
  target.input = "abcdef";
  cxx11::basic_streambuf buffer(target, 4);

  char first[2] = {};
  ASSERT_EQ(buffer.sgetn(first, sizeof first), sizeof first);
  ASSERT_EQ(std::string(first, sizeof first), "ab");
  EXPECT_EQ(buffer.pubseekoff(1, std::ios_base::cur, std::ios_base::in),
            std::streampos(3));
  EXPECT_EQ(target.last_seek_offset, -1);
  EXPECT_EQ(target.last_seek_direction, seek_dirs::seek_cur);
  EXPECT_EQ(buffer.sgetc(), 'd');
}

TEST(DefaultStreamBuf, SupportsEndAndAbsoluteSeeking) {
  memory_stream target;
  target.input = "abcdef";
  cxx11::basic_streambuf buffer(target);

  EXPECT_EQ(buffer.pubseekoff(0, std::ios_base::end, std::ios_base::in),
            std::streampos(6));
  EXPECT_EQ(buffer.pubseekpos(1, std::ios_base::in), std::streampos(1));
  EXPECT_EQ(buffer.sgetc(), 'b');
  EXPECT_EQ(buffer.pubseekoff(0, static_cast<std::ios_base::seekdir>(-1),
                              std::ios_base::in),
            std::streampos(-1));
  EXPECT_EQ(target.seek_calls, 2);
}

TEST(DefaultStreamBuf, FailedSeekInvalidatesBufferedInput) {
  memory_stream target;
  target.input = "abc";
  cxx11::basic_streambuf buffer(target);
  ASSERT_EQ(buffer.sgetc(), 'a');

  target.fail_seek = true;
  EXPECT_EQ(buffer.pubseekpos(0, std::ios_base::in), std::streampos(-1));
  EXPECT_EQ(buffer.sgetc(), std::char_traits<char>::eof());
}

TEST(DefaultStreamBuf, OutputSeekFlushesAndDoesNotSeekAfterFlushFailure) {
  memory_stream target;
  target.max_write = 0;
  cxx11::basic_streambuf buffer(target);
  ASSERT_EQ(buffer.sputn("x", 1), 1);

  EXPECT_EQ(buffer.pubseekoff(0, std::ios_base::beg, std::ios_base::out),
            std::streampos(-1));
  EXPECT_EQ(target.seek_calls, 0);

  target.max_write = (std::numeric_limits<buf_size>::max)();
  EXPECT_EQ(buffer.pubseekoff(0, std::ios_base::beg, std::ios_base::out),
            std::streampos(0));
  EXPECT_EQ(target.output, "x");
  EXPECT_EQ(target.seek_calls, 1);
}

TEST(DefaultStreamBuf, PropagatesSeekException) {
  memory_stream target;
  target.throw_on_seek = true;
  cxx11::basic_streambuf buffer(target);

  EXPECT_THROW(buffer.pubseekoff(0, std::ios_base::beg, std::ios_base::in),
               cxx11::io_exception);
}

TEST(DefaultStreamBuf, DestructorFlushesPendingOutput) {
  memory_stream target;
  {
    cxx11::basic_streambuf buffer(target);
    ASSERT_EQ(buffer.sputn("pending", 7), 7);
    EXPECT_TRUE(target.output.empty());
  }
  EXPECT_EQ(target.output, "pending");
}

TEST(ShortWrite, BufferedStreamRetriesPartialProgress) {
  partial_stream target;
  cxx11::basic_streambuf buffer(target);
  ASSERT_EQ(buffer.sputn("abcde", 5), 5);
  EXPECT_EQ(buffer.pubsync(), 0);
  EXPECT_EQ(target.written, "abcde");
  EXPECT_EQ(target.calls, 3);
}

TEST(ShortWrite, BufferedStreamRetainsOnlyUnwrittenBytesAfterStall) {
  partial_stream target;
  target.stall = true;
  cxx11::basic_streambuf buffer(target);
  ASSERT_EQ(buffer.sputn("abcde", 5), 5);
  EXPECT_EQ(buffer.pubsync(), -1);
  EXPECT_EQ(target.written, "ab");
  EXPECT_EQ(target.calls, 2);
  EXPECT_EQ(buffer.pubsync(), 0);
  EXPECT_EQ(target.written, "abcde");
}

TEST(ShortWrite, BufferedStreamRetainsOnlyUnwrittenBytesAfterException) {
  partial_stream target;
  target.fail = true;
  cxx11::basic_streambuf buffer(target);
  ASSERT_EQ(buffer.sputn("abcde", 5), 5);
  EXPECT_THROW(buffer.pubsync(), cxx11::io_exception);
  EXPECT_EQ(target.written, "ab");
  EXPECT_EQ(buffer.pubsync(), 0);
  EXPECT_EQ(target.written, "abcde");
}

TEST(FileBufComparison, WriteAndRead) {
  auto filename = "testfile.txt";
  const std::string data = "Hello, filebuf!";

  // Write using std::filebuf
  {
    std::filebuf fb;
    fb.open(filename, std::ios::out | std::ios::trunc);
    fb.sputn(data.data(), data.size());
    fb.close();
  }

  // Read using our buffered_streambuf
  std::string custom_read(data.size(), '\0');
  {
    std::filebuf impl;
    impl.open(filename, std::ios::in);
    cxx11::stream_streambuf adapter(&impl);
    cxx11::basic_streambuf fb(adapter);
    std::istream is(&fb);
    is.read(&custom_read[0], data.size());
  }

  // Read using std::filebuf
  std::string std_read(data.size(), '\0');
  {
    std::filebuf fb;
    fb.open(filename, std::ios::in);
    std::istream is(&fb);
    is.read(&std_read[0], data.size());
    fb.close();
  }

  EXPECT_EQ(data, custom_read);
  EXPECT_EQ(data, std_read);
}

TEST(DefaultStreamBuf, WriteReadSeekFlush) {
  auto filename = "default_streambuf_api_test.txt";
  const std::string data = "API test for default_streambuf!";

  // Write using default_streambuf
  {
    std::filebuf fb;
    fb.open(filename, std::ios::out | std::ios::trunc);
    std::ostream os(&fb);
    os << data;
    os.flush();
  }

  // Read using buffered_streambuf
  std::string read_data(data.size(), '\0');
  {
    std::filebuf fb;
    fb.open(filename, std::ios::in);
    cxx11::stream_streambuf adapter(&fb);
    cxx11::basic_streambuf buf(adapter);
    std::istream is(&buf);

    // Test seekg/seekoff/seekpos
    is.seekg(0, std::ios::end);
    std::streampos end_pos = is.tellg();
    EXPECT_EQ(static_cast<size_t>(end_pos), data.size());

    is.seekg(0, std::ios::beg);
    EXPECT_EQ(is.tellg(), 0);

    // Test read
    is.read(&read_data[0], data.size());
    EXPECT_EQ(is.gcount(), data.size());

    // Test underflow (should be EOF)
    int c = buf.sgetc();
    EXPECT_EQ(c, std::char_traits<char>::eof());
  }

  EXPECT_EQ(read_data, data);
}

#if 0
TEST(DefaultStreamBuf, WriteReadSeekFlush2) {
  auto filename = "/root/default_streambuf_api_test2.txt";
  const std::string data = "API test for default_streambuf!";

  // Write using default_streambuf
  {
    std::filebuf fb;
    fb.open(filename, std::ios::out);
    auto size = fb.sputn(data.data(), data.size());
    EXPECT_EQ(0, size);
    const int err = fb.pubsync(); // flushes the buffer to the file
    EXPECT_EQ(0, err);
    const auto ret = fb.close();
    EXPECT_EQ(nullptr, ret);
  }
  {
    std::filebuf impl;
    impl.open(filename, std::ios::out);
    cxx11::stream_streambuf adapter(&impl);
    cxx11::basic_streambuf fb(&adapter);
    auto size = fb.sputn(data.data(), data.size());
    EXPECT_EQ(0, size);
    const int err = fb.pubsync(); // flushes the buffer to the file
    EXPECT_EQ(0, err);
    // no exception
  }
}
#endif

TEST(DefaultStreamBuf, WriteReadSeekFlush3) {
  auto filename = "/root/default_streambuf_api_test3.txt";
  std::string data = "API test for default_streambuf!";

  // Write using default_streambuf
  {
    std::filebuf fb;
    fb.open(filename, std::ios::in);
    auto size = fb.sgetn(&data[0], data.size());
    EXPECT_EQ(0, size);
    const auto ret = fb.close();
    EXPECT_EQ(nullptr, ret);
  }
  {
    std::filebuf impl;
    impl.open(filename, std::ios::in);
    cxx11::stream_streambuf adapter(&impl);
    cxx11::basic_streambuf fb(adapter);
    auto size = fb.sgetn(&data[0], data.size());
    EXPECT_EQ(0, size);
    // no exception
  }
}

TEST(DefaultStreamBuf, WriteReadSeekFlush4) {
  auto filename = "default_streambuf_api_test.txt";
  const std::string data = "API test for default_streambuf!";

  // Write using default_streambuf
  {
    std::filebuf fb;
    std::ostream os(&fb);
    os << data;
    os.flush();
  }
}

class MyStreamBuf1 : public std::streambuf {
 protected:
  int_type underflow() override { throw std::runtime_error("here"); }
};

class MyStreamBuf2 : public std::streambuf {
 protected:
  int_type underflow() override { throw std::ios_base::failure("here"); }
};

TEST(DefaultStreamBuf, WriteReadSeekFlush5) {
  {
    MyStreamBuf1 rdbuf;
    std::ostringstream oss;
    EXPECT_TRUE(oss.exceptions() == std::ios_base::goodbit);

    try {
      oss << &rdbuf;
    } catch (const std::runtime_error&) {
      EXPECT_TRUE(false);  // this should not be reached!
    }

    EXPECT_TRUE(oss.rdstate() == std::ios_base::failbit);
  }
  {
    MyStreamBuf1 impl;
    cxx11::stream_streambuf adapter(&impl);
    cxx11::basic_streambuf rdbuf(adapter);
    std::ostringstream oss;
    EXPECT_TRUE(oss.exceptions() == std::ios_base::goodbit);

    try {
      oss << &rdbuf;
    } catch (const std::runtime_error&) {
      EXPECT_TRUE(false);  // this should not be reached!
    }

    EXPECT_TRUE(oss.rdstate() == std::ios_base::failbit);
  }
  {
    MyStreamBuf2 rdbuf;
    std::ostringstream oss;
    EXPECT_TRUE(oss.exceptions() == std::ios_base::goodbit);

    try {
      oss << &rdbuf;
    } catch (const std::runtime_error&) {
      EXPECT_TRUE(false);  // this should not be reached!
    }

    EXPECT_TRUE(oss.rdstate() == std::ios_base::failbit);
  }
  {
    MyStreamBuf2 impl;
    cxx11::stream_streambuf adapter(&impl);
    cxx11::basic_streambuf rdbuf(adapter);
    std::ostringstream oss;
    EXPECT_TRUE(oss.exceptions() == std::ios_base::goodbit);

    try {
      oss << &rdbuf;
    } catch (const std::runtime_error&) {
      EXPECT_TRUE(false);  // this should not be reached!
    }

    EXPECT_TRUE(oss.rdstate() == std::ios_base::failbit);
  }
}

class error_streambuf : public std::streambuf {
 protected:
  int_type underflow() override { return traits_type::eof(); }

  int_type overflow(int_type = traits_type::eof()) override {
    return traits_type::eof();
  }

  int sync() override { return -1; }
#if 0
  pos_type seekoff(off_type, std::ios_base::seekdir,
                   std::ios_base::openmode) override {
    return pos_type(off_type(-1));
  }
  pos_type seekpos(pos_type, std::ios_base::openmode) override {
    return pos_type(off_type(-1));
  }
#endif
};

TEST(DefaultStreamBuf, WriteReadSeekFlush6) {
  {
    error_streambuf rdbuf;
    EXPECT_TRUE(rdbuf.sbumpc() == std::streambuf::traits_type::eof());
    EXPECT_TRUE(rdbuf.sputc(' ') == std::streambuf::traits_type::eof());
    EXPECT_TRUE(rdbuf.pubsync() == -1);

    // Seek by offset from beginning
    auto result = rdbuf.pubseekoff(1234, std::ios_base::beg,
                                   std::ios_base::in | std::ios_base::out);
    EXPECT_TRUE(result == -1);

    // Seek to absolute position
    auto result2 =
        rdbuf.pubseekpos(4567, std::ios_base::in | std::ios_base::out);
    EXPECT_TRUE(result2 == -1);
  }
  {
    error_streambuf impl;
    cxx11::stream_streambuf adapter(&impl);
    // default_streambuf rdbuf(&adapter);
    cxx11::basic_streambuf rdbuf(adapter, 1);

    EXPECT_TRUE(rdbuf.sbumpc() == std::streambuf::traits_type::eof());
    // EXPECT_TRUE(rdbuf.sputc(' ') == std::streambuf::traits_type::eof());
    {
      // because of buffering we cannot simply check the return value of 'sputc'
      std::ostringstream oss;
      rdbuf.sputc(' ');
      oss << &rdbuf;
      auto str = oss.str();
      EXPECT_TRUE(str.empty());
    }
    EXPECT_TRUE(rdbuf.pubsync() == -1);

    // Seek by offset from beginning
    auto result = rdbuf.pubseekoff(1234, std::ios_base::beg,
                                   std::ios_base::in | std::ios_base::out);
    EXPECT_TRUE(result == -1);

    // Seek to absolute position
    auto result2 =
        rdbuf.pubseekpos(4567, std::ios_base::in | std::ios_base::out);
    EXPECT_TRUE(result2 == -1);
  }
}
