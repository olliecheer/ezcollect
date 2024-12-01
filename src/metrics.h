#ifndef EZCOLLECT_METRICS_H
#define EZCOLLECT_METRICS_H

// #include <memory>
#include <string>
#include <unordered_map>

namespace Metrics {
namespace StatType {
enum Type {
  NONE = 0,
  GAUGE,
  DERIVE,
  COUNTER,
};

static char const *gauge = "gauge";
static char const *derive = "derive";
static char const *counter = "counter";

char const *const type2name[] = {
    "none",
    "gauge",
    "derive",
    "counter",
};

std::unordered_map<std::string, enum Type> const name2type = {
    {"none", StatType::NONE},
    {"gauge", StatType::GAUGE},
    {"derive", StatType::DERIVE},
    {"counter", StatType::COUNTER},
};
}; // namespace StatType

namespace StatEntry {
struct System {
  char const *host_name;
  char const *plugin_name;
  char const *stat_name;
  char const *stat_type;
  double value;
};
struct Process {
  char const *plugin_name;
  char const *stat_name;
  char const *stat_type;
  double value;
  pid_t pid;
  // std::shared_ptr<std::string> pcomm;
};
struct Thread {
  char const *plugin_name;
  char const *stat_name;
  char const *stat_type;
  double value;
  pid_t pid;
  pid_t tid;
  // std::shared_ptr<std::string> pcomm, tcomm;
};
}; // namespace StatEntry

}; // namespace Metrics

#endif
