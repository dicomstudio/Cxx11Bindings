#ifndef CXX11EXCEPTIONS_HXX
#define CXX11EXCEPTIONS_HXX

#include <cassert>
#include <exception>
#include <ios>
#include <stdexcept>

namespace cxx11 {
// Define an enum for C++ exception
enum class CxxExceptionCode : int {
  // O is Success
  RuntimeError = -1,
  LogicError = -2,
  InvalidArgument = -3,
  OutOfRange = -4,
  DomainError = -5,
  IoFailure = -6,
  NullPointer = -7,
  NotImplemented = -8,
  Unknown = -1000
};

// Custom exception class
class null_pointer final : public std::exception {
 public:
  const char* what() const noexcept override {
    return "Attempted to dereference a null pointer.";
  }

  null_pointer() = default;
};

class not_implemented_exception final : public std::logic_error {
 public:
  explicit not_implemented_exception(
      const std::string& message = "Function not yet implemented")
      : std::logic_error(message) {}
};

// --- exceptions ---

[[noreturn]] static inline void throw_exception_from_enum(
    const CxxExceptionCode code, const char* msg) {
  assert(msg);
  switch (code) {
    case CxxExceptionCode::RuntimeError:
      throw std::runtime_error("Runtime error occurred (code 1)");
    case CxxExceptionCode::LogicError:
      throw std::logic_error("Logic error occurred (code 2)");
    case CxxExceptionCode::InvalidArgument:
      throw std::invalid_argument("Invalid argument (code 3)");
    case CxxExceptionCode::OutOfRange:
      throw std::out_of_range("Out of range error (code 4)");
    case CxxExceptionCode::DomainError:
      throw std::domain_error("Domain error (code 5)");
    case CxxExceptionCode::IoFailure:
      throw std::ios_base::failure(msg);
    case CxxExceptionCode::NullPointer:
      throw null_pointer();
    case CxxExceptionCode::Unknown:
    default:
      throw std::exception();  // Generic fallback
  }
}

static inline void throw_exception_from_int(int written, const char* msg) {
  if (written < 0) {
    throw_exception_from_enum(static_cast<CxxExceptionCode>(written), msg);
  }
}

static inline void throw_exception_from_long(long written, const char* msg) {
  if (written < 0) {
    throw_exception_from_enum(static_cast<CxxExceptionCode>(written), msg);
  }
}

static inline void throw_exception_from_long_long(long long written,
                                                  const char* msg) {
  if (written < 0) {
    throw_exception_from_enum(static_cast<CxxExceptionCode>(written), msg);
  }
}

// Function that converts exception type to int code
static inline CxxExceptionCode exception_to_code(const std::exception& e) {
  if (dynamic_cast<const std::runtime_error*>(&e)) {
    return CxxExceptionCode::RuntimeError;
  }
  if (dynamic_cast<const std::logic_error*>(&e)) {
    return CxxExceptionCode::LogicError;
  }
  if (dynamic_cast<const std::invalid_argument*>(&e)) {
    return CxxExceptionCode::InvalidArgument;
  }
  if (dynamic_cast<const std::out_of_range*>(&e)) {
    return CxxExceptionCode::OutOfRange;
  }
  if (dynamic_cast<const std::domain_error*>(&e)) {
    return CxxExceptionCode::DomainError;
  }
  if (dynamic_cast<const std::ios_base::failure*>(&e)) {
    return CxxExceptionCode::IoFailure;
  }
  if (dynamic_cast<const null_pointer*>(&e)) {
    return CxxExceptionCode::NullPointer;
  }
  return CxxExceptionCode::Unknown;
}
}  // namespace cxx11

#endif  // CXX11EXCEPTIONS_HXX
