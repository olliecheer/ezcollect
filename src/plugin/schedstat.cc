#include "src/log.h"
// #include "src/plugin/nl_task_stats.h"
#include "src/plugin/plugin.h"
#include "src/procfs.h"
#include "src/utils.h"
#include <charconv>
// #include <sys/capability.h>

namespace Plugin {
enum Stat {
  cpu_schedule_delay,
  cpu_runtime,
};

static char const *PLUGIN_NAME = "schedstat";

struct Data {
  uint64_t sum_exec_runtime;
  uint64_t run_delay;
};

static bool read_schedstat(pid_t pid, struct Data &res) {
  using namespace std::string_literals;

  auto s = cat_str("/proc/"s + std::to_string(pid) + "/schedstat");
  if (s.empty())
    return false;

  auto views = split_as_view(s, ' ');
  std::from_chars(views[0].begin(), views[0].end(), res.sum_exec_runtime);
  std::from_chars(views[1].begin(), views[1].end(), res.run_delay);

  return true;
}

__attribute__((constructor)) static void ezcollect_init() {
  using namespace std::string_literals;

  auto &plugin_data = process_metrics[PLUGIN_NAME];

  plugin_data.ready = false;
  plugin_data.reason = "design broken";
  plugin_data.callback = [](pid_t pid, Vec<Metrics::StatEntry::Process> &res) {
    struct Data data;
    if (!read_schedstat(pid, data))
      return;

    res.push_back(Metrics::StatEntry::Process{
        .plugin_name = PLUGIN_NAME,
        .stat_name = "cpu_run_delay",
        .stat_type = Metrics::StatType::derive,
        .value = (double)data.run_delay,
        .pid = pid,
    });

    res.push_back(Metrics::StatEntry::Process{
        .plugin_name = PLUGIN_NAME,
        .stat_name = "cpu_runtime",
        .stat_type = Metrics::StatType::derive,
        .value = (double)data.sum_exec_runtime,
        .pid = pid,
    });
  };
}

}; // namespace Plugin
