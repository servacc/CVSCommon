#include "../include/cvs/logger/logging.hpp"

#include <cvs/common/staticFactory.hpp>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/systemd_sink.h>
#include <spdlog/spdlog.h>

using namespace cvs::logger;
using namespace std::string_literals;

namespace {

using StdoutSink  = spdlog::sinks::stdout_color_sink_mt;
using SystemdSink = spdlog::sinks::systemd_sink_st;

const std::string name_in_factory = "cvs.logger";

}  // namespace

namespace cvs::logger {

static boost::property_tree::ptree& configurations(const std::string& name) {
  static std::map<std::string, boost::property_tree::ptree> settings;

  boost::property_tree::ptree& val = settings[name];
  if (val.empty())
    val.put("name", name);
  return val;
}

const boost::property_tree::ptree& loggerConfiguration(const std::string& name) { return configurations(name); }
const boost::property_tree::ptree& loggerConfiguration(const LoggerPtr::element_type& logger) {
  return loggerConfiguration(logger.name());
}

void configureLoggersList(const boost::property_tree::ptree& config) {
  try {
    auto loggers_list = config.get_child_optional("loggers");
    if (loggers_list) {
      for (auto l : *loggers_list) {
        auto cfg = LoggerConfig::make(l.second);
        configureLogger(l.second);
      }
    }
  }
  catch (...) {
    std::throw_with_nested(std::runtime_error("Can't configure loggers array."));
  }
}

void configureLogger(LoggerPtr::element_type& logger, const LoggerConfig& config) {
  try {
    if (config.pattern)
      logger.set_pattern(*config.pattern, config.time_type);

    logger.set_level(config.level);

    if (config.sink) {
      auto sinks_flags = Sink(*config.sink);
      auto sinks       = logger.sinks();
      for (auto& s : sinks) {
        auto std_sink = std::dynamic_pointer_cast<StdoutSink>(s);
        if (std_sink) {
          std_sink->set_level(sinks_flags & Sink::STDOUT ? config.level : spdlog::level::off);
          continue;
        }
        auto sys_sink = std::dynamic_pointer_cast<SystemdSink>(s);
        if (sys_sink) {
          sys_sink->set_level(sinks_flags & Sink::SYSTEMD ? config.level : spdlog::level::off);
          continue;
        }
      }
    }
  }
  catch (...) {
    std::throw_with_nested(
        std::runtime_error(fmt::format(R"(Can't configure logger object for the "{}" channal.)", logger.name())));
  }
}

void configureLogger(const boost::property_tree::ptree& config) {
  try {
    common::StaticFactory::create<LoggerPtr, std::string, const boost::property_tree::ptree&>(name_in_factory, config)
        .value();
  }
  catch (...) {
    std::throw_with_nested(std::runtime_error(R"(Can't create and configure logger.)"));
  }
}

common::CVSOutcome<LoggerPtr> createLogger(const std::string& name) {
  return common::StaticFactory::create<LoggerPtr, std::string, const std::string&>(name_in_factory, name);
}

common::CVSOutcome<LoggerPtr> createLogger(const boost::property_tree::ptree& config) {
  return common::StaticFactory::create<LoggerPtr, std::string, const boost::property_tree::ptree&>(name_in_factory,
                                                                                                   config);
}

void createDefaultLogger() {
  auto default_logger = createLogger("");
  spdlog::set_default_logger(*default_logger);
}

void createDefaultLogger(const boost::property_tree::ptree& config) {
  auto default_logger = createLogger("");
  spdlog::set_default_logger(*default_logger);

  configureLogger(**default_logger, *LoggerConfig::make(config));
}

void registerLoggersInFactory() {
  common::StaticFactory::registerType<LoggerPtr(const std::string&)>(name_in_factory, [](const std::string& name) {
    auto logger = spdlog::get(name);
    if (!logger) {
      logger = std::make_shared<spdlog::logger>(
          name, spdlog::sinks_init_list{std::make_shared<StdoutSink>(), std::make_shared<SystemdSink>()});
      spdlog::register_logger(logger);
    }
    return logger;
  });

  common::StaticFactory::registerType<LoggerPtr(const boost::property_tree::ptree&)>(
      name_in_factory, [](const boost::property_tree::ptree& cfg) -> LoggerPtr {
        auto cfg_struct_opt = LoggerConfig::make(cfg);

        auto logger_opt = common::StaticFactory::create<LoggerPtr, std::string, const std::string&>(
            name_in_factory, cfg_struct_opt->name);

        configureLogger(**logger_opt, *cfg_struct_opt);

        configurations(cfg_struct_opt->name) = cfg;

        return *logger_opt;
      });
}

void initLoggers() {
  try {
    registerLoggersInFactory();
    createDefaultLogger();
  }
  catch (...) {
    std::throw_with_nested(std::runtime_error("Can't init loggers."));
  }
}

void initLoggers(const boost::property_tree::ptree& config) {
  initLoggers();

  configureLoggersList(config);
}

}  // namespace cvs::logger
