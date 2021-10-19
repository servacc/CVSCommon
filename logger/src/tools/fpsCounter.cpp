#include "../include/cvs/logger/tools/fpsCounter.hpp"

#include "../include/cvs/logger/logging.hpp"

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

namespace cvs::logger::tools {

template <bool ThreadSafe = true>
class FPSCounter : public IFPSCounter {
 public:
  using clock      = std::chrono::system_clock;
  using duration   = clock::duration;
  using time_point = clock::time_point;

  FPSCounter(double r, Functor f)
      : ro(r)
      , functor(std::move(f)) {}

  void newFrame(time_point frame_time = clock::now()) {
    std::unique_lock lock(update_mutex);

    if (!started)
      return;

    auto last_frame_duration = frame_time - last_frame_time;
    total_duration += last_frame_duration;
    ++frame_count;

    auto fps = frame_count / duration_cast<std::chrono::duration<double>>(total_duration).count();
    smma_fps = (1 - ro) * smma_fps + ro * fps;

    auto frame_count_copy    = frame_count;
    auto total_duration_copy = total_duration;
    auto smma_fps_copy       = smma_fps;

    lock.unlock();

    functor(frame_count_copy, total_duration_copy, last_frame_duration, smma_fps_copy);
  }

  void start() {
    std::unique_lock lock(update_mutex);

    started         = true;
    last_frame_time = clock::now();
    total_duration  = duration::zero();
    frame_count     = 0;
    smma_fps        = 0;
  }

  void stop() {
    std::unique_lock lock(update_mutex);

    started = false;
  }

  std::pair<std::size_t, duration> frames() const {
    std::shared_lock lock(update_mutex);
    return {frame_count, total_duration};
  }

 private:
  mutable std::conditional_t<ThreadSafe, std::shared_mutex, FakeMutex> update_mutex;

  bool        started = false;
  time_point  last_frame_time;
  duration    total_duration;
  std::size_t frame_count = 0;

  double       smma_fps = 0;
  const double ro;

  Functor functor;
};

template class FPSCounter<true>;
template class FPSCounter<false>;

}  // namespace cvs::logger::tools

namespace cvs::logger::tools {

std::unique_ptr<IFPSCounter> IFPSCounter::make(const std::string& name, double ro, bool thread_safe) {
  duration log_duration = std::chrono::seconds(5);
  Functor  f;
  auto     q = [log_duration, logger = cvs::logger::createLogger(name).value(),
            period_counter = std::make_unique<std::atomic_size_t>()](std::size_t frame, duration total_duration,
                                                                     duration last_frame_duration, double smma) {
    auto last_frame_fps = 1 / duration_cast<std::chrono::duration<double>>(last_frame_duration).count();
    auto average_fps    = frame / duration_cast<std::chrono::duration<double>>(total_duration).count();

    LOG_TRACE(logger, "Frame {}. Current FPS {:.2f}. SMMA FPS {:.2f}. Mean FPS {:.2f}", frame, last_frame_fps, smma,
              average_fps);

    std::size_t period_number   = total_duration / log_duration;
    auto        expected_period = period_counter->load();
    if (period_number > expected_period && period_counter->compare_exchange_strong(expected_period, period_number)) {
      LOG_DEBUG(logger, "Frame {}. SMMA FPS {:.2f}. Mean FPS {:.2f}", frame, smma, average_fps);
    }
  };

  return make(ro, std::move(f), thread_safe);
}

std::unique_ptr<IFPSCounter> IFPSCounter::make(double ro, Functor fun, bool thread_safe) {
  if (thread_safe)
    return std::make_unique<FPSCounter<true>>(ro, std::move(fun));
  return std::make_unique<FPSCounter<false>>(ro, std::move(fun));
}

}  // namespace cvs::logger::tools
