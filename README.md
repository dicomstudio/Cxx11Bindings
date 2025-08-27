# Cxx11Bindings

This is a lightweight C11 library to help bind C++11 code to foreign languages like C#.

P/Invoke provides a set of marshaling capabilities for C-style APIs, but it does not support C++ Stream/APIs directly.

## Implementation details

The stream abstraction is simply defined with a set of five function pointers:

```
struct c11_stream {
  read_fn read;
  write_fn write;
  seek_fn seek;
  flush_fn flush;
  trunc_fn trunc;
};
```

Where function declarations are:

```
typedef int32_t (*read_fn)(byte* buffer, int32_t count);
typedef int32_t (*write_fn)(const byte* buffer, int32_t count);
typedef int64_t (*seek_fn)(int64_t off, int dir);
typedef int (*flush_fn)(void);
typedef int64_t (*trunc_fn)(int64_t size);
```

## See:

* https://learn.microsoft.com/en-us/cpp/build/walkthrough-using-msbuild-to-create-a-visual-cpp-project?view=msvc-170
