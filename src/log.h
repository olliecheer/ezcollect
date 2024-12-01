#ifndef EZCOLLECT_LOG_H
#define EZCOLLECT_LOG_H

#include "src/env.h"
#include <cstdio>

enum LogLevel {
  DEBUG,
  INFO,
  WARN,
  ERROR,
  FATAL,
};

#define DEBUG(...)                                                             \
  if (env.log_level <= LogLevel::DEBUG)                                        \
  fprintf(stderr, "[DEBUG] " __VA_ARGS__)

#define DEBUG_FUNC() DEBUG("[%s]\n", __FUNCTION__)

#define INFO(...)                                                              \
  if (env.log_level <= LogLevel::INFO)                                         \
  fprintf(stderr, "[INFO] " __VA_ARGS__)

#define WARN(...)                                                              \
  if (env.log_level <= LogLevel::WARN)                                         \
  fprintf(stderr, "[WARN] " __VA_ARGS__)

#define ERROR(...)                                                             \
  if (env.log_level <= LogLevel::ERROR)                                        \
  fprintf(stderr, "[ERROR] " __VA_ARGS__)

#define FATAL(...)                                                             \
  if (env.log_level <= LogLevel::FATAL)                                        \
  fprintf(stderr, "[FATAL] " __VA_ARGS__)

#endif
