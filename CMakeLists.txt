cmake_minimum_required(VERSION 3.16)
project(windows_display_mode_cli LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_VERBOSE_MAKEFILE ON)

add_executable(displaymode
  src/main.cpp
  src/cli.cpp
  src/windows_display.cpp
  src/display_config.cpp
)
set_target_properties(displaymode PROPERTIES ENABLE_EXPORTS OFF)

# Find paths
execute_process(COMMAND ${CMAKE_CXX_COMPILER} -print-libgcc-file-name
  OUTPUT_VARIABLE _libgcc_file OUTPUT_STRIP_TRAILING_WHITESPACE)
get_filename_component(_libgcc_dir "${_libgcc_file}" DIRECTORY)
get_filename_component(_cc_bin "${CMAKE_CXX_COMPILER}" DIRECTORY)
get_filename_component(_ucrt_root "${_cc_bin}/.." ABSOLUTE)
set(_ucrt_lib "${_ucrt_root}/lib")

# Static runtimes
target_link_options(displaymode PRIVATE -static)
target_link_libraries(displaymode PRIVATE
  -Wl,-Bstatic
  "${_ucrt_lib}/libstdc++.a"
  "${_libgcc_dir}/libgcc.a"
  "${_libgcc_dir}/libgcc_eh.a"
  "${_ucrt_lib}/libwinpthread.a"
  -Wl,-Bdynamic
  kernel32 user32 gdi32 winspool shell32 ole32 oleaut32 uuid comdlg32 advapi32
)

install(TARGETS displaymode RUNTIME DESTINATION .)
