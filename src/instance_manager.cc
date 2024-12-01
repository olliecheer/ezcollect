#include "src/instance_manager.h"
#include "src/procfs.h"

InstanceManager::InstanceManager(Config _config) : config(std::move(_config)) {
  auto const &match_config = config.match;
  all_processes = config.enable_process && match_config.pids.empty() &&
                  match_config.pcomm.empty() &&
                  match_config.pcomm_prefix.empty();

  all_threads = config.enable_thread && match_config.tids.empty() &&
                match_config.tcomm.empty() && match_config.tcomm_prefix.empty();
}

void InstanceManager::refresh() {
  DEBUG_FUNC();
  ProcessMap new_map;

  for (pid_t pid : get_all_pids()) {
    if (all_processes || config.match.try_match_pid(pid)) {
      new_map[pid].comm = std::make_shared<std::string>(get_pcomm(pid));
      continue;
    }

    auto pcomm = get_pcomm(pid);
    if (config.match.try_match_pcomm(pcomm) ||
        config.match.try_match_pcomm_prefix(pcomm)) {
      new_map[pid].comm = std::make_shared<std::string>(std::move(pcomm));
      continue;
    }
  }

  for (auto &it : new_map) {
    pid_t pid = it.first;
    auto &threads = it.second.threads;
    for (pid_t tid : tids_of_pid(pid)) {
      if (all_threads || config.match.try_match_tid(tid)) {
        threads[tid].comm = std::make_shared<std::string>(get_tcomm(pid, tid));
        continue;
      }

      auto tcomm = get_tcomm(pid, tid);
      if (config.match.try_match_tcomm(tcomm) ||
          config.match.try_match_tcomm_prefix(tcomm)) {
        threads[tid].comm = std::make_shared<std::string>(std::move(tcomm));
        continue;
      }
    }
  }

  process_map = std::move(new_map);
  dump_process_map();
}

void InstanceManager::for_each_system(
    Plugin::SystemCollectCallback const &callback) {
  callback(buffer.system);
}

void InstanceManager::for_each_process(
    Plugin::ProcessCollectCallback const &callback) {
  for (auto it : process_map) {
    pid_t pid = it.first;
    callback(pid, buffer.process);
  }
}

void InstanceManager::for_each_thread(
    Plugin::ThreadCollectCallback const &callback) {
  for (auto p_it : process_map) {
    pid_t pid = p_it.first;
    auto &thread_map = p_it.second.threads;
    for (auto t_it : thread_map) {
      pid_t tid = t_it.first;
      callback(pid, tid, buffer.thread);
    }
  }
}

void InstanceManager::clear() {
  buffer.system.clear();
  buffer.process.clear();
  buffer.thread.clear();
}

void InstanceManager::report(FILE *f, uint64_t ts_ns) {
  for (auto const &it : buffer.system) {
    fprintf(f, "%lu system:%s:%s %s %s %f\n", ts_ns, it.plugin_name,
            it.stat_name, it.stat_type, it.host_name, it.value);
  }

  for (auto const &it : buffer.process) {
    fprintf(f, "%lu process:%s:%s %s %d %s %f\n", ts_ns, it.plugin_name,
            it.stat_name, it.stat_type, it.pid,
            process_map[it.pid].comm->c_str(), it.value);
  }

  for (auto const &it : buffer.thread) {
    fprintf(f, "%lu thread:%s:%s %s %d %s %d %s %f\n", ts_ns, it.plugin_name,
            it.stat_name, it.stat_type, it.pid,
            process_map[it.pid].comm->c_str(), it.tid,
            process_map[it.pid].threads[it.tid].comm->c_str(), it.value);
  }
}

bool InstanceManager::Config::MatchConfig::try_match_pid(pid_t pid) {
  return pids.count(pid) != 0;
}

bool InstanceManager::Config::MatchConfig::try_match_pcomm(
    std::string const &s) {
  return pcomm.count(s) != 0;
}

bool InstanceManager::Config::MatchConfig::try_match_pcomm_prefix(
    std::string const &s) {
  for (auto const &it : pcomm_prefix) {
    if (s.find(it) == 0)
      return true;
  }

  return false;
}

bool InstanceManager::Config::MatchConfig::try_match_tid(pid_t tid) {
  return tids.count(tid) != 0;
}

bool InstanceManager::Config::MatchConfig::try_match_tcomm(
    std::string const &s) {
  return tcomm.count(s) != 0;
}

bool InstanceManager::Config::MatchConfig::try_match_tcomm_prefix(
    std::string const &s) {
  for (auto const &it : tcomm_prefix) {
    if (s.find(it) == 0)
      return true;
  }

  return false;
}
