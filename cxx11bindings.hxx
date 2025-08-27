#ifndef CXX11BINDINGS_HXX
#define CXX11BINDINGS_HXX

#include "cxx11bindings.h"
#include "cxx11exceptions.hxx"

#include <vector>
#include <cstring> // memmove

namespace cxx11 {
class stream_interface {
 public:
  stream_interface(const stream_interface& other) = delete;
  stream_interface(stream_interface&& other) noexcept = delete;
  stream_interface& operator=(const stream_interface& other) = delete;
  stream_interface& operator=(stream_interface&& other) noexcept = delete;

  stream_interface() = default;
  virtual ~stream_interface() = default;

  virtual buf_size read(byte* buf, buf_size count) = 0;
  virtual buf_size write(const byte* buf, buf_size count) = 0;
  virtual stream_offset seek(stream_offset off, seek_dir dir) = 0;
  virtual int flush() = 0;

  virtual stream_length trunc(stream_length) {
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

  buf_size read(byte* buf, const buf_size count) override {
    if (c11_stream_->read) {
      return c11_stream_->read(buf, count);
    }
    return static_cast<buf_size>(CxxExceptionCode::NotImplemented);
  }

  buf_size write(const byte* buf, const buf_size count) override {
    if (c11_stream_->write) {
      return c11_stream_->write(buf, count);
    }
    return static_cast<buf_size>(CxxExceptionCode::NotImplemented);
  }

  stream_offset seek(const stream_offset off, const seek_dir dir) override {
    if (c11_stream_->seek) {
      return c11_stream_->seek(off, dir);
    }
    return static_cast<stream_offset>(CxxExceptionCode::NotImplemented);
  }

  int flush() override {
    if (c11_stream_->flush) {
      return c11_stream_->flush();
    }
    return static_cast<int>(CxxExceptionCode::NotImplemented);
  }

  stream_length trunc(const stream_length size) override {
    if (c11_stream_->trunc) {
      return c11_stream_->trunc(size);
    }
    return static_cast<stream_length>(CxxExceptionCode::NotImplemented);
  }
};

/* Virtual buffer. No allocation done, simply provides
 * access to the virtual buffer memory */
class external_buffer {
  byte* data_;
  size_t count_;

 public:
  explicit external_buffer(byte* data, const buf_size count)
      : data_(data), count_(count) {
    if (count < 1 || count > 0x7ffff000) {
      throw std::runtime_error("invalid size");
    }
  }

  char* data() const { return reinterpret_cast<char*>(data_); }
  byte* bytes() const { return data_; }
  buf_size count() const { return static_cast<buf_size>(count_); }
};

// https://stackoverflow.com/questions/14086417/how-to-write-custom-input-stream-in-c
// https://stackoverflow.com/questions/22116158/whats-wrong-with-this-stream-buffer
class external_streambuf final : public std::streambuf {
  external_buffer buffer_;
  stream_interface* c11_stream_;

 public:
  external_streambuf(const external_streambuf& other) = delete;
  external_streambuf(external_streambuf&& other) noexcept = delete;
  external_streambuf& operator=(const external_streambuf& other) = delete;
  external_streambuf& operator=(external_streambuf&& other) noexcept = delete;

  explicit external_streambuf(const external_buffer& buffer,
                              stream_interface* stream)
      : buffer_(buffer), c11_stream_(stream) {
    // -1 trick:
    this->setp(this->buffer_.data(),
               this->buffer_.data() + this->buffer_.count() - 1);
  }

  ~external_streambuf() override { sync(); }

