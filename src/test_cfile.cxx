#include "stream_interface_impl.hxx"

#include <cstdio>
#include <gtest/gtest.h>
#include <istream>
#include <memory>
#include <ostream>
#include <string>

struct file_deleter {
  void operator()(FILE* file) const {
    if (file) {
      const int result = std::fclose(file);
      (void)result;
    }
  }
};

TEST(CFileStream, ReadWriteSeekFlushAndTruncate) {
  std::unique_ptr<FILE, file_deleter> file(std::tmpfile());
  ASSERT_NE(file.get(), nullptr);
  cxx11::cfile_stream stream(file.get());

  const byte data[] = {'a', 0, 'b', 'c'};
  ASSERT_EQ(stream.write(data, sizeof data), sizeof data);
  EXPECT_EQ(stream.flush(), 0);
  EXPECT_EQ(stream.seek(1, seek_dirs::seek_beg), 1);
  EXPECT_EQ(stream.seek(1, seek_dirs::seek_cur), 2);
  EXPECT_EQ(stream.seek(0, seek_dirs::seek_beg), 0);

  byte readback[sizeof data] = {};
  EXPECT_EQ(stream.read(readback, sizeof readback), sizeof data);
  EXPECT_EQ(std::string(reinterpret_cast<char*>(readback), sizeof readback),
            std::string(reinterpret_cast<const char*>(data), sizeof data));

  EXPECT_EQ(stream.seek(0, seek_dirs::seek_end), sizeof data);
  EXPECT_EQ(stream.trunc(2), 2);
  EXPECT_EQ(stream.seek(0, seek_dirs::seek_end), 2);
  EXPECT_EQ(stream.read(readback, sizeof readback), 0);
}

TEST(CFileStream, TruncateDiscardsBufferedInputAndPreservesPosition) {
  std::unique_ptr<FILE, file_deleter> file(std::tmpfile());
  ASSERT_NE(file.get(), nullptr);
  cxx11::cfile_stream stream(file.get());
  const byte data[] = {'a', 'b', 'c', 'd'};
  ASSERT_EQ(stream.write(data, sizeof data), sizeof data);
  ASSERT_EQ(stream.seek(0, seek_dirs::seek_beg), 0);

  byte readback[sizeof data] = {};
  ASSERT_EQ(stream.read(readback, 1), 1);
  ASSERT_EQ(readback[0], 'a');
  ASSERT_EQ(stream.trunc(2), 2);
  EXPECT_EQ(stream.seek(0, seek_dirs::seek_cur), 1);
  EXPECT_EQ(stream.read(readback, sizeof readback), 1);
  EXPECT_EQ(readback[0], 'b');
  EXPECT_EQ(stream.read(readback, sizeof readback), 0);
  ASSERT_EQ(stream.seek(0, seek_dirs::seek_beg), 0);
  ASSERT_EQ(stream.read(readback, sizeof readback), 2);
  EXPECT_EQ(readback[0], 'a');
  EXPECT_EQ(readback[1], 'b');
}

TEST(CFileStream, RejectsNullAndInvalidArguments) {
  EXPECT_THROW(cxx11::cfile_stream(nullptr), cxx11::null_pointer);

  std::unique_ptr<FILE, file_deleter> file(std::tmpfile());
  ASSERT_NE(file.get(), nullptr);
  cxx11::cfile_stream stream(file.get());
  byte value = 0;

  EXPECT_THROW(stream.read(nullptr, 1), cxx11::null_pointer);
  EXPECT_THROW(stream.read(&value, -1), cxx11::invalid_argument);
  EXPECT_THROW(stream.write(nullptr, 1), cxx11::null_pointer);
  EXPECT_THROW(stream.write(&value, -1), cxx11::invalid_argument);
  EXPECT_THROW(stream.seek(0, static_cast<seek_dir>(-1)),
               cxx11::invalid_argument);
  EXPECT_THROW(stream.trunc(-1), cxx11::invalid_argument);
}

TEST(CFileStream, WorksThroughBasicStreambuf) {
  std::unique_ptr<FILE, file_deleter> file(std::tmpfile());
  ASSERT_NE(file.get(), nullptr);
  cxx11::cfile_stream stream(file.get());
  cxx11::basic_streambuf buffer(stream, 4);
  const std::string data = "FILE stream adapter";

  std::ostream output(&buffer);
  output.write(data.data(), static_cast<std::streamsize>(data.size()));
  output.flush();
  ASSERT_TRUE(output.good());
  ASSERT_EQ(buffer.pubseekoff(0, std::ios_base::beg, std::ios_base::in),
            std::streampos(0));

  std::string readback(data.size(), '\0');
  std::istream input(&buffer);
  input.read(&readback[0], static_cast<std::streamsize>(readback.size()));
  EXPECT_EQ(input.gcount(), static_cast<std::streamsize>(data.size()));
  EXPECT_EQ(readback, data);
}

TEST(NativeStream, ReadsFromExistingHandleOrDescriptor) {
  std::unique_ptr<FILE, file_deleter> file(std::tmpfile());
  ASSERT_NE(file.get(), nullptr);
  const std::string data = "native stream adapter";
  ASSERT_EQ(std::fwrite(data.data(), 1, data.size(), file.get()), data.size());
  ASSERT_EQ(std::fflush(file.get()), 0);
  ASSERT_EQ(std::fseek(file.get(), 0, SEEK_SET), 0);

#ifdef _WIN32
  HANDLE handle = reinterpret_cast<HANDLE>(_get_osfhandle(_fileno(file.get())));
  ASSERT_NE(handle, INVALID_HANDLE_VALUE);
  cxx11::handle_stream adapter(handle);
#else
  cxx11::fd_stream adapter(fileno(file.get()));
#endif
  cxx11::basic_streambuf buffer(adapter, 4);
  std::string readback(data.size(), '\0');
  std::istream input(&buffer);
  input.read(&readback[0], static_cast<std::streamsize>(readback.size()));

  EXPECT_EQ(input.gcount(), static_cast<std::streamsize>(data.size()));
  EXPECT_EQ(readback, data);
}
