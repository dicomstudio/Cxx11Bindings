#ifndef CXX11_BINDINGS_FILE_H
#define CXX11_BINDINGS_FILE_H

#include <cxx11bindings/stream.h>

#include <stdio.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Create and initialize a c11_file_stream from a file specified by filename and
 * mode. filename is expressed in UTF-8 encoding.
 * @param p_self
 * @param filename
 * @param mode
 * @return
 */
CXX11_BINDINGS_EXPORT int c11_file_stream_create1(struct c11_stream** p_self,
                                                  const char* filename,
                                                  const char* mode);

/**
 * Create and initialize a c11_file_stream from a file specified by filename and
 * mode. filename is expressed in UTF-16 encoding.
 * @param p_self
 * @param filename
 * @param mode
 * @return
 */
CXX11_BINDINGS_EXPORT int c11_file_stream_create2(struct c11_stream** p_self,
                                                  const wchar_t* filename,
                                                  const wchar_t* mode);

/**
 * Destroy a c11_file_stream, if created by c11_file_stream_create1 or
 * c11_file_stream_create2 underlying stream will be closed.
 * @param self
 * @return
 */
CXX11_BINDINGS_EXPORT int c11_file_stream_destroy(struct c11_stream* self);

/**
 * Initialize a c11_file_stream from an existing FILE* stream.
 * The FILE* stream is not closed when the c11_stream is destroyed.
 * @param p_self
 * @param stream
 * @return
 */
CXX11_BINDINGS_EXPORT int c11_file_stream_init(struct c11_stream** p_self,
                                               FILE* stream);

#ifdef _WIN32
/**
 * Initialize a c11_file_stream from an existing file handle.
 * The file handle is not closed when the c11_stream is destroyed.
 * @param p_self
 * @param handle
 * @return
 */
CXX11_BINDINGS_EXPORT int c11_handle_stream_init(struct c11_stream** p_self,
                                                 HANDLE handle);
#else
/**
 * Initialize a c11_file_stream from an existing file descriptor.
 * The file descriptor is not closed when the c11_stream is destroyed.
 * @param p_self
 * @param fd
 * @return
 */
CXX11_BINDINGS_EXPORT int c11_fd_stream_init(struct c11_stream** p_self,
                                             int fd);
#endif

#ifdef __cplusplus
}  // end extern "C"
#endif

#endif  // CXX11_BINDINGS_FILE_H
