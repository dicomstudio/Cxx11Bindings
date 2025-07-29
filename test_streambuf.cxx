#include "stream_interface_impl.hxx"

#include <fstream>  // std::filebuf
#include <gtest/gtest.h>

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

  // Read using your filebuf_nobuffer
  std::string custom_read(data.size(), '\0');
  {
    std::filebuf impl;
    impl.open(filename, std::ios::in);
    cxx11::stream_streambuf adapter(&impl);
    cxx11::buffered_streambuf fb(&adapter);
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

  EXPECT_EQ(custom_read, std_read);
  EXPECT_EQ(std_read, data);
}

#include <sstream>

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

  // Read using default_streambuf
  std::string read_data(data.size(), '\0');
  {
    std::filebuf fb;
    fb.open(filename, std::ios::in);
    cxx11::stream_streambuf adapter(&fb);
    cxx11::buffered_streambuf buf(&adapter);
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

TEST(DefaultStreamBuf, WriteReadSeekFlush2) {
  auto filename = "/root/default_streambuf_api_test2.txt";
  const std::string data = "API test for default_streambuf!";

  // Write using default_streambuf
  {
    std::filebuf fb;
    fb.open(filename, std::ios::out);
    auto size = fb.sputn(data.data(), data.size());
    EXPECT_EQ(0, size);
    const int err = fb.pubsync();  // flushes the buffer to the file
    EXPECT_EQ(0, err);
    const auto ret = fb.close();
    EXPECT_EQ(nullptr, ret);
  }
  {
    std::filebuf impl;
    impl.open(filename, std::ios::out);
    cxx11::stream_streambuf adapter(&impl);
    cxx11::buffered_streambuf fb(&adapter);
    auto size = fb.sputn(data.data(), data.size());
    EXPECT_EQ(0, size);
    const int err = fb.pubsync();  // flushes the buffer to the file
    EXPECT_EQ(0, err);
    // no exception
  }
}

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
    cxx11::buffered_streambuf fb(&adapter);
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
    cxx11::buffered_streambuf rdbuf(&adapter);
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
    cxx11::buffered_streambuf rdbuf(&adapter);
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
    cxx11::nobuffer_streambuf rdbuf(&adapter);

    EXPECT_TRUE(rdbuf.sbumpc() == std::streambuf::traits_type::eof());
    // EXPECT_TRUE(rdbuf.sputc(' ') == std::streambuf::traits_type::eof());
    {
      // because of buffering we cannot simply check the return value of 'sputc'
      std::ostringstream oss;
      rdbuf.sputc(' ');
      oss << &rdbuf;
      auto str = oss.str();
      EXPECT_TRUE(str.size() == 0);
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