 private:
  int_type overflow(const int_type i) override {
    if (!traits_type::eq_int_type(i, traits_type::eof())) {
      // see -1 trick in cstor:
      *pptr() = traits_type::to_char_type(i);
      pbump(1);

      if (sync_impl()) {
        // pbump(-(pptr() - pbase()));
        pbump(-this->buffer_.count());
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
    const buf_size num = static_cast<buf_size>(pptr() - pbase());
    // const int size = this->buffer_.write(num);
    const auto buf = this->buffer_.bytes();
    const auto size = c11_stream_->write(buf, num);
    if (size < 0) {
      throw std::ios_base::failure("there");
    }
    return size == num;
  }

  // Override seekoff for seeking by offset
  pos_type seekoff(const off_type off, const std::ios_base::seekdir dir,
                   std::ios_base::openmode) override {
    seek_dir cdir;
    if (dir == std::ios_base::beg) {
      cdir = seek_beg;
    } else if (dir == std::ios_base::cur) {
      cdir = seek_cur;
    } else if (dir == std::ios_base::end) {
      cdir = seek_end;
    } else {
      return {-1};
    }

    const stream_offset ret = c11_stream_->seek(off, cdir);
    return ret;
  }

  // Override seekpos for seeking to an absolute position
  pos_type seekpos(const pos_type pos, std::ios_base::openmode) override {
    const stream_offset ret = c11_stream_->seek(pos, seek_dirs::seek_beg);
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

class file_streambuf_debug : public std::streambuf {
public:
    explicit file_streambuf_debug(FILE* f, std::size_t buff_sz = 4096)
        : file(f), buffer(buff_sz + put_back_size) {
        char* base = buffer.data();
        setg(base + put_back_size, base + put_back_size, base + put_back_size);
        setp(base, base + buffer.size());
    }

    ~file_streambuf_debug() override {
        sync(); // flush pending output
    }

protected:
    // Input
    int_type underflow() override {
        if (!file) return traits_type::eof();

        // putback space
        std::size_t putback = gptr() - eback();
        if (putback > put_back_size)
            putback = put_back_size;

        std::memmove(buffer.data() + (put_back_size - putback),
                     gptr() - putback, putback);

        std::size_t n = std::fread(buffer.data() + put_back_size, 1,
                                   buffer.size() - put_back_size, file);
        if (n == 0) return traits_type::eof();

        setg(buffer.data() + (put_back_size - putback),
             buffer.data() + put_back_size,
             buffer.data() + put_back_size + n);

        return traits_type::to_int_type(*gptr());
    }

    // Output
    int_type overflow(int_type ch) override {
        if (!file) return traits_type::eof();

        if (pptr() != pbase()) {
            if (flush_buffer() == EOF)
                return traits_type::eof();
        }

        if (!traits_type::eq_int_type(ch, traits_type::eof())) {
            *pptr() = traits_type::to_char_type(ch);
            pbump(1);
        }

        return traits_type::not_eof(ch);
    }

    int sync() override {
        return (flush_buffer() == EOF) ? -1 : 0;
    }

    // Seeking
    pos_type seekoff(off_type off, std::ios_base::seekdir way,
                     std::ios_base::openmode which) override {
        if (!file) return pos_type(off_type(-1));

        // Flush output buffer before seeking
        if (which & std::ios_base::out) {
            if (sync() == -1) return pos_type(off_type(-1));
        }

        // Calculate origin
        int origin;
        switch (way) {
            case std::ios_base::beg: origin = SEEK_SET; break;
            case std::ios_base::cur: origin = SEEK_CUR; break;
            case std::ios_base::end: origin = SEEK_END; break;
            default: return pos_type(off_type(-1));
        }

        // If seeking relative to current in input mode, adjust by unread bytes
        if ((which & std::ios_base::in) && way == std::ios_base::cur) {
            off -= (egptr() - gptr());
        }

        if (std::fseek(file, off, origin) != 0)
            return pos_type(off_type(-1));

        // Invalidate buffer after seek
        setg(buffer.data() + put_back_size,
             buffer.data() + put_back_size,
             buffer.data() + put_back_size);
        setp(buffer.data(), buffer.data() + buffer.size());

        long pos = std::ftell(file);
        if (pos < 0) return pos_type(off_type(-1));
        return pos_type(pos);
    }

    pos_type seekpos(pos_type sp, std::ios_base::openmode which) override {
        return seekoff(off_type(sp), std::ios_base::beg, which);
    }

private:
    static constexpr std::size_t put_back_size = 8;
    FILE* file;
    std::vector<char> buffer;

    int flush_buffer() {
        std::ptrdiff_t n = pptr() - pbase();
        if (n > 0) {
            if (std::fwrite(pbase(), 1, n, file) != static_cast<size_t>(n))
                return EOF;
            pbump(static_cast<int>(-n));
        }
        return 0;
    }
};
class file_streambuf_debug2 : public std::streambuf {
public:
    explicit file_streambuf_debug2(stream_interface * f, std::size_t buff_sz = 4096)
        : file(f), buffer(buff_sz + put_back_size) {
        char* base = buffer.data();
        setg(base + put_back_size, base + put_back_size, base + put_back_size);
        setp(base, base + buffer.size());
    }

    ~file_streambuf_debug2() override {
        sync(); // flush pending output
    }

protected:
    // Input
    int_type underflow() override {
        if (!file) return traits_type::eof();

        // putback space
        std::size_t putback = gptr() - eback();
        if (putback > put_back_size)
            putback = put_back_size;

        std::memmove(buffer.data() + (put_back_size - putback),
                     gptr() - putback, putback);

#if 0
        std::size_t n = std::fread(buffer.data() + put_back_size, 1,
                                   buffer.size() - put_back_size, file);
#else
      std::size_t n = file->read(reinterpret_cast<byte*>(buffer.data() + put_back_size),
                           static_cast<buf_size>(buffer.size() - put_back_size) );
#endif
        if (n == 0) return traits_type::eof();

        setg(buffer.data() + (put_back_size - putback),
             buffer.data() + put_back_size,
             buffer.data() + put_back_size + n);

        return traits_type::to_int_type(*gptr());
    }

    // Output
    int_type overflow(int_type ch) override {
        if (!file) return traits_type::eof();

        if (pptr() != pbase()) {
            if (flush_buffer() == EOF)
                return traits_type::eof();
        }

        if (!traits_type::eq_int_type(ch, traits_type::eof())) {
            *pptr() = traits_type::to_char_type(ch);
            pbump(1);
        }

        return traits_type::not_eof(ch);
    }

    int sync() override {
        return (flush_buffer() == EOF) ? -1 : 0;
    }

    // Seeking
    pos_type seekoff(off_type off, std::ios_base::seekdir way,
                     std::ios_base::openmode which) override {
        if (!file) return pos_type(off_type(-1));

        // Flush output buffer before seeking
        if (which & std::ios_base::out) {
            if (sync() == -1) return pos_type(off_type(-1));
        }

        // Calculate origin
        seek_dir origin;
        switch (way) {
            case std::ios_base::beg: origin = seek_dirs::seek_beg; break;
            case std::ios_base::cur: origin = seek_dirs::seek_cur; break;
            case std::ios_base::end: origin = seek_dirs::seek_end; break;
            default: return pos_type(off_type(-1));
        }

        // If seeking relative to current in input mode, adjust by unread bytes
        if ((which & std::ios_base::in) && way == std::ios_base::cur) {
            off -= (egptr() - gptr());
        }

#if 0
        if (std::fseek(file, off, origin) != 0)
            return pos_type(off_type(-1));
#else
      stream_offset pos = file->seek(off, origin);
      if ( pos < 0)
            return pos_type(off_type(-1));
#endif

        // Invalidate buffer after seek
        setg(buffer.data() + put_back_size,
             buffer.data() + put_back_size,
             buffer.data() + put_back_size);
        setp(buffer.data(), buffer.data() + buffer.size());

        //long pos = std::ftell(file);
        if (pos < 0) return pos_type(off_type(-1));
        return pos_type(pos);
    }

    pos_type seekpos(pos_type sp, std::ios_base::openmode which) override {
        return seekoff(off_type(sp), std::ios_base::beg, which);
    }

private:
    static constexpr std::size_t put_back_size = 8;
    stream_interface* file;
    std::vector<char> buffer;

    int flush_buffer() {
        std::ptrdiff_t n = pptr() - pbase();
        if (n > 0) {
#if 0
            if (std::fwrite(pbase(), 1, n, file) != static_cast<size_t>(n))
                return EOF;
#else
          const buf_size  ret = file->write(reinterpret_cast<const byte*>(pbase()), static_cast<buf_size>(n));
          if ( ret != static_cast<buf_size>(n))
                return EOF;
#endif
            pbump(static_cast<int>(-n));
        }
        return 0;
    }
};

class buffered_streambuf final : public std::streambuf {
  stream_interface* stream_;
  std::vector<char> buffer_;

 public:
  explicit buffered_streambuf(stream_interface* s,
                              const std::size_t buffer_size = 4096)
      : stream_(s), buffer_(buffer_size) {
    if (!stream_) {
      throw std::invalid_argument("file pointer is null");
    }
    setg(buffer_.data(), buffer_.data() + buffer_.size(),
         buffer_.data() + buffer_.size());
    setp(buffer_.data(), buffer_.data() + buffer_.size());
  }

  ~buffered_streambuf() override { sync(); }

 protected:
  // Input
  int_type underflow() override {
    if (gptr() < egptr()) {
      return traits_type::to_int_type(*gptr());
    }
    const int n = stream_->read(reinterpret_cast<byte*>(buffer_.data()),
                                static_cast<buf_size>(buffer_.size()));
    if (n < 0) {
      assert(0);
    }
    if (n == 0) {
      return traits_type::eof();
    }
    setg(buffer_.data(), buffer_.data(), buffer_.data() + n);
    return traits_type::to_int_type(*gptr());
  }

  // Output
  int_type overflow(const int_type ch = traits_type::eof()) override {
    if (pptr() > pbase()) {
      const buf_size n = static_cast<buf_size>(pptr() - pbase());
      const int ret = stream_->write(reinterpret_cast<const byte*>(pbase()), n);
      if (ret < 0) {
        assert(0);
      }
      if (ret != static_cast<int>(n)) {
        return traits_type::eof();
      }
    }
    setp(buffer_.data(), buffer_.data() + buffer_.size());
    if (!traits_type::eq_int_type(ch, traits_type::eof())) {
      *pptr() = traits_type::to_char_type(ch);
      pbump(1);
    }
    return traits_type::not_eof(ch);
  }

  int sync() override {
    if (overflow() == traits_type::eof()) {
      return -1;
    }
    return stream_->flush() == 0 ? 0 : -1;
  }

  std::streampos seekoff(const std::streamoff off,
                         const std::ios_base::seekdir dir,
                         const std::ios_base::openmode which) override {
    // if (!stream_ || !stream_->seek) return -1;
    seek_dir cdir;
    if (dir == std::ios_base::beg) {
      cdir = seek_beg;
    } else if (dir == std::ios_base::cur) {
      cdir = seek_cur;
    } else if (dir == std::ios_base::end) {
      cdir = seek_end;
    } else {
      return {-1};
    }

    // Only flush if seeking in output mode
    if (which & std::ios_base::out) {
      if (stream_->flush() != 0) {
        return -1;
      }
    }
    const stream_offset pos = stream_->seek(off, cdir);
    return pos < 0 ? -1 : pos;
  }

  std::streampos seekpos(const std::streampos sp,
                         const std::ios_base::openmode which) override {
    return seekoff(sp, std::ios_base::beg, which);
  }
};

class nobuffer_streambuf : public std::streambuf {
  stream_interface* stream_;
  char last_char_;

 public:
  explicit nobuffer_streambuf(stream_interface* file) : stream_(file) {
    if (!stream_) {
      throw std::invalid_argument("file pointer is null");
    }
    setg(nullptr, nullptr, nullptr);
    setp(nullptr, nullptr);
  }

 protected:
  int_type underflow() override {
    char c;
    const int n = stream_->read(reinterpret_cast<byte*>(&c), 1);
    if (n < 0) {
      assert(0);
    }
    if (n == 0) {
      return traits_type::eof();
    }
    // Store the character in a static buffer for gptr/egptr contract
    last_char_ = c;
    setg(&last_char_, &last_char_, &last_char_ + 1);
    return traits_type::to_int_type(c);
  }

  int_type overflow(const int_type ch = traits_type::eof()) override {
    if (traits_type::eq_int_type(ch, traits_type::eof())) {
      return traits_type::not_eof(ch);
    }
    char c = traits_type::to_char_type(ch);
    const int ret = stream_->write(reinterpret_cast<const byte*>(&c), 1);
    if (ret < 0) {
      assert(0);
    }
    if (ret != 1) {
      return traits_type::eof();
    }
    return ch;
  }

  int sync() override { return stream_->flush() == 0 ? 0 : -1; }

  pos_type seekoff(const off_type off, const std::ios_base::seekdir dir,
                   const std::ios_base::openmode which) override {
    seek_dir cdir;
    if (dir == std::ios_base::beg) {
      cdir = seek_beg;
    } else if (dir == std::ios_base::cur) {
      cdir = seek_cur;
    } else if (dir == std::ios_base::end) {
      cdir = seek_end;
    } else {
      return {-1};
    }

    // Only flush if seeking in output mode
    if (which & std::ios_base::out) {
      if (stream_->flush() != 0) {
        return {-1};
      }
    }
    const stream_offset pos = stream_->seek(off, cdir);
    return pos < 0 ? -1 : pos;
  }

  pos_type seekpos(const pos_type pos,
                   const std::ios_base::openmode which) override {
    return seekoff(pos, std::ios_base::beg, which);
  }
};
}  // namespace cxx11

#endif  // CXX11BINDINGS_HXX
