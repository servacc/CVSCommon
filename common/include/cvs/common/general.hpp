#pragma once

#include <boost/outcome.hpp>
#include <fmt/format.h>

namespace cvs::common {

namespace outcome = BOOST_OUTCOME_V2_NAMESPACE;
template <typename T>
struct CVSOutcome : public outcome::outcome<T, std::error_code, std::exception_ptr> {
  template <typename V>
  CVSOutcome(V&& value)
      : outcome::outcome<T, std::error_code, std::exception_ptr>(std::forward<V>(value)) {}

  T&       operator*() { return outcome::outcome<T, std::error_code, std::exception_ptr>::value(); }
  const T& operator*() const { return outcome::outcome<T, std::error_code, std::exception_ptr>::value(); }

  T*       operator->() { return &outcome::outcome<T, std::error_code, std::exception_ptr>::value(); }
  const T* operator->() const { return &outcome::outcome<T, std::error_code, std::exception_ptr>::value(); }
};

std::string exceptionStr(const std::exception& e, int level = 0);

template <typename T, typename... Args>
void throwWithNested(Args&&... args) {
  static_assert(std::is_base_of_v<std::exception, T>, "T should be std::exception.");
  std::throw_with_nested(T(fmt::format(std::forward<Args>(args)...)));
}

template <typename T, typename... Args>
void throwException(Args&&... args) {
  static_assert(std::is_base_of_v<std::exception, T>, "T should be std::exception.");
  throw T(fmt::format(std::forward<Args>(args)...));
}

}  // namespace cvs::common

#define CVS_RETURN_WITH_NESTED(nested) \
  try {                                \
    std::throw_with_nested(nested);    \
  }                                    \
  catch (...) {                        \
    return std::current_exception();   \
  }
