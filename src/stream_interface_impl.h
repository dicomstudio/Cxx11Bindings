// Mostly for internal testing, it provides different stream_interface
// implementation using standard API (FILE*)
#ifndef STREAM_INTERFACE_IMPL_H
#define STREAM_INTERFACE_IMPL_H

#include <cxx11bindings/stream.h>

#include <errno.h>
#include <stdio.h>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>  // _chsize_s
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#else
#include <unistd.h>  // ftruncate
#endif

#ifdef __cplusplus
extern "C" {
#endif

static inline buf_size fread_file_fp(FILE* stream, byte* buffer,
                                     const buf_size count) {
  if (!stream) {
    return C11_E_POINTER;
  }
  if (!buffer) {
    return C11_E_POINTER;
  }
  if (count < 0) {
    return C11_E_INVALIDARG;
  }
  const size_t read = fread(buffer, 1, (size_t)count, stream);
  if (read < (size_t)count) {
    const int err = ferror(stream);
    if (err) {
      // An error occurred
      if (err == EPERM) {
        return C11_E_NOTSUPPORTED;
      }
      // else
      return C11_E_IO;
    }
  }
  // short-read ok:
  return (buf_size)read;
}

static inline buf_size fwrite_file_fp(FILE* stream, const byte* buffer,
                                      const buf_size count) {
  if (!stream) {
    return C11_E_POINTER;
  }
  if (!buffer) {
    return C11_E_POINTER;
  }
  if (count < 0) {
    return C11_E_INVALIDARG;
  }
  const size_t written = fwrite(buffer, 1, (size_t)count, stream);
  if (written != (size_t)count) {
    return C11_E_IO;
  }
  return (buf_size)written;
}

static inline int fseek_file_fp(FILE* stream, const int64_t offset,
                                const seek_dir dir) {
  if (!stream) {
    return C11_E_POINTER;
  }
  int whence;
  switch (dir) {
    case seek_beg:
      whence = SEEK_SET;
      break;
    case seek_cur:
      whence = SEEK_CUR;
      break;
    case seek_end:
      whence = SEEK_END;
      break;
    default:
      return C11_E_INVALIDARG;
  }
#ifdef _WIN32
  const int ret = _fseeki64(stream, offset, whence);
#else
  const int ret = fseeko(stream, offset, whence);
#endif
  if (ret < 0) {
    if (errno == EINVAL) {
      // Invalid offset or whence
      return C11_E_INVALIDARG;
    }
    // else
    return C11_E_IO;
  }
  return ret;
}

static inline int64_t ftell_file_fp(FILE* stream) {
#ifdef _WIN32
  const int64_t ret = _ftelli64(stream);
#else
  const int64_t ret = ftello(stream);
#endif
  if (ret < 0) {
    return C11_E_IO;
  }
  return ret;
}

static inline int fflush_file_fp(FILE* stream) {
  if (!stream) {
    return C11_E_POINTER;
  }
  const int ret = fflush(stream);
  // Otherwise, EOF is returned and errno is set to indicate the error.
  // EBADF  stream is not an open stream, or is not open for writing.
  if (ret < 0) {
    return C11_E_IO;
  }
  return ret;
}

// Truncate an open FILE* stream to a given size (in bytes).
// Returns new size on success, -1 on error.
static inline int64_t ftruncate_file_fp(FILE* fp, int64_t new_size) {
  if (!fp) {
    return C11_E_POINTER;
  }
  if (new_size < 0) {
    return C11_E_INVALIDARG;
  }

#ifdef _WIN32
  const int fd = _fileno(fp);
  if (fd < 0) {
    return C11_E_INVALIDARG;
  }

  if (_chsize_s(fd, new_size) != 0) {
    return C11_E_NOTSUPPORTED;
  }
#else
  const int fd = fileno(fp);
  if (fd < 0) {
    return C11_E_INVALIDARG;
  }

  if (ftruncate(fd, new_size) != 0) {
    return C11_E_NOTSUPPORTED;
  }
#endif

  return new_size;
}
#ifdef _WIN32
static inline buf_size read_handle(const HANDLE handle, byte* buffer,
                                   const buf_size count) {
  if (handle == NULL) {
    return C11_E_POINTER;
  }
  if (!buffer) {
    return C11_E_POINTER;
  }
  if (count < 0) {
    return C11_E_INVALIDARG;
  }
  DWORD bytes_read;
  const BOOL success = ReadFile(handle, buffer, count, &bytes_read, NULL);
  if (!success) {
    return C11_E_IO;
  }
  // short-read ok:
  return (buf_size)bytes_read;
}
#else
static inline buf_size read_fd(int fileno, byte* buffer, const buf_size count) {
  if (fileno < 0) {
    return C11_E_POINTER;
  }
  if (!buffer) {
    return C11_E_POINTER;
  }
  if (count < 0) {
    return C11_E_INVALIDARG;
  }
  const ssize_t r = read(fileno, buffer, (size_t)count);
  if (r == -1) {
    const int err = errno;
    if (err) {
      // An error occurred
      if (err == EPERM) {
        return C11_E_NOTSUPPORTED;
      }
      // else
      return C11_E_IO;
    }
  }
  // short-read ok:
  return (buf_size)r;
}
#endif

#ifdef _WIN32
static inline buf_size write_handle(const HANDLE handle, const byte* buffer,
                                    const buf_size count) {
  if (handle == NULL) {
    return C11_E_POINTER;
  }
  if (!buffer) {
    return C11_E_POINTER;
  }
  if (count < 0) {
    return C11_E_INVALIDARG;
  }
  DWORD bytes_written;
  const BOOL success = WriteFile(handle, buffer, count, &bytes_written, NULL);
  if (!success) {
    return C11_E_IO;
  }
  return (buf_size)bytes_written;
}
#else
static inline buf_size write_fd(int fileno, const byte* buffer,
                                const buf_size count) {
  if (fileno < 0) {
    return C11_E_POINTER;
  }
  if (!buffer) {
    return C11_E_POINTER;
  }
  if (count < 0) {
    return C11_E_INVALIDARG;
  }
  const ssize_t written = write(fileno, buffer, (size_t)count);
  if (written != (ssize_t)count) {
    return C11_E_IO;
  }
  return (buf_size)written;
}
#endif

#ifdef _WIN32
static inline int64_t seek_handle(const HANDLE handle, const int64_t offset,
                                  const seek_dir dir) {
  if (handle == NULL) {
    return C11_E_POINTER;
  }
  int move_method;
  switch (dir) {
    case seek_beg:
      move_method = FILE_BEGIN;
      break;
    case seek_cur:
      move_method = FILE_CURRENT;
      break;
    case seek_end:
      move_method = FILE_END;
      break;
    default:
      return C11_E_INVALIDARG;
  }

  LARGE_INTEGER distance;
  distance.QuadPart = offset;
  LARGE_INTEGER pos;
  const BOOL result = SetFilePointerEx(handle, distance, &pos, move_method);
  if (!result) {
    // handle error
    return C11_E_IO;
  }

  return pos.QuadPart;
}
#else

static inline int64_t lseek_fd(int fileno, const int64_t offset,
                               const seek_dir dir) {
  if (fileno < 0) {
    return C11_E_POINTER;
  }
  int whence;
  switch (dir) {
    case seek_beg:
      whence = SEEK_SET;
      break;
    case seek_cur:
      whence = SEEK_CUR;
      break;
    case seek_end:
      whence = SEEK_END;
      break;
    default:
      return C11_E_INVALIDARG;
  }
  const off64_t ret = lseek64(fileno, offset, whence);
  if (ret < 0) {
    if (errno == EINVAL) {
      // Invalid offset or whence
      return C11_E_INVALIDARG;
    }
    // else
    return C11_E_IO;
  }
  return ret;
}
#endif

#ifdef _WIN32
static inline int flush_handle(const HANDLE handle) {
  if (handle == NULL) {
    return C11_E_POINTER;
  }

  const BOOL success = FlushFileBuffers(handle);
  if (!success) {
    // handle error
    return C11_E_IO;
  }
  return 0;
}
#else
static inline int fsync_fd(int fd) {
  if (fd < 0) {
    return C11_E_POINTER;
  }
  const int ret = fsync(fd);
  if (ret < 0) {
    return C11_E_IO;
  }
  return ret;
}
#endif

#ifdef _WIN32

static inline BOOL truncate_file_handle(const HANDLE handle,
                                        const int64_t new_size) {
  LARGE_INTEGER li;
  li.QuadPart = new_size;
  if (!SetFilePointerEx(handle, li, NULL, FILE_BEGIN)) {
    return FALSE;
  }
  return SetEndOfFile(handle);
}

static inline int64_t truncate_handle(const HANDLE handle,
                                      const int64_t new_size) {
  if (handle == NULL) {
    return C11_E_POINTER;
  }
  if (new_size < 0) {
    return C11_E_INVALIDARG;
  }
  if (!truncate_file_handle(handle, new_size)) {
    return C11_E_NOTSUPPORTED;
  }
  return new_size;
}
#else
static inline int64_t ftruncate_fd(int fd, int64_t new_size) {
  if (fd < 0) {
    return C11_E_POINTER;
  }
  if (new_size < 0) {
    return C11_E_INVALIDARG;
  }

  if (ftruncate(fd, new_size) != 0) {
    return C11_E_NOTSUPPORTED;
  }

  return new_size;
}
#endif

#ifdef __cplusplus
}  // end extern "C"
#endif

#endif  // STREAM_INTERFACE_IMPL_H
