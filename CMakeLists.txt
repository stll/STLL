cmake_minimum_required(VERSION 2.8)
project(STLL)
enable_testing()

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

add_library(stll SHARED
  layouter.cpp
  layouterCSS.cpp
  layouterFont.cpp
  layouterSDL.cpp
  layouterXHTML.cpp
  layouterXMLSaveLoad.cpp
  utf-8.cpp
)
target_compile_options(stll PRIVATE -std=c++11)
if(SDL_FOUND)
  target_compile_definitions(stll PRIVATE HAVE_SDL=1)
endif()
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

add_executable(runtests runtest.cpp)
target_compile_options(runtests PRIVATE -std=c++11)
target_link_libraries(runtests PRIVATE stll ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
add_test(test1 runtests)
