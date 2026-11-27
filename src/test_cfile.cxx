#include "stream_interface_impl.hxx"

#include <cerrno>
#include <cstdio>
#include <gtest/gtest.h>
#include <istream>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

#ifndef _WIN32
#include <fcntl.h>
#include <unistd.h>
#endif

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

TEST(CFileStream, ZeroLengthOperationsAndEndOfFile) {
  std::unique_ptr<FILE, file_deleter> file(std::tmpfile());
  ASSERT_NE(file.get(), nullptr);
  cxx11::cfile_stream stream(file.get());
  byte value = 'x';

  EXPECT_EQ(stream.write(&value, 0), 0);
  EXPECT_EQ(stream.read(&value, 0), 0);
  EXPECT_EQ(stream.read(&value, 1), 0);
  EXPECT_TRUE(std::feof(file.get()));
  EXPECT_EQ(stream.seek(0, seek_dirs::seek_beg), 0);
  EXPECT_FALSE(std::feof(file.get()));
  ASSERT_EQ(stream.write(&value, 1), 1);
  ASSERT_EQ(stream.seek(0, seek_dirs::seek_beg), 0);
  EXPECT_EQ(stream.read(&value, 1), 1);
  EXPECT_EQ(value, 'x');
  EXPECT_EQ(stream.read(&value, 1), 0);
}

TEST(CFileStream, TruncateFlushesOutputAndPreservesPosition) {
  std::unique_ptr<FILE, file_deleter> file(std::tmpfile());
  ASSERT_NE(file.get(), nullptr);
  cxx11::cfile_stream stream(file.get());
  const byte data[] = {'a', 'b', 'c', 'd'};
  ASSERT_EQ(stream.write(data, sizeof data), sizeof data);
  ASSERT_EQ(stream.trunc(2), 2);
  EXPECT_EQ(stream.seek(0, seek_dirs::seek_cur), 4);
  ASSERT_EQ(stream.seek(0, seek_dirs::seek_end), 2);
  ASSERT_EQ(stream.trunc(5), 5);
  EXPECT_EQ(stream.seek(0, seek_dirs::seek_cur), 2);
  ASSERT_EQ(stream.seek(0, seek_dirs::seek_beg), 0);
  byte readback[5] = {};
  ASSERT_EQ(stream.read(readback, sizeof readback), sizeof readback);
  EXPECT_EQ(readback[0], 'a');
  EXPECT_EQ(readback[1], 'b');
  EXPECT_EQ(readback[2], 0);
  EXPECT_EQ(readback[3], 0);
  EXPECT_EQ(readback[4], 0);
  EXPECT_EQ(stream.trunc(0), 0);
  ASSERT_EQ(stream.seek(0, seek_dirs::seek_end), 0);
  EXPECT_EQ(stream.read(readback, 1), 0);
}

TEST(CFileStream, SeekBeforeBeginningReportsInvalidArgument) {
  std::unique_ptr<FILE, file_deleter> file(std::tmpfile());
  ASSERT_NE(file.get(), nullptr);
  cxx11::cfile_stream stream(file.get());
  EXPECT_THROW(stream.seek(-1, seek_dirs::seek_beg), cxx11::invalid_argument);
  EXPECT_EQ(stream.seek(0, seek_dirs::seek_cur), 0);
}

TEST(CFileStreamHelpers, RejectNullStreams) {
  byte value = 0;
  EXPECT_EQ(fread_file_fp(nullptr, &value, 1), C11_E_POINTER);
  EXPECT_EQ(fwrite_file_fp(nullptr, &value, 1), C11_E_POINTER);
  EXPECT_EQ(fseek_file_fp(nullptr, 0, seek_dirs::seek_beg), C11_E_POINTER);
  EXPECT_EQ(ftell_file_fp(nullptr), C11_E_POINTER);
  EXPECT_EQ(fflush_file_fp(nullptr), C11_E_POINTER);
  EXPECT_EQ(ftruncate_file_fp(nullptr, 0), C11_E_POINTER);
}

#ifndef _WIN32
TEST(CFileStream, NonseekableFileReportsErrors) {
  int descriptors[2];
  ASSERT_EQ(pipe(descriptors), 0);
  std::unique_ptr<FILE, file_deleter> file(fdopen(descriptors[0], "r"));
  if (!file) {
    close(descriptors[0]);
  }
  ASSERT_EQ(close(descriptors[1]), 0);
  ASSERT_NE(file.get(), nullptr);
  cxx11::cfile_stream stream(file.get());

  EXPECT_THROW(stream.seek(0, seek_dirs::seek_cur), cxx11::io_exception);
  EXPECT_EQ(ftell_file_fp(file.get()), C11_E_IO);
  const int64_t result = ftruncate_file_fp(file.get(), 1);
  ASSERT_TRUE(result == C11_E_IO || result == C11_E_NOTSUPPORTED);
  if (result == C11_E_IO) {
    EXPECT_THROW(stream.trunc(1), cxx11::io_exception);
  } else {
    EXPECT_THROW(stream.trunc(1), cxx11::not_supported);
  }
}

