# Cxx11Bindings

Cxx11Bindings defines a lightweight, header-only stream interface for connecting
C++11 code to other languages, such as C# and Python.

## Interface and implementations

The interface is the C ABI defined by `c11_stream` in
`cxx11bindings/stream.h`. A foreign-language application can implement this
interface directly using its own stream and native-compatible callbacks, then
pass a pointer to the resulting `c11_stream` to C++ code. The foreign-language
implementation must keep the stream and callbacks alive while they are in use.

C++ application code can include `cxx11bindings/streambuf.hxx` directly. Its
header-defined adapters, including `cxx11::c_stream` and
`cxx11::basic_streambuf`, can be compiled into the application without linking
against the Cxx11Bindings shared library. `cxx11::c_stream` wraps the
ABI-facing `c11_stream`; it is a C++ adapter, not the C ABI itself.

The optional `basic_streambuf` buffer size must be positive and small enough
for its putback space and callback counts. Zero throws `std::invalid_argument`;
sizes exceeding the supported range throw `std::length_error` before allocation.
`basic_streambuf` throws `stream_exception` for a negative read result and
uses `InvalidOperation` when a read reports more bytes than requested.
`pubsync()` writes buffered output and then flushes the underlying stream;
check its return value or handle exceptions to detect failures. Its destructor
attempts the same operation but cannot report failures and suppresses exceptions.

The optional compiled shared library supplies `FILE*` and `std::filebuf`
implementations of the interface. These are useful examples or ready-made
adapters, not a required dependency for code that implements the interface
directly. A foreign-language implementation typically provides its own
callbacks rather than using this shared library; it must still load or link
whatever native application library consumes the interface.

## Implementation details

The stream interface is synchronous: each callback completes and returns its result
before the call returns, and may block. Read and write callbacks must not access
the supplied buffer after returning. The interface does not provide asynchronous
completion or guarantee thread safety.

The stream abstraction is defined with a set of five function pointers:

```c
struct c11_stream {
  read_fn read;
  write_fn write;
  seek_fn seek;
  flush_fn flush;
  trunc_fn trunc;
};
```

Where function declarations are:

```c
typedef int32_t (*read_fn)(struct c11_stream* self, byte* buffer, int32_t count);
typedef int32_t (*write_fn)(struct c11_stream* self, const byte* buffer, int32_t count);
typedef int64_t (*seek_fn)(struct c11_stream* self, int64_t off, int dir);
typedef int (*flush_fn)(struct c11_stream* self);
typedef int64_t (*trunc_fn)(struct c11_stream* self, int64_t size);
```

The C++ `cxx11::c_stream` adapter throws `cxx11::stream_exception` for any
negative callback result. `code()` identifies known `ErrorCode` values and
returns `ErrorCode::Unmapped` for unknown values; `raw_code()` retains the
original signed 64-bit result, and `what()` includes it. Callers migrating from
the former individual exception types should catch `stream_exception` and
inspect its code instead.

## See:

* https://learn.microsoft.com/en-us/cpp/build/walkthrough-using-msbuild-to-create-a-visual-cpp-project?view=msvc-170
