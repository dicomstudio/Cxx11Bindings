cmake_minimum_required(VERSION 3.14)
project(
  Cxx11Bindings
  VERSION 3.0.0
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
set(LIBRARY_NAME Cxx11Bindings)
option(C11B_BUILD_FILE "Build the FILE implementation" ON)
option(C11B_BUILD_FILEBUF "Build the filebuf implementation" ON)

# installation
include(GNUInstallDirs)
set(${LIBRARY_NAME}_LIBRARY_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}")

# Output directories for libraries
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# documentation
set(DOC_FILES README.md)
source_group("Documentation" FILES ${DOC_FILES})
add_custom_target(docs SOURCES ${DOC_FILES})

# sources
set(CXX11_ABI include/cxx11bindings/stream.h)
set(CXX11_API ${CXX11_ABI} include/cxx11bindings/streambuf.hxx)
set_source_files_properties(${CXX11_ABI} PROPERTIES LANGUAGE C HEADER_FILE_ONLY
                                                               ON)

set(SOURCES ${CXX11_API} src/stream_interface_impl.hxx
            src/stream_interface_impl.h)
set_source_files_properties(src/stream_interface_impl.h
                            PROPERTIES LANGUAGE C HEADER_FILE_ONLY ON)

if(C11B_BUILD_FILE)
  list(APPEND SOURCES src/file.c)
  list(APPEND SOURCES include/cxx11bindings/file.h)
endif()
if(C11B_BUILD_FILEBUF)
  list(APPEND SOURCES src/filebuf.cxx)
  list(APPEND SOURCES include/cxx11bindings/filebuf.hxx)
endif()
set_source_files_properties(src/stream_interface_impl.h
                            PROPERTIES LANGUAGE C HEADER_FILE_ONLY ON)
add_library(${LIBRARY_NAME}_common OBJECT ${SOURCES})
set_target_properties(${LIBRARY_NAME}_common
                      PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(${LIBRARY_NAME}_common
                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
if(C11B_BUILD_FILE OR C11B_BUILD_FILEBUF)
  # add_library(${LIBRARY_NAME} SHARED ${SOURCES})
  add_library(${LIBRARY_NAME} SHARED $<TARGET_OBJECTS:${LIBRARY_NAME}_common>)
  # -fPIC:
  set_target_properties(${LIBRARY_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
  target_include_directories(
    ${LIBRARY_NAME}
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
           $<INSTALL_INTERFACE:include>)
  # 64bits file IO
  target_compile_definitions(${LIBRARY_NAME}_common PUBLIC _FILE_OFFSET_BITS=64)
  target_compile_definitions(${LIBRARY_NAME}_common
                             PUBLIC _POSIX_C_SOURCE=200112L)
  if(MSVC)
    target_compile_definitions(${LIBRARY_NAME}_common
                               PRIVATE _CRT_SECURE_NO_WARNINGS)
  endif()
  set_target_properties(
    ${LIBRARY_NAME} PROPERTIES VERSION ${PROJECT_VERSION}
                               SOVERSION ${PROJECT_VERSION_MAJOR})
endif()

# module library (e.g., plugin for dlopen) ----
add_library(${LIBRARY_NAME}_module MODULE
            $<TARGET_OBJECTS:${LIBRARY_NAME}_common> src/test_cxx11bindings.c)
target_include_directories(${LIBRARY_NAME}_module
                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
set_target_properties(
  ${LIBRARY_NAME}_module
  PROPERTIES OUTPUT_NAME ${LIBRARY_NAME}Tests
             PREFIX "" # Remove 'lib' prefix for MODULE on Unix
             LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
             RUNTIME_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})

# installation
install(
  FILES ${CXX11_API}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/cxx11bindings
  COMPONENT headers)
if(C11B_BUILD_FILE)
  install(
    FILES include/cxx11bindings/file.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/cxx11bindings
    COMPONENT headers)
endif()
if(C11B_BUILD_FILEBUF)
  install(
    FILES include/cxx11bindings/filebuf.hxx
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/cxx11bindings
    COMPONENT headers)
endif()
if(C11B_BUILD_FILE OR C11B_BUILD_FILEBUF)
  install(
    TARGETS ${LIBRARY_NAME}
    EXPORT ${LIBRARY_NAME}Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT libraries)
endif()
include(CMakePackageConfigHelpers)

# Generate the config file
configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Cxx11BindingsConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/Cxx11BindingsConfig.cmake"
  INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Cxx11Bindings")

# Export targets
install(
  EXPORT ${LIBRARY_NAME}Targets
  FILE Cxx11BindingsTargets.cmake
  NAMESPACE Cxx11Bindings::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Cxx11Bindings)

# Install the config file
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/Cxx11BindingsConfig.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Cxx11Bindings)

# Enable testing via CTest
include(CTest)
if(BUILD_TESTING)
  enable_testing()

  # verify that code can be compiled in C11-only:
  if(C11B_BUILD_FILE)
    add_executable(${PROJECT_NAME}_comp src/test_cfile.c)
    target_link_libraries(${LIBRARY_NAME}_comp PRIVATE ${LIBRARY_NAME})
  endif()

  if(C11B_BUILD_FILE AND C11B_BUILD_FILEBUF)
    # Define test executable
    find_package(GTest CONFIG REQUIRED)
    include(GoogleTest)
    add_executable(${PROJECT_NAME}_tests src/test_streambuf.cxx
                                         src/test_cfile.cxx)
    target_link_libraries(${LIBRARY_NAME}_tests PRIVATE ${LIBRARY_NAME})
    if(MSVC)
      target_compile_definitions(${PROJECT_NAME}_tests
                                 PRIVATE _CRT_SECURE_NO_WARNINGS)
    endif()
    target_link_libraries(${PROJECT_NAME}_tests PRIVATE GTest::gtest_main
                                                        GTest::gtest)
    gtest_discover_tests(${PROJECT_NAME}_tests)
  endif()
endif()
