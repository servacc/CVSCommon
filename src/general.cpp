#include "../include/cvs/common/general.hpp"

namespace cvs::common {

std::string exceptionStr(const std::exception& e, int level) {
  std::string result = std::string(level, ' ') + e.what();
  try {
    std::rethrow_if_nested(e);
  }
  catch (const std::exception& e) {
    result += '\n' + exceptionStr(e, level + 1);
  }
  catch (...) {
    result += '\n' + std::string(level + 1, ' ') + "Unknown exception";
  }

  return result;
}

}  // namespace cvs::common
