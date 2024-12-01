#ifndef EZCOLLECT_UTIL_H
#define EZCOLLECT_UTIL_H

#include <functional>
#include <string_view>

static void
split_foreach(std::string_view const &s, char delimiter,
              std::function<void(std::string_view const &)> const &callback) {
  std::size_t last = 0;
  std::size_t i = 0;
  for (; i < s.size(); i++) {
    if (s[i] == delimiter) {
      if (i == 0 || s[i - 1] != delimiter)
        callback({s.begin() + last, i - last});

      last = i + 1;
    }
  }

  callback({s.begin() + last, i - last});
}

static std::vector<std::string_view> split_as_view(std::string_view const &s,
                                                   char delimiter) {
  std::vector<std::string_view> res;
  split_foreach(s, delimiter, [&res](auto v) { res.push_back(v); });
  return res;
}

#endif
