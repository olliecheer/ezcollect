#include "src/log.h"
#include "src/plugin/nl_task_stats.h"
#include "src/plugin/plugin.h"
#include <sys/capability.h>

namespace Plugin {
enum Stat {
  cpu_delay_total,
  cpu_run_real_total,
};

char const *StatName[]{
    "cpu_delay_total",
    "cpu_run_real_total",
};

char const *PLUGIN_NAME = "taskstats";

static bool check_capability() {
  cap_t caps = cap_get_proc();
  cap_flag_value_t cap_value;

  bool res =
      caps != nullptr &&
      cap_get_flag(caps, CAP_NET_ADMIN, CAP_EFFECTIVE, &cap_value) == 0 &&
      cap_value == CAP_SET;

  cap_free(caps);

  return res;
}

__attribute__((constructor)) static void ezcollect_init() {
  using namespace std::string_literals;

  auto &thread_plugin_data = thread_metrics[PLUGIN_NAME];
  auto &process_plugin_data = process_metrics[PLUGIN_NAME];

  if (!check_capability()) {
    DEBUG("no NET_ADMIN capability for %s\n", PLUGIN_NAME);
    process_plugin_data.ready = false;
    process_plugin_data.reason = "need CAP_NET_ADMIN capability";
    thread_plugin_data.ready = false;
    thread_plugin_data.reason = "need CAP_NET_ADMIN capability";
  } else {
    process_plugin_data.ready = true;
    thread_plugin_data.ready = true;
  }

  thread_plugin_data.callback = [](pid_t pid, pid_t tid,
                                   Vec<Metrics::StatEntry::Thread> &res) {
    static struct taskstats buffer;
    if (!NetlinkTaskstats::query(tid, buffer))
      return;

    res.push_back(Metrics::StatEntry::Thread{
        .plugin_name = PLUGIN_NAME,
        .stat_name = "cpu_delay_total",
        .stat_type = Metrics::StatType::type2name[Metrics::StatType::DERIVE],
        .value = double(buffer.cpu_delay_total),
        .pid = pid,
        .tid = tid,
    });

    res.push_back(Metrics::StatEntry::Thread{
        .plugin_name = PLUGIN_NAME,
        .stat_name = "cpu_runtime",
        .stat_type = Metrics::StatType::type2name[Metrics::StatType::DERIVE],
        .value = double(buffer.cpu_run_real_total),
        .pid = pid,
        .tid = tid,
    });
  };

  process_plugin_data.callback = [](pid_t pid,
                                    Vec<Metrics::StatEntry::Process> &res) {
    static struct taskstats buffer;
    if (!NetlinkTaskstats::query_tgid(pid, buffer))
      return;

    res.push_back(Metrics::StatEntry::Process{
        .plugin_name = PLUGIN_NAME,
        .stat_name = "cpu_delay_total",
        .stat_type = Metrics::StatType::type2name[Metrics::StatType::DERIVE],
        .value = double(buffer.cpu_delay_total),
        .pid = pid,
    });

    res.push_back(Metrics::StatEntry::Process{
        .plugin_name = PLUGIN_NAME,
        .stat_name = "cpu_runtime",
        .stat_type = Metrics::StatType::type2name[Metrics::StatType::DERIVE],
        .value = double(buffer.cpu_run_real_total),
        .pid = pid,
    });
  };
}

}; // namespace Plugin
