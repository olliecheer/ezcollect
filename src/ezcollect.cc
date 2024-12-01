#include "src/log.h"
#include "src/procfs.h"
#include "src/record.h"
#include "src/summarize.h"
#include "src/utils.h"
#include "third/argparse.h"
#include <signal.h>
#include <string>

struct Env env = {
    .log_level = LogLevel::INFO,
    .shutdown = false,
};

int handle_record_cmd(argparse::ArgumentParser const &record_cmd) {
  if (record_cmd["-l"] == true) {
    Plugin::dump_available_metrics();
    return 0;
  }

  if (!record_cmd.is_used("-p") && !record_cmd.is_used("--pcomm") &&
      !record_cmd.is_used("--pcomm-prefix")) {
    DEBUG("No process filter rule is specified\n");
  }

  uint64_t tick_interval_ms = std::stoul(record_cmd.get("-i"));

  Recorder::Options opts = {
      .tick_interval_ms = tick_interval_ms,
      .refresh_interval_ms = 10'000,
  };

  auto &match_config = opts.instance_config.match;

  if (record_cmd.is_used("-p")) {
    auto const &pid_list_s = record_cmd.get("-p");
    std::set<pid_t> pids;
    split_foreach(pid_list_s, ',', [&pids](auto const &s) {
      pid_t pid;
      std::from_chars(s.begin(), s.end(), pid);
      pids.insert(pid);
    });

    match_config.pids = std::move(pids);
  }

  if (record_cmd.is_used("--pcomm")) {
    auto const &pcomm_list_s = record_cmd.get("--pcomm");
    std::set<std::string> pcomms;
    split_foreach(pcomm_list_s, ',',
                  [&pcomms](auto const &s) { pcomms.insert(std::string(s)); });
    match_config.pcomm = std::move(pcomms);
  }

  if (record_cmd.is_used("--pcomm-prefix")) {
    auto const &pcomm_prefix_list_s = record_cmd.get("--pcomm-prefix");
    std::set<std::string> pcomms_prefix;
    split_foreach(pcomm_prefix_list_s, ',', [&pcomms_prefix](auto const &s) {
      pcomms_prefix.insert(std::string(s));
    });
    match_config.pcomm_prefix = std::move(pcomms_prefix);
  }

  if (record_cmd.is_used("-t")) {
    auto const &tid_list_s = record_cmd.get("-t");
    std::set<pid_t> tids;
    split_foreach(tid_list_s, ',', [&tids](auto const &s) {
      pid_t pid;
      std::from_chars(s.begin(), s.end(), pid);
      tids.insert(pid);
    });

    match_config.tids = std::move(tids);
  }

  if (record_cmd.is_used("--tcomm")) {
    auto const &tcomm_list_s = record_cmd.get("--tcomm");
    std::set<std::string> tcomms;
    split_foreach(tcomm_list_s, ',',
                  [&tcomms](auto const &s) { tcomms.insert(std::string(s)); });

    match_config.tcomm = std::move(tcomms);
  }

  if (record_cmd.is_used("--tcomm-prefix")) {
    auto const &tcomm_prefix_list_s = record_cmd.get("--tcomm-prefix");
    std::set<std::string> tcomms_prefix;
    split_foreach(tcomm_prefix_list_s, ',', [&tcomms_prefix](auto const &s) {
      tcomms_prefix.insert(std::string(s));
    });

    match_config.tcomm_prefix = std::move(tcomms_prefix);
  }

  if (!record_cmd.is_used("-e")) {
    ERROR("at least 1 metric should be specified\n");
    return 1;
  }

  std::vector<std::string> metric_names =
      record_cmd.get<std::vector<std::string>>("-e");

  for (auto const &metric_name : metric_names) {
    std::vector<std::string> words;
    split_foreach(metric_name, ':', [&words](auto const &it) {
      words.push_back(std::string(it));
    });

    if (words.size() != 2) {
      ERROR("invalid metric specifier: %s\n", metric_name.c_str());
      Plugin::dump_available_metrics(true);
      return 1;
    }

    auto const &class_name = words[0];
    auto const &stat_name = words[1];

    if (class_name == "system") {
      if (Plugin::system_metrics.count(stat_name) == 0) {
        ERROR("cannot find metric: %s\n", metric_name.c_str());
        Plugin::dump_available_metrics(true);
        return 1;
      }

      auto &plugin_data = Plugin::system_metrics[stat_name];
      if (!plugin_data.ready) {
        ERROR("metric %s not ready: %s\n", stat_name.c_str(),
              plugin_data.reason.c_str());
        return 1;
      }

      opts.system_collect_funcs.push_back(
          Plugin::system_metrics[stat_name].callback);
    } else if (class_name == "process") {
      if (Plugin::process_metrics.count(stat_name) == 0) {
        ERROR("cannot find metric: %s\n", metric_name.c_str());
        Plugin::dump_available_metrics(true);
        return 1;
      }

      auto &plugin_data = Plugin::process_metrics[stat_name];
      if (!plugin_data.ready) {
        ERROR("metric %s not ready: %s\n", stat_name.c_str(),
              plugin_data.reason.c_str());
        return 1;
      }

      opts.instance_config.enable_process = true;
      opts.process_collect_funcs.push_back(
          Plugin::process_metrics[stat_name].callback);
    } else if (class_name == "thread") {
      if (Plugin::thread_metrics.count(stat_name) == 0) {
        ERROR("cannot find metric: %s\n", metric_name.c_str());
        Plugin::dump_available_metrics(true);
        return 1;
      }

      auto &plugin_data = Plugin::thread_metrics[stat_name];
      if (!plugin_data.ready) {
        ERROR("metric %s not ready: %s\n", stat_name.c_str(),
              plugin_data.reason.c_str());
        return 1;
      }

      opts.instance_config.enable_process = true;
      opts.instance_config.enable_thread = true;
      opts.thread_collect_funcs.push_back(
          Plugin::thread_metrics[stat_name].callback);
    } else {
      ERROR("invalid metric class: %s\n", class_name.c_str());
      Plugin::dump_available_metrics(true);
      return 1;
    }
  }

  Recorder recorder(opts);
  recorder.run();

  return 0;
}

