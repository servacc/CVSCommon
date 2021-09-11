#pragma once

#include <cvs/common/config.hpp>
#include <spdlog/spdlog.h>

namespace cvs::logger {

enum Sink {
  NO_SINK = 0,
  STDOUT  = 1,
  SYSTEMD = 2,
};

CVS_CONFIG(LoggerConfig, "Basic logger configuration.") {
  CVS_FIELD(name, std::string, "Logger name.");
  CVS_FIELD_DEF(level, spdlog::level::level_enum, spdlog::level::info, "Log level.");
  CVS_FIELD_OPT(pattern, std::string, "Message pattern.");
  CVS_FIELD_DEF(time_type, spdlog::pattern_time_type, spdlog::pattern_time_type::local,
                "Time in log message (utc or local)");
  CVS_FIELD_OPT(sink, Sink, "Enabled sinks");
};

using LoggerPtr = std::shared_ptr<spdlog::logger>;

namespace detail {

class LevelTranslator {
 public:
  using internal_type = std::string;
  using external_type = spdlog::level::level_enum;

  boost::optional<external_type> get_value(const internal_type& v) { return spdlog::level::from_str(v); }
  boost::optional<internal_type> put_value(const external_type& v) {
    return std::string(spdlog::level::to_string_view(v).data());
  }
};

class SinkTranslator {
 public:
  using internal_type = std::string;
  using external_type = Sink;

  inline static const std::string names[] = {"off", "std", "sysd"};

  boost::optional<external_type> get_value(const internal_type& v) {
    int sink = 0;
    for (const auto& sink_str : names) {
      if (sink_str == v)
        return static_cast<Sink>(sink);
      ++sink;
    }
    return Sink::NO_SINK;
  }
  boost::optional<internal_type> put_value(const external_type& v) { return names[v]; }
};

class TimeTypeTranslator {
 public:
  using internal_type = std::string;
  using external_type = spdlog::pattern_time_type;

  inline static const std::string names[] = {"utc", "local"};

  boost::optional<external_type> get_value(const internal_type& v) {
    if (v == names[0])
      return spdlog::pattern_time_type::utc;

    if (v == names[1])
      return spdlog::pattern_time_type::local;

    return {};
  }
  boost::optional<internal_type> put_value(const external_type& v) {
    switch (v) {
      case spdlog::pattern_time_type::utc: return names[0];
      case spdlog::pattern_time_type::local: return names[1];
    }
    return {};
  }
};

}  // namespace detail

}  // namespace cvs::logger

namespace boost::property_tree {

template <>
struct translator_between<std::string, spdlog::level::level_enum> {
  using type = cvs::logger::detail::LevelTranslator;
};

template <>
struct translator_between<std::string, cvs::logger::Sink> {
  using type = cvs::logger::detail::SinkTranslator;
};

template <>
struct translator_between<std::string, spdlog::pattern_time_type> {
  using type = cvs::logger::detail::TimeTypeTranslator;
};

}  // namespace boost::property_tree
