cmake_minimum_required (VERSION 2.6)
project (empower-enb-agent2)

# Use C++14
set(CMAKE_CXX_STANDARD 11)


# Generate PIC code so object files can be used both in archive
# libraries and in shared library
set(CMAKE_CXX_FLAGS "-Wall -Wextra -pedantic -fPIC -DPIC")
set(CMAKE_C_FLAGS "-Wall -Wextra -pedantic -fPIC -DPIC")

# For Release builds, use the optimizer
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_C_FLAGS_RELEASE "-O3")

# For Debug builds, include debugging info
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_C_FLAGS_DEBUG "-g")

option(EMPOWER_ENB_AGENT_BUILD_EXAMPLES       "Build also the examples" ON)

# Public include files of our libraries
set(EMPOWER_ENB_AGENT_INCLUDE_DIR  ${PROJECT_SOURCE_DIR}/lib/include)

# Libraries to link for examples
set(EMPOWER_ENB_AGENT_LIBS EmpowerAgentProto)

add_subdirectory(lib)


#
# The examples just illustrate how the libraries can be used
#
if (EMPOWER_ENB_AGENT_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

#
# If Doxygen is available, use it to generate documentation.
#
find_package(Doxygen)
if (DOXYGEN_FOUND)
  set(DOXYGEN_GENERATE_LATEX NO)
  set(DOXYGEN_OUTPUT_DIRECTORY doc)

  # Add a 'doc' target generating the documentation
  doxygen_add_docs(doc
    lib/include
    lib/src
    doc/mainpage.md)
endif()

#
# If clang-format is available, use it to provide a target to reformat C++ sources
#
message("-- Searching for clang-format")
find_program (CLANG_FORMAT_EXECUTABLE
  NAMES "clang-format"
  DOC " path to clang-format executable")

if(NOT CLANG_FORMAT_EXECUTABLE)
  message("-- clang-format not found")
else()
  message("-- Found clang-format as ${CLANG_FORMAT_EXECUTABLE}")
  message("-- Adding custom target 'format'")
  FILE(GLOB_RECURSE CLANG_FORMAT_SOURCE_FILES
    lib/src/*/*.cpp lib/include/*/*.hh
    src/*.cpp src/*.hh
    examples/*.cpp examples/*.hh
    test/*.cpp test/*.hh)

  ADD_CUSTOM_TARGET(format
    COMMAND ${CLANG_FORMAT_EXECUTABLE} -i -style=file ${CLANG_FORMAT_SOURCE_FILES}
    COMMENT "Auto formatting of all C++ source files")
endif()
