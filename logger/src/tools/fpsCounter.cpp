#include "../include/cvs/logger/tools/fpsCounter.hpp"

#include "../include/cvs/logger/logging.hpp"

#include <cvs/common/config.hpp>
#include <fmt/chrono.h>

#include <queue>
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
  using CounterClock     = std::chrono::system_clock;
  using CounterDuration  = CounterClock::duration;
  using CounterTimePoint = CounterClock::time_point;

  FPSCounter(double r, ResultFunctor f)
      : ro(r)
      , functor(std::move(f)) {
    clear();
  }

  void newFrame(CounterTimePoint frame_time = CounterClock::now()) override {
    std::unique_lock lock(update_mutex);
    start_points.push(frame_time);
  }

  void frameProcessed(CounterTimePoint frame_time = CounterClock::now()) override {
    std::size_t frame_copy = 0;

    std::optional<Statistics> result_fps;
    std::optional<Statistics> result_latency;

    {
      std::unique_lock lock(update_mutex);

      if (frame_count == 0) {
        prev_frame_time = frame_time;
        total_frames    = CounterDuration::zero();
      } else {
        auto last_frame = frame_time - prev_frame_time;
        prev_frame_time = frame_time;
        auto cur_fps    = 1 / duration_cast<std::chrono::duration<double>>(last_frame).count();
        smma_fps        = (1 - ro) * smma_fps + ro * cur_fps;
        total_frames += last_frame;
        result_fps = Statistics{last_frame, total_frames, smma_fps};
      }

      if (!start_points.empty()) {
        auto last_latency = frame_time - start_points.front();
        total_latency += last_latency;

        ++frame_count;
        start_points.pop();

        smma_latency = (1 - ro) * smma_latency + ro * last_latency.count();

        result_latency = Statistics{last_latency, total_latency, smma_latency};
      }

      frame_copy = frame_count;
    }

    functor(frame_copy, result_fps, result_latency);
  }

  void clear() override final {
    std::unique_lock lock(update_mutex);

    total_frames  = CounterDuration::zero();
    total_latency = CounterDuration::zero();
    frame_count   = 0;
    smma_fps      = 0;
    smma_latency  = 0;
  }

  double statistics() const override {
    std::shared_lock lock(update_mutex);
    return {};
  }

 private:
  mutable std::conditional_t<ThreadSafe, std::shared_mutex, FakeMutex> update_mutex;

  std::queue<CounterTimePoint> start_points;

  CounterTimePoint prev_frame_time;
  CounterDuration  total_frames;

  CounterDuration total_latency;
  std::size_t     frame_count = 0;

  double       smma_fps     = 0;
  double       smma_latency = 0;
  const double ro;

  ResultFunctor functor;
};

template class FPSCounter<true>;
template class FPSCounter<false>;

}  // namespace cvs::logger::tools

namespace cvs::logger::tools {

IFPSCounterUPtr IFPSCounter::make(const std::string& name, std::size_t logged_frame, double ro, bool thread_safe) {
  auto          logger         = cvs::logger::createLogger(name).value();
  auto          period_counter = std::make_shared<std::atomic_size_t>();
  ResultFunctor f = [logged_frame, logger, period_counter](std::size_t frame, std::optional<Statistics> fps,
                                                           std::optional<Statistics> latency) {
    if (fps) {
      auto current_fps = 1 / duration_cast<std::chrono::duration<double>>(fps->last).count();
      auto mean_fps    = frame / duration_cast<std::chrono::duration<double>>(fps->total).count();

      LOG_TRACE(logger, "FPS: frame {} current {:.2f} SMMA {:.2f} mean {:.2f}", frame, current_fps, fps->smma,
                mean_fps);

      if (frame % logged_frame == 0) {
        LOG_DEBUG(logger, "FPS: frame {} SMMA {:.2f} mean {:.2f}", frame, current_fps, fps->smma, mean_fps);
      }
    }

    if (latency) {
      auto                          mean_latency = latency->total / frame;
      std::chrono::duration<double> smma(latency->smma / CounterDuration::period::den);

      LOG_TRACE(logger, "Latency: frame {} current {} SMMA {} mean {}", frame,
                std::chrono::duration_cast<std::chrono::milliseconds>(latency->last),
                std::chrono::duration_cast<std::chrono::milliseconds>(smma),
                std::chrono::duration_cast<std::chrono::milliseconds>(mean_latency));

      if (frame % logged_frame == 0) {
        LOG_DEBUG(logger, "Latency: frame {} SMMA {} mean {}", frame,
                  std::chrono::duration_cast<std::chrono::milliseconds>(latency->last),
                  std::chrono::duration_cast<std::chrono::milliseconds>(smma),
                  std::chrono::duration_cast<std::chrono::milliseconds>(mean_latency));
      }
    }
  };

  return make(ro, std::move(f), thread_safe);
}

IFPSCounterUPtr IFPSCounter::make(double ro, ResultFunctor fun, bool thread_safe) {
  if (thread_safe)
    return std::make_unique<FPSCounter<true>>(ro, std::move(fun));
  return std::make_unique<FPSCounter<false>>(ro, std::move(fun));
}

}  // namespace cvs::logger::tools
