#include <boost/property_tree/json_parser.hpp>
#include <cvs/common/config.hpp>
#include <cvs/common/general.hpp>
#include <cvs/logger/logging.hpp>
#include <cvs/logger/opencvHelper.hpp>
#include <gtest/gtest.h>

namespace {

TEST(CVSLoggerTest, opencv_cfg_help) {
  std::cout << cvs::logger::ArgumentPreprocessor<cv::Mat>::Config::describe() << std::endl;
}

TEST(CVSLoggerTest, opencv_cfg) {
  std::string config_str = R"({
  "name": "test",
  "log_img": true,
  "img_path": ")" TEST_DIR R"(",
  "level": "debug",
  "sink": "std"
})";

  cvs::logger::initLoggers();

  std::stringstream ss;
  ss << config_str;
  cvs::common::Properties json;

  ASSERT_NO_THROW(boost::property_tree::read_json(ss, json));

  auto logger_path =
      std::filesystem::path(TEST_DIR) / cvs::logger::ArgumentPreprocessor<cv::Mat>::Config::subfolderName() / "test";
  if (std::filesystem::exists(logger_path))
    EXPECT_TRUE(std::filesystem::remove_all(logger_path));

  auto logger = cvs::logger::createLogger(json).value();

  cv::Mat mat(300, 300, CV_8UC3);
  LOG_TRACE(logger, "Trace {}", mat);
  LOG_INFO(logger, "Info {}", mat);

  EXPECT_FALSE(std::filesystem::exists(logger_path / "T" / "0.png"));
  EXPECT_TRUE(std::filesystem::exists(logger_path / "I" / "0.png"));

  std::filesystem::remove_all(TEST_DIR);
}

}  // namespace
