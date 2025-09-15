# Cxx11Bindings

This is a lightweight C11 header-only library to help bind C++11 code to foreign languages like C#.

## Implementation details

The stream abstraction is simply defined with a set of five function pointers:

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

## See:

* https://learn.microsoft.com/en-us/cpp/build/walkthrough-using-msbuild-to-create-a-visual-cpp-project?view=msvc-170
