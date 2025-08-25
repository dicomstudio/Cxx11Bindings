cmake_minimum_required(VERSION 3.14)
project(
  Cxx11Bindings
  VERSION 0.0.1
  LANGUAGES CXX C)

# only export limited set of symbols
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

# require c++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# require c11
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# config
set(PLUGIN_NAME Cxx11Bindings)

# installation
include(GNUInstallDirs)
set(${PLUGIN_NAME}_LIBRARY_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}")

# Output directories for libraries
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# sources
set(CXX11_ABI cxx11bindings.h)
set(CXX11_API ${CXX11_ABI} cxx11bindings.hxx cxx11exceptions.hxx)
set_source_files_properties(cxx11bindings.h PROPERTIES LANGUAGE C
                                                       HEADER_FILE_ONLY ON)
set(SOURCES ${CXX11_API} cxx11bindings.c stream_interface_impl.hxx)
# shared obj lib
add_library(${PLUGIN_NAME}_obj OBJECT ${SOURCES})
set_target_properties(${PLUGIN_NAME}_obj PROPERTIES POSITION_INDEPENDENT_CODE
                                                    ON)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_compile_definitions(${PLUGIN_NAME}_obj
                             PRIVATE _GLIBCXX_USE_CXX11_ABI=1)
endif()
if(MSVC)
  target_compile_definitions(${PLUGIN_NAME}_obj PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# shared library
add_library(${PLUGIN_NAME}_shared SHARED $<TARGET_OBJECTS:${PLUGIN_NAME}_obj>)
set_target_properties(
  ${PLUGIN_NAME}_shared
  PROPERTIES OUTPUT_NAME ${PLUGIN_NAME}
             ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
             LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
             RUNTIME_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})

# module library (e.g., plugin for dlopen) ----
add_library(${PLUGIN_NAME}_module MODULE $<TARGET_OBJECTS:${PLUGIN_NAME}_obj>)
set_target_properties(
  ${PLUGIN_NAME}_module
  PROPERTIES OUTPUT_NAME ${PLUGIN_NAME}
             PREFIX "" # Remove 'lib' prefix for MODULE on Unix
             LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
             RUNTIME_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})

install(
  TARGETS ${PLUGIN_NAME}_shared ${PLUGIN_NAME}_module
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib COMPONENT libraries)

install(
  FILES ${CXX11_API}
  DESTINATION include
  COMPONENT headers)

# Enable testing via CTest
include(CTest)
if(BUILD_TESTING)
  enable_testing()

  # verify that code can be compiled in C11-only:
  add_executable(${PROJECT_NAME}_comp test_cfile.c)
  if(MSVC)
    target_compile_definitions(${PROJECT_NAME}_comp
                               PRIVATE _CRT_SECURE_NO_WARNINGS)
  endif()
  # fseeko/ftello support:
  target_compile_definitions(${PROJECT_NAME}_comp PUBLIC _FILE_OFFSET_BITS=64)
  target_compile_definitions(${PROJECT_NAME}_comp
                             PUBLIC _POSIX_C_SOURCE=200112L)
  target_link_libraries(${PROJECT_NAME}_comp ${PLUGIN_NAME}_shared)
  # Define test executable
  find_package(GTest CONFIG REQUIRED)
  include(GoogleTest)
  add_executable(${PROJECT_NAME}_tests test_streambuf.cxx test_cfile.cxx)
  if(MSVC)
    target_compile_definitions(${PROJECT_NAME}_tests
                               PRIVATE _CRT_SECURE_NO_WARNINGS)
  endif()
  target_link_libraries(
    ${PROJECT_NAME}_tests PRIVATE ${PLUGIN_NAME}_shared GTest::gtest_main
                                  GTest::gtest)
  gtest_discover_tests(${PROJECT_NAME}_tests)
endif()
