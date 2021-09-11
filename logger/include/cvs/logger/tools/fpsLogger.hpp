#pragma once

#include <cvs/logger/loggable.hpp>

#include <chrono>
#include <memory>
#include <string_view>
#include <tuple>

namespace cvs::logger::tools {

class FpsLogger : public Loggable<FpsLogger> {
  class Private;

 public:
  using clock      = std::chrono::system_clock;
  using duration   = clock::duration;
  using time_point = clock::time_point;

  struct TotalStat {
    double      last_fps, smma_fps, fps;
    std::size_t total_cnt;
  };

  FpsLogger(std::string_view name = "cvs.logger.tools.fsplogger");
  virtual ~FpsLogger();

  void   setRo(double);
  double ro() const;

  void            setReportDuration(duration);
  const duration& reportDuration() const;

  void setAutoreport(bool);
  bool autoreport() const;

  double    fps() const;
  double    smmaFps() const;
  double    lastFps() const;
  TotalStat getStat() const;
  void      setUseLock(bool use_lock = true);

  void start();
  bool started() const;
  void stop();

  void newFrame(time_point frame_time = clock::now());
  void clear();

  std::size_t framesCount() const;

 private:
  std::shared_ptr<Private> m;
};

}  // namespace cvs::logger::tools
