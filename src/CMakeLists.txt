# add_library(procfs ./procfs.cc)
# add_library(record ./record.cc)
# target_link_libraries(record procfs)
# add_library(metrics ./metrics.cc)
# add_library(summarize ./summarize.cc)
#
# add_executable(ezcollect ./ezcollect.cc)
# target_link_libraries(ezcollect procfs record metrics summarize)

add_executable(ezcollect
  ./ezcollect.cc
  ./instance_manager.cc
  ./metrics.cc
  ./procfs.cc
  ./record.cc
  ./summarize.cc
)

add_subdirectory(plugin)

install(TARGETS ezcollect DESTINATION bin)

# add_custom_command(TARGET ezcollect POST_BUILD
#   COMMAND ${CMAKE_SOURCE_DIR}/appimage/build_appimage.sh $<TARGET_FILE:ezcollect> ${CMAKE_BINARY_DIR}/ezcollect.appimage)
