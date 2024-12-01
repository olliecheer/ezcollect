#ifndef EZCOLLECT_INSTANCE_REFRESH_H
#define EZCOLLECT_INSTANCE_REFRESH_H

#include "src/alias.h"
#include "src/log.h"
#include "src/metrics.h"
#include "src/plugin/plugin.h"
#include <set>
#include <unordered_map>

class InstanceManager {
public:
  struct Config {
    struct MatchConfig {
      std::set<pid_t> pids;
      std::set<std::string> pcomm;
      std::set<std::string> pcomm_prefix;

      std::set<pid_t> tids;
      std::set<std::string> tcomm;
      std::set<std::string> tcomm_prefix;

      bool try_match_pid(pid_t pid);
      bool try_match_pcomm(std::string const &s);
      bool try_match_pcomm_prefix(std::string const &s);

      bool try_match_tid(pid_t tid);
      bool try_match_tcomm(std::string const &s);
      bool try_match_tcomm_prefix(std::string const &s);

    } match;
    bool enable_process;
    bool enable_thread;
  };

public:
  InstanceManager(Config _config);
  void refresh();
  void for_each_thread(Plugin::ThreadCollectCallback const &callback);
  void for_each_process(Plugin::ProcessCollectCallback const &callback);
  void for_each_system(Plugin::SystemCollectCallback const &callback);
  void report(FILE *f, uint64_t ts_ns);
  void clear();

private:
  bool all_processes;
  bool all_threads;
  // bool enable_process;
  // bool enable_thread;
  // bool enable_system;

  struct ThreadInfo {
    std::shared_ptr<std::string> comm;
  };
  using ThreadMap = std::unordered_map<pid_t, struct ThreadInfo>;

  struct ProcessInfo {
    std::shared_ptr<std::string> comm;
    ThreadMap threads;
  };
  using ProcessMap = std::unordered_map<pid_t, struct ProcessInfo>;

  void dump_process_map() {
    for (auto const &it_p : process_map) {
      pid_t pid = it_p.first;
      auto const &pcomm = it_p.second.comm;
      DEBUG("%d-%s\n", pid, pcomm->c_str());
      for (auto const &it_t : it_p.second.threads) {
        pid_t tid = it_t.first;
        auto const &tcomm = it_t.second.comm;
        DEBUG("    %d-%s\n", tid, tcomm->c_str());
      }
    }
  }

  ProcessMap process_map;

  struct Buffer {
    Vec<Metrics::StatEntry::System> system;
    Vec<Metrics::StatEntry::Process> process;
    Vec<Metrics::StatEntry::Thread> thread;
  } buffer;

private:
  Config config;
};

#endif
