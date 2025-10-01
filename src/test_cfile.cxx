#include "stream_interface_impl.hxx"

#include <fstream>  // std::filebuf
#include <gtest/gtest.h>

TEST(FileComparison0, WriteAndRead) {
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
#ifdef _WIN32
  {
    HANDLE impl = CreateFileA(filename, GENERIC_READ, 0, nullptr, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
    cxx11::handle_stream adapter(impl);
    cxx11::basic_streambuf fb(adapter);
    std::istream is(&fb);
    is.read(&custom_read[0], data.size());
    CloseHandle(impl);
  }
#else
  {
    int impl;
    impl = open(filename, O_RDONLY);  // Open for reading
    cxx11::fd_stream adapter(impl);
    cxx11::basic_streambuf fb(adapter);
    std::istream is(&fb);
    is.read(&custom_read[0], data.size());
    close(impl);
  }
#endif

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

TEST(FileComparison, WriteAndRead) {
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
    FILE* impl;
    impl = fopen(filename, "rb");
    cxx11::cfile_stream adapter(impl);
    cxx11::basic_streambuf fb(adapter);
    std::istream is(&fb);
    is.read(&custom_read[0], data.size());
    fclose(impl);
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
