#include <boost/property_tree/json_parser.hpp>
#include <cvs/common/factory.hpp>
#include <cvs/logger/logging.hpp>
#include <gtest/gtest.h>

#include <list>
#include <regex>
#include <tuple>

using namespace cvs::logger;
using namespace std::string_literals;

namespace {

class DefaultLoggerTest : public ::testing::TestWithParam<std::string> {
 public:
};

TEST_P(DefaultLoggerTest, configure) {
  std::string config_json = GetParam();

  std::stringstream ss;
  ss << config_json;
  boost::property_tree::ptree root;

  ASSERT_NO_THROW(boost::property_tree::read_json(ss, root));

  cvs::logger::configureLogger(root);

  LOG_GLOB_TRACE("Test {} {}", 0, 1);
  LOG_GLOB_DEBUG("Test {} {}", 0, 1);
  LOG_GLOB_INFO("Test {} {}", 0, 1);
  LOG_GLOB_WARN("Test {} {}", 0, 1);
  LOG_GLOB_ERROR("Test {} {}", 0, 1);
}

INSTANTIATE_TEST_SUITE_P(LoggerTest,
                         DefaultLoggerTest,
                         ::testing::Values(R"({ "name": "", "level": "trace", "sink": "std" })",
                                           R"({ "name": "", "level": "info", "sink": "std" })"));

TEST(LoggerTest, config_array) {
  std::string config_json = R"(
{
  "item1" : { "name": "1" },
  "loggers": [
    { "name": "", "level": "trace", "sink": "std" },
    { "name": "test0", "level": "trace", "sink": "std" },
    { "name": "test1", "level": "debug", "sink": "std" },
    { "name": "test2", "level": "info", "sink": "std" }
  ],
  "item1" : { "name": "2" }
})";

  std::stringstream ss;
  ss << config_json;
  boost::property_tree::ptree root;

  ASSERT_NO_THROW(boost::property_tree::read_json(ss, root));

  configureLoggersList(root);

  LOG_GLOB_TRACE("DEF {} {}", 0, 1);
  LOG_GLOB_DEBUG("DEF {} {}", 0, 1);
  LOG_GLOB_INFO("DEF {} {}", 0, 1);
  LOG_GLOB_WARN("DEF {} {}", 0, 1);
  LOG_GLOB_ERROR("DEF {} {}", 0, 1);

  auto logger0 = *createLogger("test0");

  LOG_TRACE(logger0, "test0 {} {}", 0, 1);
  LOG_DEBUG(logger0, "test0 {} {}", 0, 1);
  LOG_INFO(logger0, "test0 {} {}", 0, 1);
  LOG_WARN(logger0, "test0 {} {}", 0, 1);
  LOG_ERROR(logger0, "test0 {} {}", 0, 1);

  auto logger1 = *createLogger("test1");

  LOG_TRACE(logger1, "test1 {} {}", 0, 1);
  LOG_DEBUG(logger1, "test1 {} {}", 0, 1);
  LOG_INFO(logger1, "test1 {} {}", 0, 1);
  LOG_WARN(logger1, "test1 {} {}", 0, 1);
  LOG_ERROR(logger1, "test1 {} {}", 0, 1);

  auto logger2 = *createLogger("test2");

  LOG_TRACE(logger2, "test2 {} {}", 0, 1);
  LOG_DEBUG(logger2, "test2 {} {}", 0, 1);
  LOG_INFO(logger2, "test2 {} {}", 0, 1);
  LOG_WARN(logger2, "test2 {} {}", 0, 1);
  LOG_ERROR(logger2, "test2 {} {}", 0, 1);
}

}  // namespace
