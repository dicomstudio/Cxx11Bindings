#include "stream_interface_impl.hxx"

#include <cxx11bindings/filebuf.hxx>
#include <fstream>  // std::filebuf
#include <gtest/gtest.h>
#include <sstream>

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
