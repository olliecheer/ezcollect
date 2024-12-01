#include <filesystem>
#include <map>
#include <memory>
#include <set>
#include <vector>

std::vector<char> cat_vec(std::filesystem::path const &path);
std::string cat_str(std::filesystem::path const &path);

std::set<pid_t> tids_of_pid(std::string_view pid_s);
std::set<pid_t> tids_of_pid(pid_t pid);

std::string get_pcomm(std::string_view pid_s);
std::string get_pcomm(pid_t pid);

std::string get_tcomm(std::string_view pid_s, std::string_view tid_s);
std::string get_tcomm(pid_t pid, pid_t tid);

std::map<pid_t, std::shared_ptr<std::string>>
pgrep(std::set<std::string> const &pcomms, bool prefix_match);

std::map<pid_t, std::shared_ptr<std::string>>
tgrep(pid_t pid, std::set<std::string> const &tcomms, bool prefix_match);

std::set<pid_t> get_all_pids();
