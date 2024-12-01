#include "src/procfs.h"
#include "src/log.h"
#include <algorithm>
#include <fstream>
#include <iterator>

namespace fs = std::filesystem;

std::vector<char> cat_vec(std::filesystem::path const &path) {
  std::ifstream ifs(path);
  std::vector<char> res;
  std::copy(std::istream_iterator<char>(ifs), std::istream_iterator<char>{},
            std::back_inserter(res));
  return res;
}

std::string cat_str(std::filesystem::path const &path) {
  // DEBUG("cat_str: %s\n", path.c_str());
  std::ifstream ifs(path);
  std::string res;
  std::copy(std::istreambuf_iterator<char>(ifs),
            std::istreambuf_iterator<char>{}, std::back_inserter(res));

  if (res.back() == '\n')
    res.pop_back();

  return res;
}

std::set<pid_t> tids_of_pid(std::string_view pid_s) {
  std::set<pid_t> res;
  using namespace std::string_literals;
  std::string dir("/proc/");
  dir += pid_s;
  dir += "/task";
  std::error_code err;

  for (auto entry : fs::directory_iterator(
           dir, fs::directory_options::skip_permission_denied, err)) {
    if (!entry.is_directory())
      continue;

    pid_t tid = std::stoi(entry.path().filename());
    res.insert(tid);
  }

  return res;
}

std::set<pid_t> tids_of_pid(pid_t pid) {
  return tids_of_pid(std::to_string(pid));
}

std::string get_pcomm(std::string_view pid_s) {
  std::string path = "/proc/";
  path += pid_s;
  path += "/comm";

  return cat_str(path);
}
std::string get_pcomm(pid_t pid) { return get_pcomm(std::to_string(pid)); }

std::string get_tcomm(std::string_view pid_s, std::string_view tid_s) {
  std::string path = "/proc/";
  path += pid_s;
  path += "/task/";
  path += tid_s;
  path += "/comm";

  return cat_str(path);
}
std::string get_tcomm(pid_t pid, pid_t tid) {
  return get_tcomm(std::to_string(pid), std::to_string(tid));
}

std::map<pid_t, std::shared_ptr<std::string>>
pgrep(std::set<std::string> const &target_pcomms, bool prefix_match) {
  std::map<pid_t, std::shared_ptr<std::string>> res;
  std::error_code err;

  for (auto entry : fs::directory_iterator(
           "/proc", fs::directory_options::skip_permission_denied, err)) {
    if (!entry.is_directory())
      continue;

    std::string filename = entry.path().filename();

    auto is_digital_s = [](std::string const &s) {
      return s.end() == std::find_if_not(s.begin(), s.end(), [](char c) {
               return std::isdigit(c);
             });
    };

    if (!is_digital_s(filename))
      continue;

    std::string pcomm = cat_str(entry.path().string() + "/comm");

    // DEBUG("%s %s\n", filename.c_str(), pcomm.c_str());

    if (prefix_match) {
      for (auto const &it : target_pcomms) {
        if (pcomm.find(it) == 0) {
          pid_t pid = std::stoi(filename);
          res[pid] = std::make_shared<std::string>(std::move(pcomm));
          break;
        }
      }
    } else {
      for (auto const &it : target_pcomms) {
        if (pcomm == it) {
          pid_t pid = std::stoi(filename);
          res[pid] = std::make_shared<std::string>(std::move(pcomm));
          break;
        }
      }
    }
  }

  return res;
}

std::map<pid_t, std::shared_ptr<std::string>>
tgrep(pid_t pid, std::set<std::string> const &target_tcomms,
      bool prefix_match) {
  namespace fs = std::filesystem;
  std::string dir = "/proc/" + std::to_string(pid) + "/task/";

  std::map<pid_t, std::shared_ptr<std::string>> res;
  std::error_code err;

  for (auto const &it : fs::directory_iterator(
           dir, fs::directory_options::skip_permission_denied, err)) {
    std::string tid_s = it.path().filename();
    std::string tcomm = cat_str(it.path().string() + "/comm");

    if (prefix_match) {
      for (auto const &it : target_tcomms) {
        if (tcomm.find(it) == 0) {
          pid_t tid = std::stoi(tid_s);
          res[tid] = std::make_shared<std::string>(std::move(tcomm));
          break;
        }
      }
    } else {
      for (auto const &it : target_tcomms) {
        if (tcomm == it) {
          pid_t tid = std::stoi(tid_s);
          res[tid] = std::make_shared<std::string>(std::move(tcomm));
          break;
        }
      }
    }
  }

  return res;
}

std::set<pid_t> get_all_pids() {
  std::set<pid_t> res;
  std::error_code err;

  for (auto const &it : fs::directory_iterator(
           "/proc", fs::directory_options::skip_permission_denied, err)) {
    if (!it.is_directory())
      continue;

    std::string s = it.path().filename();

    auto is_digital_s = [](std::string const &s) {
      return s.end() == std::find_if_not(s.begin(), s.end(), [](char c) {
               return std::isdigit(c);
             });
    };

    if (is_digital_s(s))
      res.insert(std::stoi(s));
  }

  return res;
}
