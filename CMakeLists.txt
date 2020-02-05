# Copyright (C) 2013 The Regents of the University of California (Regents).
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#
#     * Redistributions in binary form must reproduce the above
#       copyright notice, this list of conditions and the following
#       disclaimer in the documentation and/or other materials provided
#       with the distribution.
#
#     * Neither the name of The Regents or University of California nor the
#       names of its contributors may be used to endorse or promote products
#       derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
# Please contact the author of this library if you have any questions.
# Author: Chris Sweeney (sweeney.chris.m@gmail.com)

cmake_minimum_required(VERSION 2.8.3)
if (COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
endif (COMMAND cmake_policy)

# Disable warnings about rpath on MacOS X for now.
# TODO(cmsweeney): Figure out the proper way to handle this!
if(POLICY CMP0042)
  cmake_policy(SET CMP0042 NEW) # CMake 3.0
endif()

project(THEIA C CXX)
include_directories(${PROJECT_SOURCE_DIR})

# Set up the git hook to make Gerrit Change-Id: lines in commit messages.
set (LOCAL_GIT_DIRECTORY)
if (EXISTS ${CMAKE_SOURCE_DIR}/.git)
  # .git directory can be found on Unix based system, or on Windows with
  # Git Bash (shipped with msysgit)
  set (LOCAL_GIT_DIRECTORY ${CMAKE_SOURCE_DIR}/.git)
else (EXISTS ${CMAKE_SOURCE_DIR}/.git)
endif (EXISTS ${CMAKE_SOURCE_DIR}/.git)

if (EXISTS ${LOCAL_GIT_DIRECTORY})
  if (NOT EXISTS ${LOCAL_GIT_DIRECTORY}/hooks/commit-msg)
    # Download the hook only if it is not already present. We borrow the hook
    # hosted by Ceres because it is not specific to the project.
    file(DOWNLOAD http://www.theia-sfm.org/_static/commit-msg
         ${CMAKE_BINARY_DIR}/commit-msg)

    # Make the downloaded file executable, since it is not by default.
    file(COPY ${CMAKE_BINARY_DIR}/commit-msg
         DESTINATION ${LOCAL_GIT_DIRECTORY}/hooks/
         FILE_PERMISSIONS
           OWNER_READ OWNER_WRITE OWNER_EXECUTE
           GROUP_READ GROUP_WRITE GROUP_EXECUTE
           WORLD_READ WORLD_EXECUTE)
  endif (NOT EXISTS ${LOCAL_GIT_DIRECTORY}/hooks/commit-msg)
endif (EXISTS ${LOCAL_GIT_DIRECTORY})

# ==============================================================================
# Additional cmake find modules
# ==============================================================================
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

option(BUILD_TESTING "Enable testing" ON)
option(BUILD_DOCUMENTATION "Build html User's Guide" OFF)

enable_testing()
if (NOT MSVC)
  add_definitions(-DGTEST_USE_OWN_TR1_TUPLE=1)
endif (NOT MSVC)
set(GTEST_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/libraries/gtest")
add_definitions(-DGTEST_TESTING_OUTPUT_DIRECTORY="${CMAKE_BINARY_DIR}/testing_output")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

set(CMAKE_RELEASE_POSTFIX "")
set(CMAKE_DEBUG_POSTFIX "-debug")

set(THEIA_VERSION_MAJOR 0)
set(THEIA_VERSION_MINOR 7)
set(THEIA_VERSION_PATCH 0)
set(THEIA_VERSION
    ${THEIA_VERSION_MAJOR}.${THEIA_VERSION_MINOR}.${THEIA_VERSION_PATCH})
set(THEIA_ABI_VERSION 0.7.0)

# THEIA data directory
add_definitions(-DTHEIA_DATA_DIR="${CMAKE_SOURCE_DIR}/data")

# Eigen
# set(MIN_EIGEN_VERSION 3.2.0)
# find_package(Eigen ${MIN_EIGEN_VERSION} REQUIRED)
# if (EIGEN_FOUND)
#   message("-- Found Eigen version ${EIGEN_VERSION}: ${EIGEN_INCLUDE_DIRS}")
# endif (EIGEN_FOUND)

set(EIGEN_INCLUDE_DIRS "include/eigen3")
include_directories(${EIGEN_INCLUDE_DIRS})

# Use a larger inlining threshold for Clang, since it hobbles Eigen,
# resulting in an unreasonably slow version of the blas routines. The
# -Qunused-arguments is needed because CMake passes the inline
# threshold to the linker and clang complains about it and dies.
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -Qunused-arguments -mllvm -inline-threshold=600")
  # Older versions of Clang (<= 2.9) do not support the 'return-type-c-linkage'
  # option, so check for its presence before adding it to the default flags set.
  include(CheckCXXCompilerFlag)
  check_cxx_compiler_flag("-Wno-return-type-c-linkage"
    HAVE_RETURN_TYPE_C_LINKAGE)
  if (HAVE_RETURN_TYPE_C_LINKAGE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-return-type-c-linkage")
  endif(HAVE_RETURN_TYPE_C_LINKAGE)
endif ()

# GFlags. The namespace patch is borrow from Ceres Solver (see license in
# FindGflags.cmake)
find_package(Gflags REQUIRED)
if (GFLAGS_FOUND)
  message("-- Found Google Flags: ${GFLAGS_INCLUDE_DIRS} in namespace: ${GFLAGS_NAMESPACE}")
  add_definitions(-DTHEIA_GFLAGS_NAMESPACE=${GFLAGS_NAMESPACE})
else (GFLAGS_FOUND)
  message(FATAL_ERROR "Can't find Google FLAGS.")
endif (GFLAGS_FOUND)

# Google Logging
message("-- Check for Google Log")
find_package(Glog REQUIRED)
if (GLOG_FOUND)
  message("-- Found Google Logging: ${GLOG_INCLUDE_DIRS}")
else (GLOG_FOUND)
  message(FATAL_ERROR "Can't find Google Logging. Please set GLOG_INCLUDE_DIR & "
    "GLOG_LIBRARY")
endif (GLOG_FOUND)

# Ceres
# message("-- Check for Ceres")
# find_package(Ceres REQUIRED SuiteSparse)
# if (CERES_FOUND)
#   message("-- Found Ceres: ${CERES_INCLUDE_DIRS}")
# else (CERES_FOUND)
#   message(FATAL_ERROR "Can't find Ceres. Please set CERES_INCLUDE_DIR & "
#     "CERES_LIBRARY")
# endif (CERES_FOUND)

# OpenImageIO
# message("-- Check for OpenImageIO")
# find_package(OpenImageIO REQUIRED)
# if (OPENIMAGEIO_FOUND)
#   message("-- Found OpenImageIO: ${OPENIMAGEIO_INCLUDE_DIRS}")
# else (OPENIMAGEIO_FOUND)
#   message(FATAL_ERROR "Can't find OpenImageIO. Please set OPENIMAGEIO_INCLUDE_DIR & "
#     "OPENIMAGEIO_LIBRARY")
# endif (OPENIMAGEIO_FOUND)

# Suitesparse
# find_package(SuiteSparse REQUIRED)
# if (SUITESPARSE_FOUND)
#   # On Ubuntu the system install of SuiteSparse (v3.4.0) up to at least
#   # Ubuntu 13.10 cannot be used to link shared libraries.
#   if (BUILD_SHARED_LIBS AND
#       SUITESPARSE_IS_BROKEN_SHARED_LINKING_UBUNTU_SYSTEM_VERSION)
#     message(FATAL_ERROR "You are attempting to build Theia as a shared "
#       "library on Ubuntu using a system package install of SuiteSparse "
#       "3.4.0. This package is broken and does not support the "
#       "construction of shared libraries (you can still build Theia as "
#       "a static library).  If you wish to build a shared version of Theia "
#       "you should uninstall the system install of SuiteSparse "
#       "(libsuitesparse-dev) and perform a source install of SuiteSparse "
#       "(we recommend that you use the latest version), "
#       "see http://theia-solver.org/building.html for more information.")
#   endif (BUILD_SHARED_LIBS AND
#     SUITESPARSE_IS_BROKEN_SHARED_LINKING_UBUNTU_SYSTEM_VERSION)
#   message("-- Found SuiteSparse ${SUITESPARSE_VERSION}")
#   add_definitions(-DTHEIA_SUITESPARSE_VERSION="${SUITESPARSE_VERSION}")
# else (SUITESPARSE_FOUND)
#   # Disable use of SuiteSparse if it cannot be found and continue.
#   message(FATAL ERROR "Can't find SuiteSparse. This library is required "
#     "for bundle adjustment and for solving convex optimization problems. "
#     "Please set SUITESPARSE_INCLUDE_DIR & SUITESPARSE_LIBRARY")
# endif (SUITESPARSE_FOUND)

include_directories(
  include
  src
  # ${CERES_INCLUDE_DIRS}
  ${GFLAGS_INCLUDE_DIRS}
  ${GLOG_INCLUDE_DIRS}
  # ${SUITESPARSE_INCLUDE_DIRS}
  # ${OPENIMAGEIO_INCLUDE_DIRS}
)

# NOTE: This fix came from Ceres solver with the following comment:
#
# Eigen SparseQR generates various compiler warnings related to unused and
# uninitialised local variables, which prevents Ceres compilation as we use
# -Werror.  To avoid having to individually suppress these warnings around
# the #include statments for Eigen headers across all GCC/Clang versions, we
# tell CMake to treat Eigen headers as system headers.  This results in all
# compiler warnings from them being suppressed.
#
# Note that this is *not* propagated to clients, ie CERES_INCLUDE_DIRS
# used by clients after find_package(Ceres) does not identify Eigen as
# as system headers.
# include_directories(SYSTEM ${EIGEN_INCLUDE_DIRS})

# build Third party libraries included in distro.
add_subdirectory(libraries)
include_directories(
#   libraries
#   libraries/spectra
#   ${akaze_SOURCE_DIR}
#   ${akaze_INCLUDE_DIR}
  ${cereal_SOURCE_DIR}
  ${cereal_SOURCE_DIR}/include
#   ${flann_SOURCE_DIR}
#   ${flann_SOURCE_DIR}/src/cpp
#   ${gtest_SOURCE_DIR}/include
#   ${gtest_SOURCE_DIR}
#   ${optimo_SOURCE_DIR}
#   ${statx_SOURCE_DIR}
#   ${stlplus3_SOURCE_DIR}
#   ${vlfeat_SOURCE_DIR}
#   ${visual_sfm_SOURCE_DIR}
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(OptimizeTheiaCompilerFlags)
optimizetheiacompilerflags()

add_subdirectory(src/theia)
# add_subdirectory(applications)

if (BUILD_DOCUMENTATION)
  message("-- Documentation building is enabled")

  # Generate the User's Guide (html).
  # The corresponding target is UserGuide, but is included in ALL.
  add_subdirectory(docs)
endif (BUILD_DOCUMENTATION)

# ---------------- Install commands ----------------- #
