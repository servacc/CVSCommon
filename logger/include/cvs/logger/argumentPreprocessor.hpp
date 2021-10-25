#pragma once

#include <cvs/common/config.hpp>
#include <spdlog/spdlog.h>

namespace cvs::logger {

template <typename T>
struct ArgumentPreprocessor {
  template <typename Arg>
  static auto exec(const std::shared_ptr<spdlog::logger>&,
                   spdlog::level::level_enum,
                   const common::Properties&,
                   Arg&& arg) {
    return std::forward<Arg>(arg);
  }
};

}  // namespace cvs::logger
