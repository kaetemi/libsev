########################################################################
# CMake project
PROJECT("libsev")

# Minimum required 3.2
CMAKE_MINIMUM_REQUIRED(VERSION 3.2 FATAL_ERROR)
# set (CMAKE_VERBOSE_MAKEFILE ON)

########################################################################

IF (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
	
	########################################################################
	# Policies
	IF (POLICY CMP0020)
		CMAKE_POLICY(SET CMP0020 NEW)
	ENDIF ()
	
	
	########################################################################
	# Ensure that we are not building in our source directories.
	
	SET(Build_Dir_OK "TRUE")
	STRING(REGEX MATCH "^${CMAKE_SOURCE_DIR}" In_Sub_Dir ${CMAKE_BINARY_DIR})
	IF (In_Sub_Dir)
		STRING(REGEX MATCH "^${CMAKE_SOURCE_DIR}/build" In_Build_Dir ${CMAKE_BINARY_DIR})
		IF (NOT In_Build_Dir)
			SET(Build_Dir_OK "FALSE")
		ENDIF ()
	ENDIF ()
	
	IF (NOT Build_Dir_OK)
		MESSAGE(FATAL_ERROR "You must run cmake from a directory that is not in your source tree, or that is in a special subdirectory of the tree whose name begins with 'build'.")
	ENDIF ()
	
	
	########################################################################
	# Set up the basic build environment
	
	SET(CMAKE_CXX_STANDARD 17)
	SET(CMAKE_CXX_STANDARD_REQUIRED ON)
	
	IF (CMAKE_BUILD_TYPE STREQUAL "")
		# CMake defaults to leaving CMAKE_BUILD_TYPE empty. This messes up
		# differentiation between debug and release builds.
		SET(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel." FORCE)
	ENDIF ()
	
	SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
	
	########################################################################
	# Strip binary in release mode
	
	IF (CMAKE_COMPILER_IS_GNUCXX)
		SET(CMAKE_EXE_LINKER_FLAGS_RELEASE "-s")
	ENDIF ()
	
	########################################################################
	
	LINK_DIRECTORIES(
	)
	INCLUDE_DIRECTORIES(
		${CMAKE_SOURCE_DIR}/dependencies/debugbreak
		${CMAKE_SOURCE_DIR}/dependencies/gsl/include
	)
	
	########################################################################
	
	SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
	SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
	SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
	
	########################################################################
	
ENDIF ()

########################################################################

ADD_SUBDIRECTORY(dependencies/fmt)

ADD_SUBDIRECTORY(sev)
ADD_SUBDIRECTORY(sev_static)
ADD_SUBDIRECTORY(sev_lite)

########################################################################
