#pragma once

#include <boost/property_tree/ptree.hpp>
#include <spdlog/spdlog.h>

namespace cvs::logger {

template <typename T>
struct ArgumentPreprocessor {
  template <typename Arg>
  static auto exec(const std::shared_ptr<spdlog::logger>&,
                   spdlog::level::level_enum,
                   const boost::property_tree::ptree&,
                   Arg&& arg) {
    return std::forward<Arg>(arg);
  }
};

}  // namespace cvs::logger
