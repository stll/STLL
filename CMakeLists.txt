cmake_minimum_required(VERSION 2.8)
project(STLL)
enable_testing()

#include build target for coverage
SET( CMAKE_CXX_FLAGS_COVERAGE "${CMAKE_CXX_FLAGS_DEBUG} -fprofile-arcs -ftest-coverage" )
SET( CMAKE_C_FLAGS_COVERAGE "${CMAKE_C_FLAGS_DEBUG} -fprofile-arcs -ftest-coverage" )
SET( CMAKE_EXE_LINKER_FLAGS_COVERAGE "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -fprofile-arcs -ftest-coverage" )
SET( CMAKE_SHARED_LINKER_FLAGS_COVERAGE " ${CMAKE_SHARED_LINKER_FLAGS_DEBUG} -fprofile-arcs -ftest-coverage" )

# Dependencies with direct support for CMake
find_package(Freetype REQUIRED)
find_package(Boost REQUIRED COMPONENTS unit_test_framework)
find_package(SDL)

# Dependencies without support for CMake, but with support for pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(HARFBUZZ REQUIRED harfbuzz)
pkg_check_modules(FRIBIDI REQUIRED fribidi)

# Dependencies without support for neither CMake nor pkg-config
find_library(LINEBREAK_LIBRARY NAMES linebreak)
find_library(PUGIXML_LIBRARY NAMES pugixml)

# STLL library
set(stll_SOURCES
  layouter.cpp
  layouterCSS.cpp
  layouterFont.cpp
  layouterXHTML.cpp
  layouterXMLSaveLoad.cpp
  utf-8.cpp
)
if(SDL_FOUND)
  list(APPEND stll_SOURCES layouterSDL.cpp)
endif()
add_library(stll SHARED ${stll_SOURCES})
target_compile_options(stll PRIVATE -std=c++11)
target_include_directories(stll PUBLIC
  ${FREETYPE_INCLUDE_DIRS}
)
target_include_directories(stll PRIVATE
  ${FRIBIDI_INCLUDE_DIRS}
  ${HARFBUZZ_INCLUDE_DIRS}
  ${SDL_INCLUDE_DIR}
)
target_link_libraries(stll PUBLIC
  ${FREETYPE_LIBRARIES}
  ${FRIBIDI_LIBRARIES}
  ${HARFBUZZ_LIBRARIES}
  ${LINEBREAK_LIBRARY}
  ${PUGIXML_LIBRARY}
  ${SDL_LIBRARY}
)

# Tests
add_executable(runtests runtests.cpp)
target_compile_options(runtests PRIVATE -std=c++11)
target_link_libraries(runtests PRIVATE stll ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
add_test(test1 runtests)

# Example programs
if(SDL_FOUND)
  add_executable(example example.cpp)
  target_compile_options(example PRIVATE -std=c++11)
  target_include_directories(example PRIVATE ${SDL_INCLUDE_DIR})
  target_link_libraries(example PRIVATE stll ${SDL_LIBRARY})
endif()

if(SDL_FOUND)
  add_executable(viewer viewer.cpp)
  target_compile_options(viewer PRIVATE -std=c++11)
  target_include_directories(viewer PRIVATE ${SDL_INCLUDE_DIR})
  target_link_libraries(viewer PRIVATE stll ${SDL_LIBRARY})
endif()
