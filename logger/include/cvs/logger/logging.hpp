#pragma once

#include <cvs/common/config.hpp>
#include <cvs/logger/argumentPreprocessor.hpp>
#include <cvs/logger/loggerTypes.hpp>
#include <spdlog/spdlog.h>

#ifdef CVS_LOGGER_OPENCV_ENABLED
#include <cvs/logger/opencvHelper.hpp>
#endif

namespace cvs::logger {

void registerLoggersInFactory();

void createDefaultLogger();
void createDefaultLogger(const boost::property_tree::ptree&);

void initLoggers();
void initLoggers(const boost::property_tree::ptree&);

const boost::property_tree::ptree& loggerConfiguration(const std::string&);
const boost::property_tree::ptree& loggerConfiguration(const LoggerPtr::element_type&);

void configureLoggersList(const boost::property_tree::ptree&);
void configureLogger(const boost::property_tree::ptree&);

common::CVSOutcome<LoggerPtr> createLogger(const std::string&);
common::CVSOutcome<LoggerPtr> createLogger(const boost::property_tree::ptree&);

namespace detail {

template <typename... Args>
void logHelper(const LoggerPtr& logger, spdlog::level::level_enum lvl, spdlog::string_view_t fmt, Args&&... args) {
  logger->log(lvl, fmt,
              (ArgumentPreprocessor<std::remove_cvref_t<Args>>::exec(logger, lvl, loggerConfiguration(*logger),
                                                                     std::forward<Args>(args)))...);
}

}  // namespace detail

}  // namespace cvs::logger

#define LOG_TRACE(CH, ...)                        \
  if (CH && CH->should_log(spdlog::level::trace)) \
  cvs::logger::detail::logHelper(CH, spdlog::level::trace, __VA_ARGS__)
#define LOG_DEBUG(CH, ...)                        \
  if (CH && CH->should_log(spdlog::level::debug)) \
  cvs::logger::detail::logHelper(CH, spdlog::level::debug, __VA_ARGS__)
#define LOG_INFO(CH, ...)                        \
  if (CH && CH->should_log(spdlog::level::info)) \
  cvs::logger::detail::logHelper(CH, spdlog::level::info, __VA_ARGS__)
#define LOG_WARN(CH, ...)                        \
  if (CH && CH->should_log(spdlog::level::warn)) \
  cvs::logger::detail::logHelper(CH, spdlog::level::warn, __VA_ARGS__)
#define LOG_ERROR(CH, ...)                      \
  if (CH && CH->should_log(spdlog::level::err)) \
  cvs::logger::detail::logHelper(CH, spdlog::level::err, __VA_ARGS__)
#define LOG_CRITICAL(CH, ...)                        \
  if (CH && CH->should_log(spdlog::level::critical)) \
  cvs::logger::detail::logHelper(CH, spdlog::level::critical, __VA_ARGS__)

#define LOG_GLOB_TRACE(...)                                                        \
  if (auto l = spdlog::default_logger(); l && l->should_log(spdlog::level::trace)) \
  cvs::logger::detail::logHelper(l, spdlog::level::trace, __VA_ARGS__)
#define LOG_GLOB_DEBUG(...)                                                        \
  if (auto l = spdlog::default_logger(); l && l->should_log(spdlog::level::debug)) \
  cvs::logger::detail::logHelper(l, spdlog::level::debug, __VA_ARGS__)
#define LOG_GLOB_INFO(...)                                                        \
  if (auto l = spdlog::default_logger(); l && l->should_log(spdlog::level::info)) \
  cvs::logger::detail::logHelper(l, spdlog::level::info, __VA_ARGS__)
#define LOG_GLOB_WARN(...)                                                        \
  if (auto l = spdlog::default_logger(); l && l->should_log(spdlog::level::warn)) \
  cvs::logger::detail::logHelper(l, spdlog::level::warn, __VA_ARGS__)
#define LOG_GLOB_ERROR(...)                                                      \
  if (auto l = spdlog::default_logger(); l && l->should_log(spdlog::level::err)) \
  cvs::logger::detail::logHelper(l, spdlog::level::err, __VA_ARGS__)
#define LOG_GLOB_CRITICAL(...)                                                        \
  if (auto l = spdlog::default_logger(); l && l->should_log(spdlog::level::critical)) \
  cvs::logger::detail::logHelper(l, spdlog::level::critical, __VA_ARGS__)
