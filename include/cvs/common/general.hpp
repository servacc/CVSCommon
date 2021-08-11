#pragma once

#include <boost/outcome.hpp>

namespace cvs::common {

namespace outcome = BOOST_OUTCOME_V2_NAMESPACE;
template <typename T>
using CVSOutcome = outcome::outcome<T, std::error_code, std::exception_ptr>;

std::string exceptionStr(const std::exception& e, int level = 0);
}  // namespace cvs::common

#define CVS_RETURN_WITH_NESTED(nested) \
  try {                                \
    std::throw_with_nested(nested);    \
  }                                    \
  catch (...) {                        \
    return std::current_exception();   \
  }
