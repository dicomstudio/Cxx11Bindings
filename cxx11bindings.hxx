#pragma once
#include "cxx11bindings.h"
#include "cxx11exceptions.hxx"

namespace cxx11 {
class stream_interface {
 public:
  stream_interface(const stream_interface& other) = delete;
  stream_interface(stream_interface&& other) noexcept = delete;
  stream_interface& operator=(const stream_interface& other) = delete;
  stream_interface& operator=(stream_interface&& other) noexcept = delete;

  stream_interface() = default;
  virtual ~stream_interface() = default;

  virtual int read(byte* buf, size count) = 0;
  virtual int write(const byte* buf, size count) = 0;
  virtual offset seek(offset off, seek_dir dir) = 0;
  virtual int flush() = 0;

  virtual int trunc(length size) {
    return static_cast<int>(CxxExceptionCode::NotImplemented);
  }
};

class c_stream final : public stream_interface {
  c11_stream* c11_stream_;

 public:
  c_stream(const c_stream& other) = delete;
  c_stream(c_stream&& other) noexcept = delete;
  c_stream& operator=(const c_stream& other) = delete;
  c_stream& operator=(c_stream&& other) noexcept = delete;

  explicit c_stream(c11_stream* c11_stream) : stream_interface() {
    c11_stream_ = c11_stream;
  }

  ~c_stream() override = default;

  int read(byte* buf, const size count) override {
    if (c11_stream_->read) {
      return c11_stream_->read(buf, count);
    }
    return static_cast<int>(CxxExceptionCode::NotImplemented);
  }

  int write(const byte* buf, const size count) override {
    if (c11_stream_->write) {
      return c11_stream_->write(buf, count);
    }
    return static_cast<int>(CxxExceptionCode::NotImplemented);
  }

  offset seek(const offset off, const seek_dir dir) override {
    if (c11_stream_->seek) {
      return c11_stream_->seek(off, dir);
    }
    return static_cast<offset>(CxxExceptionCode::NotImplemented);
  }

  int flush() override {
    if (c11_stream_->flush) {
      return c11_stream_->flush();
    }
    return static_cast<int>(CxxExceptionCode::NotImplemented);
  }

  int trunc(const length size) override {
    if (c11_stream_->trunc) {
      return c11_stream_->trunc(size);
    }
    return static_cast<int>(CxxExceptionCode::NotImplemented);
  }
};

/* Virtual buffer. No allocation done, simply provides
 * access to the virtual buffer memory */
class virtual_buffer {
  byte* data_;
  size count_;

 public:
  explicit virtual_buffer(byte* data, const size count)
      : data_(data), count_(count) {
    if (count < 1 || count > 0x7ffff000) {
      throw std::runtime_error("invalid size");
    }
  }

  char* data() const { return reinterpret_cast<char*>(data_); }
  byte* bytes() const { return data_; }
  size count() const { return count_; }
};

// https://stackoverflow.com/questions/14086417/how-to-write-custom-input-stream-in-c
// https://stackoverflow.com/questions/22116158/whats-wrong-with-this-stream-buffer
class simple_streambuf final : public std::streambuf {
  virtual_buffer buffer_;
  c11_stream* c11_stream_;

 public:
  simple_streambuf(const simple_streambuf& other) = delete;
  simple_streambuf(simple_streambuf&& other) noexcept = delete;
  simple_streambuf& operator=(const simple_streambuf& other) = delete;
  simple_streambuf& operator=(simple_streambuf&& other) noexcept = delete;

  explicit simple_streambuf(const virtual_buffer& buffer,
                            c11_stream* c11_stream)
      : buffer_(buffer), c11_stream_(c11_stream) {
    // -1 trick:
    this->setp(this->buffer_.data(),
               this->buffer_.data() + this->buffer_.count() - 1);
  }

  ~simple_streambuf() override { sync(); }

 private:
  int_type overflow(const int_type i) override {
    if (!traits_type::eq_int_type(i, traits_type::eof())) {
      // see -1 trick in cstor:
      *pptr() = traits_type::to_char_type(i);
      pbump(1);

      if (sync_impl()) {
        // pbump(-(pptr() - pbase()));
        pbump(-static_cast<int>(this->buffer_.count()));
        return i;
      }
      return traits_type::eof();
    }
    return traits_type::not_eof(i);
  }

  // O on success, -1 on error
  int sync() override {
    if (!sync_impl()) {
      return -1;
    }
    // bubble up to managed layer:
    const int ret = c11_stream_->flush();
    // flush_func_ is: -1 on error, 0 on success:
    return ret;
  }

  // helper:
  bool sync_impl() {
    const int num = static_cast<int>(pptr() - pbase());
    // const int size = this->buffer_.write(num);
    const auto buf = reinterpret_cast<const byte*>(this->buffer_.data());
    const int size = c11_stream_->write(buf, num);
    if (size < 0) {
      throw std::ios_base::failure("there");
    }
    return size == num;
  }

  // Override seekoff for seeking by offset
  pos_type seekoff(const off_type off, const std::ios_base::seekdir dir,
                   std::ios_base::openmode which) override {
    const offset ret = c11_stream_->seek(static_cast<offset>(off), dir);
    return ret;
  }

  // Override seekpos for seeking to an absolute position
  pos_type seekpos(const pos_type pos, std::ios_base::openmode which) override {
    const offset ret =
        c11_stream_->seek(static_cast<offset>(pos), std::ios_base::beg);
    return ret;
  }

