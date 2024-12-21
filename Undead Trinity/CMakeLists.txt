cmake_minimum_required(VERSION 3.28 FATAL_ERROR)
project(undead_trinity DESCRIPTION "Undead Trinity" VERSION 0.1.0 LANGUAGES CXX)

configure_file(res/version.h.in ${CMAKE_CURRENT_BINARY_DIR}/src/version.h LF)

find_package(CommonLibSSE CONFIG REQUIRED)
add_commonlibsse_plugin(undead_trinity SOURCES
  src/game.hpp
  src/game.cpp
  src/trinity.hpp
  src/trinity.cpp
  src/main.cpp)

target_compile_features(undead_trinity PRIVATE cxx_std_23)
target_include_directories(undead_trinity PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/src)
target_include_directories(undead_trinity PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_precompile_headers(undead_trinity PRIVATE src/main.hpp)

find_package(boost_container REQUIRED CONFIG)
target_link_libraries(undead_trinity PRIVATE Boost::container)

add_custom_command(TARGET undead_trinity POST_BUILD COMMAND
  ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:undead_trinity>
  ${CMAKE_SOURCE_DIR}/../SKSE/Plugins/$<TARGET_FILE_NAME:undead_trinity>)
