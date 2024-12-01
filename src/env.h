#ifndef EZCOLLECT_ENV_H
#define EZCOLLECT_ENV_H

#include <atomic>

struct Env {
  int log_level;
  std::atomic<bool> shutdown;
  char hostname[16];
};

extern struct Env env;


#endif
