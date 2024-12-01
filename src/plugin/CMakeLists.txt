add_library(plugin ./plugin.cc)

function(register_metrics libname)
  target_link_libraries(${libname} PRIVATE plugin)
  target_link_libraries(ezcollect -Wl,--whole-archive ${libname} -Wl,--no-whole-archive)
endfunction()

add_library(example ./example.cc)
register_metrics(example)

add_library(taskstats ./taskstats.cc ./nl_task_stats.cc)
target_link_libraries(taskstats PRIVATE cap)
register_metrics(taskstats)

add_library(schedstat ./schedstat.cc)
register_metrics(schedstat)
