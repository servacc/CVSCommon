#pragma once

#include <boost/core/demangle.hpp>
#include <cvs/logger/logging.hpp>

#include <string_view>

namespace cvs::logger {

template <typename T>
class Loggable {
 public:
  explicit Loggable(const boost::property_tree::ptree& cfg) { log = *createLogger(cfg); }
  explicit Loggable(const std::string& name = boost::core::demangle(typeid(T).name())) { log = *createLogger(name); }
  virtual ~Loggable() = default;

  void configureLogger(const boost::property_tree::ptree& cfg) { configureLogger(logger(), cfg); }

  [[nodiscard]] LoggerPtr&       logger() { return log; }
  [[nodiscard]] const LoggerPtr& logger() const { return log; }

 private:
  LoggerPtr log;
};

}  // namespace cvs::logger
