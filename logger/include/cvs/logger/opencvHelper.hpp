#pragma once

#include <cvs/common/config.hpp>
#include <cvs/logger/argumentPreprocessor.hpp>
#include <cvs/logger/logging.hpp>
#include <fmt/chrono.h>
#include <opencv2/core/mat.hpp>
#include <opencv2/imgcodecs.hpp>

#include <filesystem>
#include <map>

namespace cvs::logger {

template <>
struct ArgumentPreprocessor<cv::Mat> {
  CVS_CONFIG(Config, "Logging parameters for the cv::Mat type.") {
    static const std::string& subfolderName() {
      static const std::string name =
          fmt::format("{:%Y.%m.%d-%H.%M.%S}",
                      fmt::localtime(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())));

      return name;
    }

    static const std::filesystem::path& defaultPath() {
      static const std::filesystem::path default_path = std::filesystem::temp_directory_path() / "cvslogger";
      return default_path;
    }

    CVS_FIELD(name, std::string, "Logger name.");
    CVS_FIELD_DEF(log_img, bool, true, "Enabled image logging.");
    CVS_FIELD_DEF(img_path, std::string, defaultPath().string(), "Root folder for saving images.");
    CVS_FIELD_DEF(img_subfolder, bool, true, "Create subfolder on every startup.");
  };

  static std::string exec(const LoggerPtr&          logger,
                          spdlog::level::level_enum lvl,
                          const common::Properties& ptree,
                          const cv::Mat&            arg) {
    try {
      auto config = Config::make(ptree).value();

      if (!config.log_img)
        return "Image{save disabled}";

      std::filesystem::path save_path = config.img_path;
      if (config.img_subfolder)
        save_path /= Config::subfolderName();
      save_path = save_path / logger->name() / spdlog::level::to_short_c_str(lvl);

      if (!std::filesystem::exists(save_path))
        std::filesystem::create_directories(save_path);

      auto cnt_iter = counters.find(logger->name());
      if (cnt_iter == counters.end())
        cnt_iter = counters.emplace(logger->name(), 0ul).first;

      auto filepath = save_path / (std::to_string(cnt_iter->second++) + ".png");

      if (cv::imwrite(filepath.string(), arg))
        return "cv::Mat(" + std::to_string(arg.cols) + "x" + std::to_string(arg.rows) + "){" + filepath.string() + "}";
      return "Image{can't save}";
    }
    catch (...) {
      std::throw_with_nested(std::runtime_error(fmt::format(R"(Can't log image for logger "{}".)", logger->name())));
    }
  }

  inline static std::map<std::string, std::size_t> counters;
};

}  // namespace cvs::logger
