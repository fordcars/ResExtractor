# Copyright 2020 Carl Hewett
#
# This file is part of ResExtractor.
#
# ResExtractor is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# ResExtractor is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with ResExtractor. If not, see <http://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.7.0)
project(ResExtractorProject)

# Definitions
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQURED ON)

# CMAKE_SOURCE_DIR: path to directory of the top-level 'CMakeLists.txt'.
# CMAKE_BINARY_DIR: path to where we ran 'cmake' from.

set(RES_EXTRACTOR_SOURCE_DIR ${CMAKE_SOURCE_DIR})
set(RES_EXTRACTOR_INCLUDE_DIR ${RES_EXTRACTOR_SOURCE_DIR}/include)
set(RES_EXTRACTOR_OUTPUT_LIB_DIR ${CMAKE_BINARY_DIR}/../lib/ResExtractor)
set(RES_EXTRACTOR_OUTPUT_EXE_DIR ${CMAKE_BINARY_DIR}/../bin)

set(RES_EXTRACTOR_SOURCES
	${RES_EXTRACTOR_SOURCE_DIR}/RESX/File.cpp
	${RES_EXTRACTOR_SOURCE_DIR}/RESX/ResourceFork.cpp
)

set(RES_EXTRACTOR_HEADERS
	${RES_EXTRACTOR_INCLUDE_DIR}/ResExtractor.hpp # Public interface
	${RES_EXTRACTOR_INCLUDE_DIR}/RESX/Defs.hpp
	${RES_EXTRACTOR_INCLUDE_DIR}/RESX/File.hpp
	${RES_EXTRACTOR_INCLUDE_DIR}/RESX/ResourceFork.hpp
)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release")
endif()

# Output packaged library to output directory
# Must be set BEFORE calling add_library()!!
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${RES_EXTRACTOR_OUTPUT_LIB_DIR})

# Output executable to output directory
# Must be set BEFORE calling add_executable()!!
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${RES_EXTRACTOR_OUTPUT_EXE_DIR})

# Create library
message(STATUS "Creating library target...")
add_library(
	ResExtractor STATIC # Static library
	${RES_EXTRACTOR_SOURCES}
	${RES_EXTRACTOR_HEADERS}
)

# Add library includes
message(STATUS "Adding library includes...")
target_include_directories(
	ResExtractor
	PRIVATE ${RES_EXTRACTOR_INCLUDE_DIR}
)

# Create cmdline executable
add_executable(
	ResExtractorCmdLine
    
    main.cpp
    main.hpp
)

# Add executable includes
message(STATUS "Adding executable includes...")
target_include_directories(
	ResExtractorCmdLine
	PRIVATE ${RES_EXTRACTOR_INCLUDE_DIR}
)

# Link cmdline executable with our STATIC library
target_link_libraries(
    ResExtractorCmdLine
    ResExtractor
)

# Copy include directory to output directory for ease of use
add_custom_command(TARGET ResExtractor POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory ${RES_EXTRACTOR_INCLUDE_DIR} ${RES_EXTRACTOR_OUTPUT_LIB_DIR}/include
)

# Fix Microsoft.
# Unlike other compilers where we can check
# the CMAKE_CXX_COMPILER_ID string, Microsoft Visual C++
# has its own constant. MSVC is set to true if we are using
# a version of Microsoft Visual C++.
if(MSVC)
	set(CMAKE_CXX_FLAGS_DEBUG
		"${CMAKE_CXX_FLAGS_DEBUG} /wd4100 /wd4127 /wd4189 /wd4512 /wd4702")
		
	# Repairs conflicts with Windows libraries, or tries to.
	# If this doesn't work, building as release will work.
	set(CMAKE_EXE_LINKER_FLAGS /NODEFAULTLIB:\"LIBCMT, MSVCRT, MSVPRTD\")

	set(CMAKE_FIND_LIBRARY_PREFIXES "")
	set(CMAKE_FIND_LIBRARY_SUFFIXES ".lib" ".dll")
endif()

# Fix GNU
if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
	# Repeated calls for the same target append the items
	# in the order called.
	target_link_libraries(
		ResExtractor

		# To remove missing DSO symbol error, it has
		# to be here (annoyingly).
		# This is actually a library called libdl, which
		# is the Dynamic Linking library.
		PRIVATE -ldl
	)
endif()
