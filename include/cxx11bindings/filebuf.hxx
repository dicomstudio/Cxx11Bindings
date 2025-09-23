#ifndef CXX11_BINDINGS_FILEBUF_HXX
#define CXX11_BINDINGS_FILEBUF_HXX

#include <cxx11bindings/stream.h>

#include <cwchar>  // wchar_t
#include <iosfwd>  // std::streambuf

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Create and initialize a cxx11_file_stream from a file specified by filename
 * and mode. filename is expressed in UTF-8 encoding.
 * @param p_self
 * @param filename
 * @param mode
 * @return
 */
CXX11_BINDINGS_EXPORT int cxx11_file_stream_create1(c11_stream** p_self,
                                                    const char* filename,
                                                    const char* mode);

/**
 * Create and initialize a cxx11_file_stream from a file specified by filename
 * and mode. filename is expressed in UTF-16 encoding.
 * @param p_self
 * @param wfilename
 * @param wmode
 * @return
 */
CXX11_BINDINGS_EXPORT int cxx11_file_stream_create2(c11_stream** p_self,
                                                    const wchar_t* wfilename,
                                                    const wchar_t* wmode);
/**
 * Destroy a cxx11_file_stream, if created by cxx11_file_stream_create1 or
 * cxx11_file_stream_create2 underlying stream will be closed.
 * @param self
 * @return
 */
CXX11_BINDINGS_EXPORT int cxx11_file_stream_destroy(c11_stream* self);

/**
 * Initialize a cxx11_file_stream from an existing std::streambuf& stream.
 * The std::streambuf& stream is not deleted when the c11_stream is destroyed.
 * @param self
 * @param streambuf
 * @return
 */
CXX11_BINDINGS_EXPORT int cxx11_file_stream_init(c11_stream** p_self,
                                                 std::streambuf& sb);

#ifdef __cplusplus
}  // end extern "C"
#endif

#endif  // CXX11_BINDINGS_FILEBUF_HXX