  // fetch more data:
  int_type underflow() override {
    if (this->gptr() == this->egptr()) {
      // const int size = this->buffer_.read();
      const auto buf = reinterpret_cast<byte*>(this->buffer_.data());
      const auto count = this->buffer_.count();
      const auto size = c11_stream_->read(buf, count);
      if (size < 0) {
        // https://developercommunity.visualstudio.com/t/Exception-from-streambuf-should-be-caugh/10555755
        // https://stackoverflow.com/a/77704741/136285
        // https://github.com/microsoft/STL/issues/4322
        throw std::ios_base::failure("here");
      }
      this->setg(this->buffer_.data(), this->buffer_.data(),
                 this->buffer_.data() + size);
    }
    return this->gptr() == this->egptr()
               ? traits_type::eof()
               : traits_type::to_int_type(*this->gptr());
  }
};

template <int N = 4096>
class buffered_streambuf final : public std::streambuf {
  stream_interface* stream_;
  char buffer_[N];

 public:
  buffered_streambuf(const buffered_streambuf& other) = delete;
  buffered_streambuf(buffered_streambuf&& other) noexcept = delete;
  buffered_streambuf& operator=(const buffered_streambuf& other) = delete;
  buffered_streambuf& operator=(buffered_streambuf&& other) noexcept = delete;

  explicit buffered_streambuf(stream_interface* stream = nullptr)
      : stream_(stream) {
    if (!stream_) {
      throw null_pointer();
    }
    setg(buffer_, buffer_, buffer_);
    setp(buffer_, buffer_ + sizeof(buffer_));
  }

  ~buffered_streambuf() override {
    if (stream_) {
      close();
    }
  }

#if 0
    bool open(const char* filename, const char* mode) {
        file_ = std::fopen(filename, mode);
        return file_ != nullptr;
    }

    void close() {
        sync();
        if (file_) std::fclose(file_);
        file_ = nullptr;
    }
#else
  void close() {
    sync();
    // FIXME: what if flush returns error?
    if (stream_) {
      stream_->flush();
    }
    stream_ = nullptr;
  }
#endif

 protected:
  int_type underflow() override {
    if (!stream_) {
      throw null_pointer();
    }
#if 0
    size_t n = std::fread(buffer_, 1, sizeof(buffer_), file_);
#else
    auto n = stream_->read(reinterpret_cast<byte*>(buffer_), sizeof(buffer_));
    throw_exception_from_int(n, "read error");
#endif
    if (n == 0) {
      return traits_type::eof();
    }
    setg(buffer_, buffer_, buffer_ + n);
    return traits_type::to_int_type(*gptr());
  }

  int_type overflow(const int_type ch = traits_type::eof()) override {
    if (!stream_) {
      throw null_pointer();
    }
    if (pptr() == pbase()) {
      return traits_type::eof();
    }
#if 0
    size_t n = pptr() - pbase();
        size_t written = std::fwrite(pbase(), 1, n, file_);
#else
    auto n = pptr() - pbase();
    auto written = stream_->write(reinterpret_cast<byte*>(pbase()), n);
    throw_exception_from_int(written, "write error");
#endif
    setp(buffer_, buffer_ + sizeof(buffer_));
    if (ch != traits_type::eof()) {
      *pptr() = traits_type::to_char_type(ch);
      pbump(1);
    }
    return written == n ? ch : traits_type::eof();
  }

  int sync() override {
    if (!stream_) {
      throw null_pointer();
    }
    return stream_->flush();
  }

  std::streamsize xsputn(const char_type* s, const std::streamsize n) override {
    // by default sputn is optimized for small writes, we must implement this to
    // trigger edge cases
    if (!stream_) {
      throw null_pointer();
    }
    const auto written = stream_->write(reinterpret_cast<const byte*>(s), n);
    throw_exception_from_int(written, "write error 2");
    if (written > 0) {
      pbump(static_cast<int>(written));
    }
    return written;
  }

  pos_type seekoff(const off_type off, const std::ios_base::seekdir way,
                   const std::ios_base::openmode which) override {
    if (!stream_) {
      throw null_pointer();
    }
    (void)which;
#if 0
    int whence;
    if (way == std::ios_base::beg) whence = SEEK_SET;
    else if (way == std::ios_base::cur) whence = SEEK_CUR;
    else if (way == std::ios_base::end) whence = SEEK_END;
    else
      throw std::ios_base::failure("invalid dir");
        if (std::fseek(file_, off, whence) != 0) return pos_type(-1);
        return std::ftell(file_);
#else
    int whence;
    if (way == std::ios_base::beg) {
      whence = seek_dirs::seek_beg;
    } else if (way == std::ios_base::cur) {
      whence = seek_dirs::seek_cur;
    } else if (way == std::ios_base::end) {
      whence = seek_dirs::seek_end;
    } else {
      // throw std::invalid_argument("invalid way");
      return -1;
    }
    const auto pos = stream_->seek(off, whence);
    // throw_exception_from_long_long(pos, "seekoff error");
    //  must return {-1} if seek operation fails
    return pos;
#endif
  }

  pos_type seekpos(const pos_type pos,
                   const std::ios_base::openmode which) override {
#if 0
        if (std::fseek(file_, pos, SEEK_SET) != 0) return pos_type(-1);
        return std::ftell(file_);
#else
    const auto new_pos = stream_->seek(pos, seek_dirs::seek_beg);
    // throw_exception_from_long_long(new_pos, "seekpos error");
    // must return {-1} if seek operation fails
    return new_pos;
#endif
  }
};

// no internal buffering, rely on DefaultStreamInterface implementation detail
// simply forwards calls to the DefaultStreamInterface methods
typedef buffered_streambuf<> default_streambuf;
typedef buffered_streambuf<1> nobuffer_streambuf;
}  // namespace cxx11
