#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <string_view>

namespace cvs::logger::tools {

class IFPSCounter {
 public:
  using clock      = std::chrono::system_clock;
  using duration   = clock::duration;
  using time_point = clock::time_point;

  using Functor = std::function<void(std::size_t, duration, duration, double)>;

  static std::unique_ptr<IFPSCounter> make(const std::string &name, double ro = 0.1, bool thread_safe = true);
  static std::unique_ptr<IFPSCounter> make(double ro, Functor fun, bool thread_safe = true);

  virtual ~IFPSCounter() = default;

  virtual void newFrame(time_point frame_time = clock::now()) = 0;

  virtual void start() = 0;
  virtual void stop()  = 0;

  virtual std::pair<std::size_t, duration> frames() const = 0;
};

}  // namespace cvs::logger::tools
