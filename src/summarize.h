#ifndef EZCOLLECT_SUMMARIZE_H
#define EZCOLLECT_SUMMARIZE_H

#include "src/metrics.h"
#include <filesystem>
#include <unordered_map>
#include <vector>

class Summarize {
  template <typename KeyType, typename ValueType>
  using HashMap = std::unordered_map<KeyType, ValueType>;

  struct TimeValue {
    uint64_t timestamp;
    double value;

    TimeValue(uint64_t _ts, double _val) : timestamp(_ts), value(_val) {}
  };
  using Series = std::vector<TimeValue>;

  using MetricName = std::string;
  using InstanceName = std::string;

  struct ComputeResult {
    double min;
    double avg;
    double p0_1;
    double p1;
    double p10;
    double p25;
    double p50;
    double p75;
    double p90;
    double p99;
    double p999;
    double max;
  };

  struct DataStruct {
    Metrics::StatType::Type stat_type;
    Series series;
    ComputeResult result;
  };

  HashMap<MetricName, HashMap<InstanceName, DataStruct>> raw_data;

  ComputeResult get_quantiles(std::vector<double> values);

  ComputeResult compute_derive(Series const &s);
  ComputeResult compute_counter(Series const &s);
  ComputeResult compute_gauge(Series const &s);

  void parse_one_file(std::filesystem::path const &pathname);

public:
  Summarize(std::vector<std::string> files);
  void run();
  void report();
};

#endif
