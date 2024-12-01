#ifndef EZCOLLECT_RECORD_H
#define EZCOLLECT_RECORD_H

#include "src/alias.h"
#include "src/instance_manager.h"
#include <chrono>
#include <optional>
#include <variant>

class Recorder {
  template <typename T> using Opt = std::optional<T>;
  template <typename... Args> using Var = std::variant<Args...>;

public:
  struct Options;

  struct Options {
    InstanceManager::Config instance_config;
    FILE *output = stdout;
    uint64_t tick_interval_ms;
    uint64_t refresh_interval_ms;

    Vec<Plugin::SystemCollectCallback> system_collect_funcs;
    Vec<Plugin::ProcessCollectCallback> process_collect_funcs;
    Vec<Plugin::ThreadCollectCallback> thread_collect_funcs;
  };

private:
  struct Options opts;

  uint64_t n_ticks_to_refresh;
  uint64_t realtime_monotime_offset;

  std::chrono::milliseconds tick_interval;

  InstanceManager instance_manager;

public:
  explicit Recorder(Options const &_opts);
  void run();

private:
  void tick(uint64_t tick_id);
};

#endif
