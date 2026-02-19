cmake_minimum_required(VERSION 3.16)

project(lyricsd)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDBUS REQUIRED IMPORTED_TARGET sdbus-c++)
pkg_check_modules(CURL REQUIRED IMPORTED_TARGET libcurl)

set(SOURCES 
  src/main.cpp
  src/mpris.cpp
  src/lyrics.cpp
  src/simplejson.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} 
  PkgConfig::SDBUS
  PkgConfig::CURL
)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src)
