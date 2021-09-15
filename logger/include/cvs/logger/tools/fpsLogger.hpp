#pragma once

#include <cvs/logger/loggable.hpp>

#include <chrono>
#include <memory>
#include <string_view>
#include <tuple>

namespace cvs::logger::tools {

class IFpsLogger : public Loggable<IFpsLogger> {
 public:
  using clock      = std::chrono::system_clock;
  using duration   = clock::duration;
  using time_point = clock::time_point;

  struct TotalStat {
    double      last_fps, smma_fps, fps;
    std::size_t total_cnt;
  };

  static std::unique_ptr<IFpsLogger> make(std::string_view name            = "cvs.logger.tools.fsplogger",
                                          double           ro              = 0.1,
                                          duration         report_duration = std::chrono::minutes(10),
                                          bool             autoreport      = true,
                                          bool             thread_safe     = false);

  IFpsLogger(std::string_view name);
  virtual ~IFpsLogger() = default;

  virtual double          ro() const             = 0;
  virtual const duration& reportDuration() const = 0;
  virtual bool            autoreport() const     = 0;

  virtual double    fps() const     = 0;
  virtual double    smmaFps() const = 0;
  virtual double    lastFps() const = 0;
  virtual TotalStat getStat() const = 0;

  virtual void start()         = 0;
  virtual bool started() const = 0;
  virtual void stop()          = 0;

  virtual void newFrame(time_point frame_time = clock::now()) = 0;
  virtual void clear()                                        = 0;

  virtual std::size_t framesCount() const = 0;
};

}  // namespace cvs::logger::tools
