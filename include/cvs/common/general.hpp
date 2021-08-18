#pragma once

#include <boost/outcome.hpp>

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
}  // namespace cvs::common

#define CVS_RETURN_WITH_NESTED(nested) \
  try {                                \
    std::throw_with_nested(nested);    \
  }                                    \
  catch (...) {                        \
    return std::current_exception();   \
  }
