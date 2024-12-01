#include "src/plugin/plugin.h"

namespace Plugin {
std::map<std::string, SystemCollectPlugin> system_metrics
    __attribute__((init_priority(101)));
std::map<std::string, ProcessCollectPlugin> process_metrics
    __attribute__((init_priority(101)));
std::map<std::string, ThreadCollectPlugin> thread_metrics
    __attribute__((init_priority(101)));

void dump_available_metrics(bool as_err) {
  FILE *f = stdout;
  if (as_err)
    f = stderr;

  fprintf(f, "available system metrics:\n");
  for (auto const &it : system_metrics) {
    if (it.second.ready)
      fprintf(f, "  system:%s\n", it.first.c_str());
    else
      fprintf(f, "  system:%s (not ready: %s)\n", it.first.c_str(),
              it.second.reason.c_str());
  }
  fprintf(f, "\n");

  fprintf(f, "available process metrics:\n");
  for (auto const &it : process_metrics) {
    if (it.second.ready)
      fprintf(f, "  process:%s\n", it.first.c_str());
    else
      fprintf(f, "  process:%s (not ready: %s)\n", it.first.c_str(),
              it.second.reason.c_str());
  }
  fprintf(f, "\n");

  fprintf(f, "available thread metrics:\n");
  for (auto const &it : thread_metrics) {
    if (it.second.ready)
      fprintf(f, "  thread:%s\n", it.first.c_str());
    else
      fprintf(f, "  thread:%s (not ready: %s)\n", it.first.c_str(),
              it.second.reason.c_str());
  }
  fprintf(f, "\n");
}
}; // namespace Plugin
