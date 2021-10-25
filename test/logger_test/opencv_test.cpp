#include <cvs/common/factory.hpp>
#include <cvs/logger/logging.hpp>
#include <gtest/gtest.h>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

using namespace std::string_literals;

namespace {

void drawRandomLines(cv::Mat& image, cv::RNG rng = {}) {
  constexpr int lineType   = 8;
  constexpr int line_count = 8;
  cv::Point     pt1, pt2;
  for (int i = 0; i < line_count; ++i) {
    pt1.x = rng.uniform(0, image.cols);
    pt1.y = rng.uniform(0, image.rows);
    pt2.x = rng.uniform(0, image.cols);
    pt2.y = rng.uniform(0, image.rows);
    cv::line(image, pt1, pt2, cv::Scalar(rng.uniform(0, 255), rng.uniform(0, 255), rng.uniform(0, 255)),
             rng.uniform(1, 10), lineType);
  }
}

TEST(CVSLoggerTest, opencv) {
  try {
    cv::Mat mat(300, 300, CV_8UC3, cv::Scalar(0));
    drawRandomLines(mat);

    auto logger = *cvs::logger::createLogger("test.logger"s);

    ASSERT_NE(nullptr, logger);

    LOG_INFO(logger, "Save to {}", mat);
    LOG_INFO(logger, "Save to {}", mat);

    auto logger_path = cvs::logger::ArgumentPreprocessor<cv::Mat>::Config::defaultPath() /
                       cvs::logger::ArgumentPreprocessor<cv::Mat>::Config::subfolderName() / "test.logger";

    ASSERT_TRUE(std::filesystem::exists(logger_path / "I" / "0.png"));
    ASSERT_TRUE(std::filesystem::exists(logger_path / "I" / "1.png"));
  }
  catch (std::exception& e) {
    std::cout << cvs::common::exceptionStr(e) << std::endl;
  }
}

}  // namespace