TEST(CFileStream, ReadErrorAndPartialProgress) {
  int descriptors[2];
  ASSERT_EQ(pipe(descriptors), 0);
  std::unique_ptr<FILE, file_deleter> file(fdopen(descriptors[0], "r"));
  if (!file) {
    close(descriptors[0]);
  }
  std::unique_ptr<FILE, file_deleter> writer(fdopen(descriptors[1], "w"));
  if (!writer) {
    close(descriptors[1]);
  }
  ASSERT_NE(file.get(), nullptr);
  ASSERT_NE(writer.get(), nullptr);
  const int flags = fcntl(descriptors[0], F_GETFL);
  ASSERT_NE(flags, -1);
  ASSERT_EQ(fcntl(descriptors[0], F_SETFL, flags | O_NONBLOCK), 0);
  ASSERT_EQ(write(descriptors[1], "ab", 2), 2);
  cxx11::cfile_stream stream(file.get());
  byte data[4] = {};

  EXPECT_EQ(stream.read(data, sizeof data), 2);
  EXPECT_EQ(data[0], 'a');
  EXPECT_EQ(data[1], 'b');
  EXPECT_TRUE(std::ferror(file.get()));
  EXPECT_THROW(stream.read(data, sizeof data), cxx11::io_exception);
  EXPECT_EQ(stream.read(data, 0), 0);
  std::clearerr(file.get());
  writer.reset();
  EXPECT_EQ(stream.read(data, sizeof data), 0);
  EXPECT_TRUE(std::feof(file.get()));
}

TEST(CFileStream, WriteErrorAndPartialProgress) {
  int descriptors[2];
  ASSERT_EQ(pipe(descriptors), 0);
  std::unique_ptr<FILE, file_deleter> reader(fdopen(descriptors[0], "r"));
  if (!reader) {
    close(descriptors[0]);
  }
  std::unique_ptr<FILE, file_deleter> writer(fdopen(descriptors[1], "w"));
  if (!writer) {
    close(descriptors[1]);
  }
  ASSERT_NE(reader.get(), nullptr);
  ASSERT_NE(writer.get(), nullptr);
  ASSERT_EQ(std::setvbuf(writer.get(), nullptr, _IONBF, 0), 0);
  const long pipe_buf = fpathconf(descriptors[1], _PC_PIPE_BUF);
  ASSERT_GT(pipe_buf, 0);
  ASSERT_LE(pipe_buf, INT32_MAX / 2);
  std::vector<byte> data(static_cast<size_t>(pipe_buf) * 2, 'x');
  const int flags = fcntl(descriptors[1], F_GETFL);
  ASSERT_NE(flags, -1);
  ASSERT_EQ(fcntl(descriptors[1], F_SETFL, flags | O_NONBLOCK), 0);
  ssize_t written;
  while ((written = write(descriptors[1], data.data(), pipe_buf)) > 0) {}
  ASSERT_EQ(written, -1);
  ASSERT_TRUE(errno == EAGAIN || errno == EWOULDBLOCK);
  ASSERT_EQ(read(descriptors[0], data.data(), pipe_buf), pipe_buf);

  cxx11::cfile_stream stream(writer.get());
  const buf_size partial =
      stream.write(data.data(), static_cast<buf_size>(data.size()));
  EXPECT_GT(partial, 0);
  EXPECT_LT(partial, static_cast<buf_size>(data.size()));
  EXPECT_TRUE(std::ferror(writer.get()));
  EXPECT_THROW(stream.write(data.data(), static_cast<buf_size>(pipe_buf)),
               cxx11::io_exception);
  EXPECT_EQ(stream.write(data.data(), 0), 0);
}

TEST(CFileStream, StreambufPropagatesReadError) {
  int descriptors[2];
  ASSERT_EQ(pipe(descriptors), 0);
  std::unique_ptr<FILE, file_deleter> file(fdopen(descriptors[0], "r"));
  if (!file) {
    close(descriptors[0]);
  }
  std::unique_ptr<FILE, file_deleter> writer(fdopen(descriptors[1], "w"));
  if (!writer) {
    close(descriptors[1]);
  }
  ASSERT_NE(file.get(), nullptr);
  ASSERT_NE(writer.get(), nullptr);
  const int flags = fcntl(descriptors[0], F_GETFL);
  ASSERT_NE(flags, -1);
  ASSERT_EQ(fcntl(descriptors[0], F_SETFL, flags | O_NONBLOCK), 0);

  cxx11::cfile_stream stream(file.get());
  cxx11::basic_streambuf buffer(stream, 4);
  std::istream input(&buffer);
  input.exceptions(std::ios_base::badbit);
  EXPECT_THROW(input.get(), cxx11::io_exception);
  EXPECT_TRUE(input.bad());
}

#ifdef __linux__
TEST(CFileStream, WriteAndFlushErrors) {
  std::unique_ptr<FILE, file_deleter> file(std::fopen("/dev/full", "wb"));
  if (!file) {
    GTEST_SKIP() << "/dev/full is unavailable";
  }
  cxx11::cfile_stream stream(file.get());
  byte value = 'x';

  ASSERT_EQ(stream.write(&value, 1), 1);
  EXPECT_THROW(stream.flush(), cxx11::io_exception);
  EXPECT_TRUE(std::ferror(file.get()));
  std::unique_ptr<FILE, file_deleter> unbuffered(std::fopen("/dev/full", "wb"));
  ASSERT_NE(unbuffered.get(), nullptr);
  ASSERT_EQ(std::setvbuf(unbuffered.get(), nullptr, _IONBF, 0), 0);
  cxx11::cfile_stream unbuffered_stream(unbuffered.get());
  EXPECT_THROW(unbuffered_stream.write(&value, 1), cxx11::io_exception);

  std::unique_ptr<FILE, file_deleter> truncate_file(
      std::fopen("/dev/full", "wb"));
  ASSERT_NE(truncate_file.get(), nullptr);
  cxx11::cfile_stream truncate_stream(truncate_file.get());
  ASSERT_EQ(truncate_stream.write(&value, 1), 1);
  EXPECT_THROW(truncate_stream.trunc(0), cxx11::io_exception);
}
#endif
#endif

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
