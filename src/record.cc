#include "src/record.h"
#include "src/log.h"
#include "src/procfs.h"
#include <thread>

Recorder::Recorder(Options const &_opts)
    : instance_manager(_opts.instance_config), opts(_opts) {
  opts.tick_interval_ms = std::max<uint64_t>(1, opts.tick_interval_ms);
  n_ticks_to_refresh =
      std::max<uint64_t>(1, opts.refresh_interval_ms / opts.tick_interval_ms);
  tick_interval = std::chrono::milliseconds(opts.tick_interval_ms);
  realtime_monotime_offset =
      std::chrono::system_clock::now().time_since_epoch().count() -
      std::chrono::steady_clock::now().time_since_epoch().count();
}

static void periodic_work(std::chrono::nanoseconds interval,
                          std::function<void(uint64_t tick_id)> const &tick,
                          std::atomic<bool> const &stop = {}) {

  auto now = []() { return std::chrono::steady_clock::now(); };

  auto start_ts_ns = now();

  uint64_t tick_counter = 0;

  while (!stop.load(std::memory_order_relaxed)) {
    tick(tick_counter);
    ++tick_counter;
    auto dur_to_sleep = std::chrono::nanoseconds(
        interval.count() - (now() - start_ts_ns).count() % interval.count());
    std::this_thread::sleep_for(dur_to_sleep);
  }
}

void Recorder::run() {
  periodic_work(
      tick_interval, [this](uint64_t tick_id) { this->tick(tick_id); },
      env.shutdown);
}

void Recorder::tick(uint64_t tick_id) {
  auto now = std::chrono::steady_clock::now().time_since_epoch().count() +
             realtime_monotime_offset;

  DEBUG("tick %lu at %lu\n", tick_id, now);

  if (tick_id % n_ticks_to_refresh == 0)
    instance_manager.refresh();

  for (auto const &callback : opts.system_collect_funcs) {
    DEBUG("call system collect\n");
    instance_manager.for_each_system(callback);
  }

  for (auto const &callback : opts.process_collect_funcs) {
    DEBUG("call process collect\n");
    instance_manager.for_each_process(callback);
  }

  for (auto const &callback : opts.thread_collect_funcs) {
    DEBUG("call thread collect\n");
    instance_manager.for_each_thread(callback);
  }

  instance_manager.report(opts.output, now);
  instance_manager.clear();
}
