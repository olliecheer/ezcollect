#include "src/summarize.h"
#include "src/log.h"
#include "src/metrics.h"
#include "src/utils.h"
#include <algorithm>
#include <charconv>
#include <fstream>
#include <numeric>
#include <string_view>

void Summarize::parse_one_file(std::filesystem::path const &pathname) {
  std::ifstream ifs(pathname);
  std::string line;

  while (std::getline(ifs, line)) {
    auto const &words = split_as_view(line, ' ');
    auto const &ts_ns_s = words[0];
    uint64_t ts_ns;
    std::from_chars(ts_ns_s.begin(), ts_ns_s.end(), ts_ns);

    auto const &metric = words[1];
    auto metric_words = split_as_view(metric, ':');

    auto const &classname = metric_words[0];
    auto const &pluginname = metric_words[1];
    auto const &statname = metric_words[2];

    auto const &stat_type = words[2];

    std::string instance_name;
    double value;

    if (classname == "system") {
      auto const &hostname = words[3];
      auto const &value_s = words[4];
      std::from_chars(value_s.begin(), value_s.end(), value);

      instance_name = hostname;
    } else if (classname == "process") {
      auto const &pid_s = words[3];
      // pid_t pid;
      // std::from_chars(pid_s.begin(), pid_s.end(), pid);
      auto const &pcomm = words[4];
      auto const &value_s = words[5];
      std::from_chars(value_s.begin(), value_s.end(), value);

      instance_name += pid_s;
      instance_name += " ";
      instance_name += pcomm;
    } else if (classname == "thread") {
      auto const &pid_s = words[3];
      // pid_t pid;
      // std::from_chars(pid_s.begin(), pid_s.end(), pid);
      auto const &pcomm = words[4];

      auto const &tid_s = words[5];
      // pid_t tid;
      // std::from_chars(tid_s.begin(), tid_s.end(), tid);
      auto const &tcomm = words[6];

      auto const &value_s = words[7];
      std::from_chars(value_s.begin(), value_s.end(), value);

      instance_name += pid_s;
      instance_name += " ";
      instance_name += pcomm;
      instance_name += " ";
      instance_name += tid_s;
      instance_name += " ";
      instance_name += tcomm;
    }

    auto &data = raw_data[std::string(metric)][instance_name];
    data.series.emplace_back(ts_ns, value);
    data.stat_type =
        Metrics::StatType::name2type.find(std::string(stat_type))->second;
  }
}

double get_average(std::vector<double> const &nums) {
  return std::accumulate(nums.begin(), nums.end(), 0.0f) / (double)nums.size();
}

Summarize::ComputeResult Summarize::get_quantiles(std::vector<double> values) {
  std::size_t p0_1_idx, p1_idx, p10_idx, p25_idx, p50_idx, p75_idx, p90_idx,
      p99_idx, p999_idx, min_idx, max_idx;

  std::size_t nr = values.size();

  if (std::is_sorted(values.begin(), values.end(), std::greater<>{})) {
    max_idx = 0;
    p999_idx = nr * 0.001;
    p99_idx = nr * 0.01;
    p90_idx = nr * 0.1;
    p75_idx = nr * 0.25;
    p50_idx = nr * 0.5;
    p25_idx = nr * 0.75;
    p10_idx = nr * 0.90;
    p1_idx = nr * 0.99;
    p0_1_idx = nr * 0.999;
    min_idx = nr - 1;
  } else {
    min_idx = 0;
    p0_1_idx = nr * 0.001;
    p1_idx = nr * 0.01;
    p10_idx = nr * 0.1;
    p25_idx = nr * 0.25;
    p50_idx = nr * 0.5;
    p75_idx = nr * 0.75;
    p90_idx = nr * 0.90;
    p99_idx = nr * 0.99;
    p999_idx = nr * 0.999;
    max_idx = nr - 1;

    if (!std::is_sorted(values.begin(), values.end())) {
      std::sort(values.begin(), values.end());
    }
  }

  return {
      .min = values[min_idx],
      .avg = get_average(values),
      .p0_1 = values[p0_1_idx],
      .p1 = values[p1_idx],
      .p10 = values[p10_idx],
      .p25 = values[p25_idx],
      .p50 = values[p50_idx],
      .p75 = values[p75_idx],
      .p90 = values[p90_idx],
      .p99 = values[p99_idx],
      .p999 = values[p999_idx],
      .max = values[max_idx],
  };
}

