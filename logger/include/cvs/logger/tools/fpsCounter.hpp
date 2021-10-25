#pragma once

#include <cvs/common/config.hpp>

#include <chrono>
#include <functional>
#include <memory>
#include <string_view>

namespace cvs::logger::tools {

using IFPSCounterPtr  = std::shared_ptr<class IFPSCounter>;
using IFPSCounterUPtr = std::unique_ptr<class IFPSCounter>;

class IFPSCounter {
 public:
  using CounterClock     = std::chrono::system_clock;
  using CounterDuration  = CounterClock::duration;
  using CounterTimePoint = CounterClock::time_point;

  struct Statistics {
    CounterDuration last;
    CounterDuration total;
    double          smma;
  };

  using ResultFunctor =
      std::function<void(std::size_t frame, std::optional<Statistics> fps, std::optional<Statistics> latency)>;

  static IFPSCounterUPtr make(const std::string& name,
                              std::size_t        logged_frame = 10,
                              double             ro           = 0.1,
                              bool               thread_safe  = true);
  static IFPSCounterUPtr make(double ro, ResultFunctor fun, bool thread_safe = true);

  virtual ~IFPSCounter() = default;

  virtual void newFrame(CounterTimePoint frame_time = CounterClock::now())       = 0;
  virtual void frameProcessed(CounterTimePoint frame_time = CounterClock::now()) = 0;

  virtual void clear() = 0;

  virtual double statistics() const = 0;
};

}  // namespace cvs::logger::tools
