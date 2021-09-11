#include "../include/cvs/logger/tools/fpsLogger.hpp"

#include "../include/cvs/logger/logging.hpp"

#include <fmt/chrono.h>

#include <list>
#include <shared_mutex>

using namespace std::chrono_literals;

using duration   = cvs::logger::tools::FpsLogger::duration;
using time_point = cvs::logger::tools::FpsLogger::time_point;

namespace cvs::logger::tools {

class FpsLogger::Private {
 public:
  static double   fps(const duration&, std::size_t counter = 2);
  static duration avrTime(const duration&, std::size_t counter = 2);

  void update(duration last_dur);

  bool started  = false;
  bool autolog  = true;
  bool use_lock = false;

  time_point prev_frame_point;

  duration report_period = 10min;

  duration    report_dur;
  std::size_t report_cnt = 0;
  duration    total_dur;
  std::size_t total_cnt = 0;

  double last_fps = 0;
  double smma_fps = 0;
  double ro       = 0.1;

  mutable std::shared_mutex update_mutex;
};

void FpsLogger::Private::update(duration last_dur) {
  auto f = fps(last_dur);
  if (use_lock)
    update_mutex.lock();
  total_dur += last_dur;
  last_fps = f;
  smma_fps = (1 - ro) * smma_fps + ro * f;
  ++total_cnt;
  if (use_lock)
    update_mutex.unlock();
}

double FpsLogger::Private::fps(const duration& dur, std::size_t counter) {
  return (counter - 1) / duration_cast<std::chrono::duration<double>>(dur).count();
}

duration FpsLogger::Private::avrTime(const duration& dur, std::size_t counter) { return dur / (counter - 1); }

}  // namespace cvs::logger::tools

namespace cvs::logger::tools {

FpsLogger::FpsLogger(std::string_view name)
    : Loggable<FpsLogger>(std::string{name})
    , m(std::make_shared<Private>()) {
  if (name.empty())
    LOG_GLOB_CRITICAL("The FpsLogger name must not be empty.");
}

FpsLogger::~FpsLogger() { stop(); }

void   FpsLogger::setRo(double ro) { m->ro = ro; }
double FpsLogger::ro() const { return m->ro; }

void            FpsLogger::setReportDuration(duration d) { m->report_period = std::move(d); }
const duration& FpsLogger::reportDuration() const { return m->report_period; }

void FpsLogger::setAutoreport(bool enable) { m->autolog = enable; }
bool FpsLogger::autoreport() const { return m->autolog; }

void FpsLogger::start() {
  if (!m->started) {
    m->started = true;
    clear();
  }
}

bool FpsLogger::started() const { return m->started; }

void FpsLogger::stop() {
  if (m->started) {
    m->started = false;

    if (m->autolog) {
      LOG_INFO(logger(), "{} frames for {}. Average FPS: {:.2f}.", m->total_cnt,
               duration_cast<std::chrono::milliseconds>(m->total_dur), fps());
    }
  }
}

double FpsLogger::fps() const {
  if (m->total_dur.count())
    return m->fps(m->total_dur, m->total_cnt);

  return -1;
}

double FpsLogger::smmaFps() const { return m->smma_fps; }

double FpsLogger::lastFps() const { return m->last_fps; }

FpsLogger::TotalStat FpsLogger::getStat() const {
  if (m->use_lock)
    m->update_mutex.lock_shared();
  TotalStat res{m->last_fps, m->smma_fps, fps(), m->total_cnt};
  if (m->use_lock)
    m->update_mutex.unlock_shared();
  return res;
}

void FpsLogger::setUseLock(bool use_lock) { m->use_lock = use_lock; }

void FpsLogger::newFrame(time_point frame_time) {
  if (!m->started)
    return;

  if (m->total_cnt) {
    auto last_dur = frame_time - m->prev_frame_point;

    m->update(last_dur);

    if (m->autolog) {
      LOG_TRACE(logger(), "Frame {}. Current FPS {:.2f}. SMMA FPS {:.2f}. Mean FPS {:.2f}", m->total_cnt, m->last_fps,
                m->smma_fps, fps());

      m->report_dur += last_dur;
      ++m->report_cnt;
      if (m->report_dur >= m->report_period) {
        LOG_DEBUG(logger(), "FPS for last {} frames is {:.2f}.", m->report_cnt, m->fps(m->report_dur, m->report_cnt));

        m->report_cnt = 0;
        m->report_dur = duration{0};
      }
    }
  } else
    ++m->total_cnt;

  m->prev_frame_point = frame_time;
}

void FpsLogger::clear() {
  m->prev_frame_point = clock::now();
  m->report_dur       = duration{0};
  m->report_cnt       = 0;
  m->total_dur        = duration{0};
  m->total_cnt        = 0;
  m->smma_fps         = 0;
  m->last_fps         = 0;
}

std::size_t FpsLogger::framesCount() const { return m->total_cnt; }
}  // namespace cvs::logger::tools
