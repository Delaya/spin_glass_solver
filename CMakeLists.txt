cmake_minimum_required(VERSION 2.8)
project(spin_glass_solver)

# Setup cmake policies.
foreach(p
    CMP0012
    CMP0013
    CMP0014
    CMP0022 # CMake 2.8.12
    CMP0025 # CMake 3.0
    CMP0043 # CMake 3.0
    CMP0053 # CMake 3.1
  )
  if(POLICY ${p})
    cmake_policy(SET ${p} NEW)
  endif()
endforeach()

#--------------------------------------------------
# Setup Output directories
#--------------------------------------------------
SET (CMAKE_RUNTIME_OUTPUT_DIRECTORY
    ${PROJECT_BINARY_DIR}/bin CACHE PATH "Single Directory for all Executables."
)
SET (CMAKE_LIBRARY_OUTPUT_DIRECTORY
    ${PROJECT_BINARY_DIR}/bin CACHE PATH "Single Directory for all Libraries"
)
SET (CMAKE_ARCHIVE_OUTPUT_DIRECTORY
    ${PROJECT_BINARY_DIR}/bin CACHE PATH "Single Directory for all static libraries."
)

#--------------------------------------------------
# CMake module path
#------------------------------------------------------------------------------
#list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/add_definition.cmake)

#------------------------------------------------------------------------------
# Detect which machine we are running on to setup defaults for convenience
#------------------------------------------------------------------------------
SITE_NAME(hostname)
if(hostname MATCHES "unknown" OR hostname MATCHES "No")
  message("unknown hostname \"${hostname}\", using ENV$ $ENV{HOSTNAME} instead")
  set(hostname $ENV{HOSTNAME})
endif()

if(hostname MATCHES bbpbg1)
  set(BGQ 1)
  message("Running on BGQ")
elseif(hostname MATCHES bbpbg2)
  set(BGAS 1)
  message("Running on BGAS")
elseif(hostname MATCHES bbpviz1)
  set(BGVIZ 1)
  message("Running on BBP-viz")
elseif(hostname MATCHES monch)
  set(MONCH 1)
  message("Running on monch")
endif()
message("CMAKE_SYSTEM is ${CMAKE_SYSTEM} with hostname ${hostname} and processor ${CMAKE_SYSTEM_PROCESSOR}" )

#--------------------------------------------------
# Flags
#--------------------------------------------------
# needed for all compilers
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
#  set(CMAKE_BUILD_TYPE RELEASE)
#  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall -pedantic ")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
#  set(CMAKE_BUILD_TYPE RELEASE)
#  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall -pedantic ")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
endif()

#--------------------------------------------------
# Find Boost (should be found by HPX already)
#--------------------------------------------------
find_package(Boost 1.54.0
    COMPONENTS 
        program_options thread system date_time chrono serialization atomic filesystem log log_setup  
    REQUIRED
)
if (NOT Boost_USE_STATIC_LIBS)
  add_config_define(BOOST_LOG_DYN_LINK)
  add_config_define(BOOST_ALL_DYN_LINK)
  # BOOST_PROGRAM_OPTIONS_DYN_LINK
endif()


#--------------------------------------------------
# Find HPX
#--------------------------------------------------
find_package(HPX REQUIRED)
#set(CMAKE_CXX_FLAGS ${HPX_CXX_FLAGS})
include_directories(${HPX_INCLUDE_DIRS})

#--------------------------------------------------
# Use our subset of KWSys
#--------------------------------------------------
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/kwsys)

#--------------------------------------------------
# Our multi-thread wrappers 
#--------------------------------------------------
include_directories(src)
include_directories(async)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

#--------------------------------------------------
# compile definitions 
#--------------------------------------------------
# use these for default input filenames and output locations
add_definitions(-DSPINSOLVE_SOURCE_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}\")
add_definitions(-DSPINSOLVE_BINARY_DIR=\"${CMAKE_CURRENT_BINARY_DIR}\")
add_definitions(-DKWSYS_NAMESPACE=kwsys)
if (WIN32)
  hpx_add_compile_flag(-bigobj)
  add_definitions(-DKWSYS_ENCODING_DEFAULT_CODEPAGE=CP_UTF8)
endif()
if (BGAS)
  add_definitions(-DSPINSOLVE_SCRIPT_NAME=\"add_nodes-bgas.sh\")
elseif (MONCH)
  add_definitions(-DSPINSOLVE_SCRIPT_NAME=\"add_nodes-monch.sh\")
else ()
  add_definitions(-DSPINSOLVE_SCRIPT_NAME=\"undefined\")
endif()
#--------------------------------------------------
# Library
#--------------------------------------------------
if (WIN32)
  set(kwsys_process src/kwsys/ProcessWin32.c)
else()
  set(kwsys_process src/kwsys/ProcessUNIX.c)
endif()

add_library(solver STATIC 
  src/result.cpp 
  src/hamiltonian.cpp 
  src/sa_solver.cpp
  src/CommandCapture.cpp
  src/kwsys/System.c
  ${kwsys_process}
)

#--------------------------------------------------
# Exe
#--------------------------------------------------
add_executable(spinsolve src/main.cpp)
hpx_setup_target(spinsolve)
target_link_libraries(spinsolve 
    solver 
    iostreams_component 
    ${Boost_LIBRARIES}
)
target_compile_definitions(spinsolve PUBLIC -DAPP_BINARY_NAME=\"$<TARGET_FILE:spinsolve>\")

#--------------------------------------------------
# Exe, reference code that runs on single local thread
#--------------------------------------------------
add_definitions(-DHPX_APPLICATION_EXPORTS)
add_executable(solver_reference src/single_thread.cpp)
hpx_setup_target(solver_reference)
target_link_libraries(solver_reference 
    solver
    iostreams_component 
    ${Boost_LIBRARIES}
)

################################################################################
# Configure the header that includes all compile definitions
################################################################################
get_property(PROJECT_CONFIG_DEFINITIONS_VAR GLOBAL PROPERTY PROJECT_CONFIG_DEFINITIONS)

list(SORT PROJECT_CONFIG_DEFINITIONS_VAR)
list(REMOVE_DUPLICATES PROJECT_CONFIG_DEFINITIONS_VAR)

set(project_config_defines "\n")
foreach(def ${PROJECT_CONFIG_DEFINITIONS_VAR})
  set(project_config_defines "${project_config_defines}#define ${def} ${${def}_define}\n")#"
endforeach()

configure_file("${PROJECT_SOURCE_DIR}/cmake/config_defines.hpp.in"
               "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_defines.h"
               @ONLY)

#--------------------------------------------------
# Generate benchmarking scripts
#--------------------------------------------------
hpx_option(${PROJECT_NAME}_BENCHMARK_SCRIPTS_PATH PATH
  "Directory to place batch scripts in"
  "${spin_glass_solver_BINARY_DIR}/scripts"
  ADVANCED)

include(generate_jobs)

#--------------------------------------------------
# Install
#--------------------------------------------------
install(TARGETS spinsolve solver
        RUNTIME DESTINATION bin
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        )
SET(CMAKE_INSTALL_PREFIX ${CMAKE_HOME_DIRECTORY})

