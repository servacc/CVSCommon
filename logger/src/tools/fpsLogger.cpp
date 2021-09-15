#include "../include/cvs/logger/tools/fpsLogger.hpp"

#include "../include/cvs/logger/logging.hpp"

#include <fmt/chrono.h>

#include <list>
#include <shared_mutex>

namespace {

class FakeMutex {
 public:
  FakeMutex()  = default;
  ~FakeMutex() = default;

  FakeMutex(const FakeMutex&) = delete;
  FakeMutex& operator=(const FakeMutex&) = delete;

  void lock() {}
  bool try_lock() { return true; }
  void unlock() {}

  void lock_shared() {}
  bool try_lock_shared() { return true; }
  void unlock_shared() {}
};

}  // namespace

using namespace std::chrono_literals;

namespace cvs::logger::tools {

template <bool ThreadSafe = false>
class FpsLogger : public IFpsLogger {
  class Private;

 public:
  FpsLogger(std::string_view name, double ro, duration report_duration, bool autoreport);
  virtual ~FpsLogger();

  double          ro() const override;
  const duration& reportDuration() const override;
  bool            autoreport() const override;

  double    fps() const override;
  double    smmaFps() const override;
  double    lastFps() const override;
  TotalStat getStat() const override;

  void start() override;
  bool started() const override;
  void stop() override;

  void newFrame(time_point frame_time = clock::now()) override;
  void clear() override;

  std::size_t framesCount() const override;

 private:
  static double   fps(const duration&, std::size_t counter = 2);
  static duration avrTime(const duration&, std::size_t counter = 2);

  void update(duration last_dur);

  bool started_ = false;
  bool autolog  = true;

  time_point prev_frame_point;

  duration report_period = 10min;

  duration    report_dur;
  std::size_t report_cnt = 0;
  duration    total_dur;
  std::size_t total_cnt = 0;

  double last_fps = 0;
  double smma_fps = 0;
  double ro_      = 0.1;

  mutable std::conditional_t<ThreadSafe, std::shared_mutex, FakeMutex> update_mutex;
};

}  // namespace cvs::logger::tools

namespace cvs::logger::tools {

template <bool ThreadSafe>
class FpsLogger<ThreadSafe>::Private {
 public:
};

template <bool ThreadSafe>
void FpsLogger<ThreadSafe>::update(duration last_dur) {
  auto f = fps(last_dur);

  std::unique_lock lock(update_mutex);

  total_dur += last_dur;
  last_fps = f;
  smma_fps = (1 - ro_) * smma_fps + ro_ * f;
  ++total_cnt;
}

template <bool ThreadSafe>
double FpsLogger<ThreadSafe>::fps(const duration& dur, std::size_t counter) {
  return (counter - 1) / duration_cast<std::chrono::duration<double>>(dur).count();
}

template <bool ThreadSafe>
typename FpsLogger<ThreadSafe>::duration FpsLogger<ThreadSafe>::avrTime(const duration& dur, std::size_t counter) {
  return dur / (counter - 1);
}

template <bool ThreadSafe>
FpsLogger<ThreadSafe>::FpsLogger(std::string_view name, double ro, duration report_duration, bool autoreport)
    : IFpsLogger(name) {
  if (name.empty())
    LOG_GLOB_CRITICAL("The FpsLogger name must not be empty.");
  ro_           = ro;
  report_period = std::move(report_duration);
  autolog       = autoreport;
}

template <bool ThreadSafe>
FpsLogger<ThreadSafe>::~FpsLogger() {
  stop();
}

template <bool ThreadSafe>
double FpsLogger<ThreadSafe>::ro() const {
  return ro_;
}

template <bool ThreadSafe>
const typename FpsLogger<ThreadSafe>::duration& FpsLogger<ThreadSafe>::reportDuration() const {
  return report_period;
}

template <bool ThreadSafe>
bool FpsLogger<ThreadSafe>::autoreport() const {
  return autolog;
}

template <bool ThreadSafe>
void FpsLogger<ThreadSafe>::start() {
  if (!started_) {
    started_ = true;
    clear();
  }
}

template <bool ThreadSafe>
bool FpsLogger<ThreadSafe>::started() const {
  return started_;
}

template <bool ThreadSafe>
void FpsLogger<ThreadSafe>::stop() {
  if (started_) {
    started_ = false;

    if (autolog) {
      LOG_INFO(this->logger(), "{} frames for {}. Average FPS: {:.2f}.", total_cnt,
               std::chrono::duration_cast<std::chrono::milliseconds>(total_dur), fps());
    }
  }
}

template <bool ThreadSafe>
double FpsLogger<ThreadSafe>::fps() const {
  if (total_dur.count())
    return fps(total_dur, total_cnt);

  return -1;
}

template <bool ThreadSafe>
double FpsLogger<ThreadSafe>::smmaFps() const {
  return smma_fps;
}

template <bool ThreadSafe>
double FpsLogger<ThreadSafe>::lastFps() const {
  return last_fps;
}

template <bool ThreadSafe>
typename FpsLogger<ThreadSafe>::TotalStat FpsLogger<ThreadSafe>::getStat() const {
  std::shared_lock lock(update_mutex);

  TotalStat res{last_fps, smma_fps, fps(), total_cnt};

  return res;
}

template <bool ThreadSafe>
void FpsLogger<ThreadSafe>::newFrame(time_point frame_time) {
  if (!started_)
    return;

  if (total_cnt) {
    auto last_dur = frame_time - prev_frame_point;

    update(last_dur);

    if (autolog) {
      LOG_TRACE(this->logger(), "Frame {}. Current FPS {:.2f}. SMMA FPS {:.2f}. Mean FPS {:.2f}", total_cnt, last_fps,
                smma_fps, fps());

      report_dur += last_dur;
      ++report_cnt;
      if (report_dur >= report_period) {
        LOG_DEBUG(this->logger(), "FPS for last {} frames is {:.2f}.", report_cnt, fps(report_dur, report_cnt));

        report_cnt = 0;
        report_dur = duration{0};
      }
    }
  } else
    ++total_cnt;

  prev_frame_point = frame_time;
}

template <bool ThreadSafe>
void FpsLogger<ThreadSafe>::clear() {
  prev_frame_point = clock::now();
  report_dur       = duration{0};
  report_cnt       = 0;
  total_dur        = duration{0};
  total_cnt        = 0;
  smma_fps         = 0;
  last_fps         = 0;
}

template <bool ThreadSafe>
std::size_t FpsLogger<ThreadSafe>::framesCount() const {
  return total_cnt;
}
}  // namespace cvs::logger::tools

namespace cvs::logger::tools {

template class FpsLogger<true>;
template class FpsLogger<false>;

}  // namespace cvs::logger::tools

namespace cvs::logger::tools {

IFpsLogger::IFpsLogger(std::string_view name)
    : Loggable<IFpsLogger>(std::string{name}) {}

std::unique_ptr<IFpsLogger> IFpsLogger::make(std::string_view name,
                                             double           ro,
                                             duration         report_duration,
                                             bool             autoreport,
                                             bool             thread_safe) {
  if (thread_safe)
    return std::make_unique<FpsLogger<true>>(name, ro, std::move(report_duration), autoreport);
  else
    return std::make_unique<FpsLogger<false>>(name, ro, std::move(report_duration), autoreport);
}

}  // namespace cvs::logger::tools
