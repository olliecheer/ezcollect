#ifndef EZCOLLECT_PLUGIN_PLUGIN_H
#define EZCOLLECT_PLUGIN_PLUGIN_H

#include "src/alias.h"
#include "src/metrics.h"
#include <functional>
#include <map>

namespace Plugin {
using ThreadCollectCallback =
    std::function<void(pid_t, pid_t, Vec<Metrics::StatEntry::Thread> &)>;
using ProcessCollectCallback =
    std::function<void(pid_t, Vec<Metrics::StatEntry::Process> &)>;
using SystemCollectCallback =
    std::function<void(Vec<Metrics::StatEntry::System> &)>;

struct SystemCollectPlugin {
  SystemCollectCallback callback;
  bool ready;
  std::string reason;
  // std::string plugin_name;
  // std::unordered_map<std::string, SystemCollectMetricInfo> metrics;
};

struct ProcessCollectPlugin {
  ProcessCollectCallback callback;
  bool ready;
  std::string reason;
  // std::string plugin_name;
  // std::unordered_map<std::string, ProcessCollectMetricInfo> metrics;
};

struct ThreadCollectPlugin {
  ThreadCollectCallback callback;
  bool ready;
  std::string reason;
  // std::string plugin_name;
  // std::unordered_map<std::string, ThreadCollectMetricInfo> metrics;
};

extern std::map<std::string, SystemCollectPlugin> system_metrics;
extern std::map<std::string, ProcessCollectPlugin> process_metrics;
extern std::map<std::string, ThreadCollectPlugin> thread_metrics;

void dump_available_metrics(bool as_err = false);
}; // namespace Plugin

#endif