Summarize::ComputeResult Summarize::compute_derive(Series const &s) {
  ComputeResult res{};
  if (s.size() < 2)
    return res;

  auto copy = s;

  for (std::size_t i = s.size() - 1; i > 0; i--) {
    double ts_delta = copy[i].timestamp - copy[i - 1].timestamp;
    copy[i].value = (copy[i].value - copy[i - 1].value) / ts_delta;
  }

  std::vector<double> values;
  std::transform(copy.begin() + 1, copy.end(), std::back_inserter(values),
                 [](auto const &it) { return it.value; });

  std::sort(values.begin(), values.end());

  return get_quantiles(values);
}

Summarize::ComputeResult Summarize::compute_counter(Series const &s) {
  return compute_gauge(s);
}

Summarize::ComputeResult Summarize::compute_gauge(Series const &s) {
  ComputeResult res{};
  if (s.size() < 2)
    return res;

  std::vector<double> values;
  std::transform(s.begin(), s.end(), std::back_inserter(values),
                 [](auto const &it) { return it.value; });

  return get_quantiles(std::move(values));
}

Summarize::Summarize(std::vector<std::string> files) {
  for (auto const &it : files) {
    parse_one_file(it);
  }
}

void Summarize::run() {
  DEBUG_FUNC();
  for (auto const &it_metric : raw_data) {
    auto const &metric_name = it_metric.first;
    for (auto &it_instance : it_metric.second) {
      auto const &instance = it_instance.first;
      auto const &series = it_instance.second.series;
      auto &quantiles_result =
          const_cast<ComputeResult &>(it_instance.second.result);

      auto stat_type = it_instance.second.stat_type;

      if (stat_type == Metrics::StatType::COUNTER) {
        DEBUG("[counter] %s %s\n", metric_name.c_str(), instance.c_str());
        quantiles_result = compute_counter(series);
      } else if (stat_type == Metrics::StatType::DERIVE) {
        DEBUG("[derive] %s %s\n", metric_name.c_str(), instance.c_str());
        quantiles_result = compute_derive(series);
        DEBUG("min: %lf, avg: %lf, max: %lf\n", quantiles_result.min,
              quantiles_result.avg, quantiles_result.max);
      } else if (stat_type == Metrics::StatType::GAUGE) {
        DEBUG("[gauge] %s %s\n", metric_name.c_str(), instance.c_str());
        quantiles_result = compute_gauge(series);
      } else {
        ERROR("unknown stat type: %d\n", stat_type);
      }
    }
  }
}

void Summarize::report() {
  auto report_quantiles = [](ComputeResult const &res) {
    fprintf(stdout, "    min : %lf\n", res.min);
    fprintf(stdout, "    avg : %lf\n", res.avg);
    fprintf(stdout, "    p0.1: %lf\n", res.p0_1);
    fprintf(stdout, "    p1  : %lf\n", res.p1);
    fprintf(stdout, "    p10 : %lf\n", res.p10);
    fprintf(stdout, "    p25 : %lf\n", res.p25);
    fprintf(stdout, "    p50 : %lf\n", res.p50);
    fprintf(stdout, "    p75 : %lf\n", res.p75);
    fprintf(stdout, "    p90 : %lf\n", res.p90);
    fprintf(stdout, "    p99 : %lf\n", res.p99);
    fprintf(stdout, "    p999: %lf\n", res.p999);
    fprintf(stdout, "    max : %lf\n", res.max);
  };

  for (auto const &it_metric : raw_data) {
    auto const &metric_name = it_metric.first;
    fprintf(stdout, "%s\n", metric_name.c_str());
    fprintf(stdout, "\n");

    for (auto const &it_instance : it_metric.second) {
      auto const &instance_name = it_instance.first;
      auto const &data_struct = it_instance.second;

      fprintf(stdout, "  %s\n", instance_name.c_str());
      // fprintf(stdout, "\n");
      report_quantiles(data_struct.result);
      fprintf(stdout, "\n");
    }
  }
}
