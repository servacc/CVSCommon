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
void createDefaultLogger(const common::Properties&);

void initLoggers();
void initLoggers(const common::Properties&);

const common::Properties& loggerConfiguration(const std::string&);
const common::Properties& loggerConfiguration(const LoggerPtr::element_type&);

void configureLoggersList(const common::Properties&);
void configureLogger(const common::Properties&);

common::CVSOutcome<LoggerPtr> createLogger(const std::string&);
common::CVSOutcome<LoggerPtr> createLogger(const common::Properties&);

namespace detail {

template <typename... Args>
void logHelper(const LoggerPtr& logger, spdlog::level::level_enum lvl, spdlog::string_view_t fmt, Args&&... args) {
  logger->log(lvl, fmt,
              (ArgumentPreprocessor<std::remove_cvref_t<Args>>::exec(logger, lvl, loggerConfiguration(*logger),
                                                                     std::forward<Args>(args)))...);
}

}  // namespace detail

}  // namespace cvs::logger

#define CVSCOMMON_LOG_HELPER_MACRO_INNER(LL, CH, ...) \
  if (CH && CH->should_log(LL))                       \
    cvs::logger::detail::logHelper(CH, LL, __VA_ARGS__);

#define LOG_TRACE(CH, ...) CVSCOMMON_LOG_HELPER_MACRO_INNER(spdlog::level::trace, CH, __VA_ARGS__)
#define LOG_TRACE_L(...)   LOG_TRACE(logger(), __VA_ARGS__)

#define LOG_DEBUG(CH, ...) CVSCOMMON_LOG_HELPER_MACRO_INNER(spdlog::level::debug, CH, __VA_ARGS__)
#define LOG_DEBUG_L(...)   LOG_DEBUG(logger(), __VA_ARGS__)

#define LOG_INFO(CH, ...) CVSCOMMON_LOG_HELPER_MACRO_INNER(spdlog::level::info, CH, __VA_ARGS__)
#define LOG_INFO_L(...)   LOG_INFO(logger(), __VA_ARGS__)

#define LOG_WARN(CH, ...) CVSCOMMON_LOG_HELPER_MACRO_INNER(spdlog::level::warn, CH, __VA_ARGS__)
#define LOG_WARN_L(...)   LOG_WARN(logger(), __VA_ARGS__)

#define LOG_ERROR(CH, ...) CVSCOMMON_LOG_HELPER_MACRO_INNER(spdlog::level::err, CH, __VA_ARGS__)
#define LOG_ERROR_L(...)   LOG_ERROR(logger(), __VA_ARGS__)

#define LOG_CRITICAL(CH, ...) CVSCOMMON_LOG_HELPER_MACRO_INNER(spdlog::level::critical, CH, __VA_ARGS__)
#define LOG_CRITICAL_L(...)   LOG_CRITICAL(logger(), __VA_ARGS__)

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