int handle_summarize_cmd(argparse::ArgumentParser const &summarize_cmd) {
  if (!summarize_cmd.is_used("files")) {
    ERROR("at least 1 file should be specified\n");
    return 1;
  }

  std::vector<std::string> files =
      summarize_cmd.get<std::vector<std::string>>("files");

  Summarize summarize(files);
  summarize.run();
  summarize.report();
  return 0;
}

void setup_clean_up() {
  atexit([]() { std::fflush(nullptr); });

  signal(SIGINT, [](int signum) {
    env.shutdown.store(true, std::memory_order_relaxed);
  });

  signal(SIGTERM, [](int signum) {
    env.shutdown.store(true, std::memory_order_relaxed);
  });
}

void setup_env() { gethostname(env.hostname, sizeof(env.hostname)); }

void enable_debug() { env.log_level = LogLevel::DEBUG; }

int main(int argc, char **argv) {
  setup_env();
  setup_clean_up();
  argparse::ArgumentParser program("ezcollect");
  argparse::ArgumentParser record_cmd("record");
  argparse::ArgumentParser summarize_cmd("summarize");

  program.add_subparser(record_cmd);
  program.add_subparser(summarize_cmd);

  program.add_argument("-g").help("debug").flag();
  record_cmd.add_argument("-g").help("debug").flag();
  summarize_cmd.add_argument("-g").help("debug").flag();

  record_cmd.add_argument("-l", "--list")
      .help("list available metrics")
      .flag()
      .default_value(false);
  record_cmd.add_argument("-i")
      .help("tick interval(ms)")
      .default_value("1000")
      .nargs(1);

  record_cmd.add_argument("-p", "--pid").help("filter by pid");
  record_cmd.add_argument("--pcomm").help("filter by process comm");
  record_cmd.add_argument("--pcomm-prefix")
      .help("filter by process comm prefix");
  record_cmd.add_argument("-t", "--tid").help("filter by tid");
  record_cmd.add_argument("--tcomm").help("filter by thread comm");
  record_cmd.add_argument("--tcomm-prefix")
      .help("filter by thread comm prefix");
  // record_cmd.add_argument("metrics").remaining();
  record_cmd.add_argument("-e")
      .help("metrics")
      .nargs(argparse::nargs_pattern::at_least_one);

  summarize_cmd.add_argument("files").remaining();

  try {
    program.parse_args(argc, argv);
  } catch (const std::exception &err) {
    std::cerr << err.what() << std::endl;
    std::cerr << program;
    std::cerr << "\n";
    Plugin::dump_available_metrics(true);
    return 1;
  }

  if (program.is_used("-g") || record_cmd.is_used("-g") ||
      summarize_cmd.is_used("-g"))
    enable_debug();

  if (program.is_subcommand_used(record_cmd)) {
    return handle_record_cmd(record_cmd);
  } else if (program.is_subcommand_used(summarize_cmd)) {
    return handle_summarize_cmd(summarize_cmd);
  } else {
    ERROR("no subcommand is used\n");
    std::cerr << program;
    std::cerr << "\n";
    Plugin::dump_available_metrics(true);
    return 1;
  }
}
