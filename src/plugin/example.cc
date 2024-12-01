#include "src/env.h"
#include "src/plugin/plugin.h"

namespace Plugin {
static char const *PLUGIN_NAME = "example";
__attribute__((constructor)) static void ezcollect_init() {
  auto &plugin_data = system_metrics[PLUGIN_NAME];

  plugin_data.ready = true;
  plugin_data.callback = [](Vec<Metrics::StatEntry::System> &res) {
    res.push_back(Metrics::StatEntry::System{
        .host_name = env.hostname,
        .plugin_name = PLUGIN_NAME,
        .stat_name = "hello",
        .stat_type = Metrics::StatType::type2name[Metrics::StatType::COUNTER],
        .value = 1.01});
  };
}
}; // namespace Plugin
