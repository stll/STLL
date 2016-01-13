cmake_minimum_required(VERSION 2.8)
project(STLL)
enable_testing()

# Include build target for coverage
set(CMAKE_CXX_FLAGS_COVERAGE "${CMAKE_CXX_FLAGS_DEBUG} -fprofile-arcs -ftest-coverage")
set(CMAKE_C_FLAGS_COVERAGE "${CMAKE_C_FLAGS_DEBUG} -fprofile-arcs -ftest-coverage")
set(CMAKE_EXE_LINKER_FLAGS_COVERAGE "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -fprofile-arcs -ftest-coverage")
set(CMAKE_SHARED_LINKER_FLAGS_COVERAGE " ${CMAKE_SHARED_LINKER_FLAGS_DEBUG} -fprofile-arcs -ftest-coverage")

# Dependencies with direct support for CMake
find_package(Freetype REQUIRED)
find_package(Boost REQUIRED COMPONENTS unit_test_framework)
find_package(SDL)
find_package(LibXml2)

# Dependencies without support for CMake, but with support for pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(HARFBUZZ REQUIRED harfbuzz)
pkg_check_modules(FRIBIDI REQUIRED fribidi)

# Dependencies without support for neither CMake nor pkg-config
find_library(UNIBREAK_LIBRARY NAMES unibreak linebreak)
find_library(PUGIXML_LIBRARY NAMES pugixml)

if(LIBXML2_FOUND)
  list(APPEND XMLLIBDEFINES -DUSE_LIBXML2)
endif()
if(PUGIXML_LIBRARY)
  list(APPEND XMLLIBDEFINES -DUSE_PUGI_XML)
endif()

# STLL library
set(stll_SOURCES
  src/output/blurr.cpp
  src/layouter.cpp
  src/layouterCSS.cpp
  src/layouterFont.cpp
  src/layouterXHTML.cpp
  src/utf-8.cpp
  src/output/cache_single.cpp
)
if(PUGIXML_LIBRARY)
  list(APPEND stll_SOURCES src/layouterXHTML_Pugi.cpp)
endif()
if(LIBXML2_FOUND)
  list(APPEND stll_SOURCES src/layouterXHTML_LibXML2.cpp)
endif()
if(SDL_FOUND)
  list(APPEND stll_SOURCES src/output/output_SDL.cpp)
endif()
add_library(stll SHARED ${stll_SOURCES})
target_compile_options(stll PRIVATE -std=c++14 ${XMLLIBDEFINES})
target_include_directories(stll PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}
  ${FREETYPE_INCLUDE_DIRS}
  ${FRIBIDI_INCLUDE_DIRS}
  ${HARFBUZZ_INCLUDE_DIRS}
  ${SDL_INCLUDE_DIR}
  ${LIBXML2_INCLUDE_DIR}
  include
  src
)
target_link_libraries(stll PUBLIC
  ${FREETYPE_LIBRARIES}
  ${FRIBIDI_LIBRARIES}
  ${HARFBUZZ_LIBRARIES}
  ${UNIBREAK_LIBRARY}
  ${SDL_LIBRARY}
  ${PUGIXML_LIBRARY}
  ${LIBXML2_LIBRARIES}
)

# Tests
add_executable(runtestsPugi examples/runtests.cpp examples/layouterXMLSaveLoad.cpp)
target_compile_options(runtestsPugi PRIVATE -std=c++14 -DUSE_PUGI_XML)
target_include_directories(runtestsPugi PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}
  include
  )
target_link_libraries(runtestsPugi PRIVATE stll
  ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
)
add_test(test_PugiXML runtestsPugi)

if(LIBXML2_FOUND)
  add_executable(runtestsLibXML2 examples/runtests.cpp examples/layouterXMLSaveLoad.cpp)
  target_compile_options(runtestsLibXML2 PRIVATE -std=c++14 -DUSE_LIBXML2)
  target_include_directories(runtestsLibXML2 PRIVATE
    ${LIBXML2_INCLUDE_DIR}
    include
  )
  target_link_libraries(runtestsLibXML2 PRIVATE stll
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
  )
  add_test(text_LibXML2 runtestsLibXML2)
endif()

# Example programs
if(SDL_FOUND)
  add_executable(example1 examples/example1.cpp)
  target_compile_options(example1 PRIVATE -std=c++14)
  target_include_directories(example1 PRIVATE
    ${SDL_INCLUDE_DIR}
    include
  )
  target_link_libraries(example1 PRIVATE stll
    ${SDL_LIBRARY}
  )

  add_executable(example2 examples/example2.cpp)
  target_compile_options(example2 PRIVATE -std=c++14)
  target_include_directories(example2 PRIVATE
    ${SDL_INCLUDE_DIR}
    include
  )
  target_link_libraries(example2 PRIVATE stll
    ${SDL_LIBRARY}
  )

  add_executable(example3 examples/example3.cpp examples/layouterXMLSaveLoad.cpp)
  target_compile_options(example3 PRIVATE -std=c++14)
  target_include_directories(example3 PRIVATE
    ${SDL_INCLUDE_DIR}
    include
  )
  target_link_libraries(example3 PRIVATE stll
    ${SDL_LIBRARY}
  )
endif()

if(SDL_FOUND)
  add_executable(viewer examples/viewer.cpp examples/layouterXMLSaveLoad.cpp)
  target_compile_options(viewer PRIVATE -std=c++14)
  target_include_directories(viewer PRIVATE
    ${SDL_INCLUDE_DIR}
    include
  )
  target_link_libraries(viewer PRIVATE stll
    ${SDL_LIBRARY}
  )
endif()

# source package
set(CPACK_PACKAGE_VERSION_MAJOR "0")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "0")
set(CPACK_SOURCE_GENERATOR "TBZ2")
set(CPACK_SOURCE_IGNORE_FILES
  "/.git/;/.kdev4/;/tests/*;~$;try2.kdev4;.gitignore;${CPACK_SOURCE_IGNORE_FILES}")
include(CPack)

