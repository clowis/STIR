# - this module looks for Matlab
# Defines:
#  MATLAB_INCLUDE_DIR: include path for mex.h, engine.h
#  MATLAB_LIBRARIES:   required libraries: libmex, etc
#  MATLAB_MEX_LIBRARY: path to libmex.lib
#  MATLAB_MX_LIBRARY:  path to libmx.lib
#  MATLAB_MAT_LIBRARY:  path to libmat.lib # added
#  MATLAB_ENG_LIBRARY: path to libeng.lib
#  MATLAB_ROOT: path to Matlab's root directory

# This file is part of Gerardus
#
# This is a derivative work of file FindMatlab.cmake released with
# CMake v2.8, because the original seems to be a bit outdated and
# doesn't work with my Windows XP and Visual Studio 10 install
#
# (Note that the original file does work for Ubuntu Natty)
#
# Author: Ramon Casero <rcasero@...>, Tom Doel
# Version: 0.2.3
#
# Modified by Kris Thielemans for WIN32
# The original file was copied from an Ubuntu Linux install
# /usr/share/cmake-2.8/Modules/FindMatlab.cmake

#=============================================================================
# Copyright 2005-2009 Kitware, Inc.
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of CMake, substitute the full
#  License text for the above reference.)

# If MATLAB_ROOT was defined in the environment, use it.
if (NOT MATLAB_ROOT AND NOT $ENV{MATLAB_ROOT} STREQUAL "")
  set(MATLAB_ROOT $ENV{MATLAB_ROOT} CACHE PATH "set this if CMake does not find it automatically")
  file(TO_CMAKE_PATH "${MATLAB_ROOT}" MATLAB_ROOT)
endif()

if (NOT MATLAB_ROOT)
    # get path to the Matlab executable
    find_program(MATLAB_EXE_PATH matlab
        PATHS /usr/local/bin)
    if (MATLAB_EXE_PATH)
      message(STATUS "MATLAB executable found: ${MATLAB_EXE_PATH}")
      # remove symbolic links
      get_filename_component(MATLAB_EXE_PATH "${MATLAB_EXE_PATH}" REALPATH )
      # find directory of executable
      get_filename_component(my_MATLAB_ROOT "${MATLAB_EXE_PATH}" PATH )
      # find root dir
      get_filename_component(my_MATLAB_ROOT "${my_MATLAB_ROOT}" PATH )
      # store it in the cache
      set(MATLAB_ROOT "${my_MATLAB_ROOT}" CACHE PATH "Location of MATLAB files")
      message(STATUS "MATLAB_ROOT set to ${MATLAB_ROOT}")
     endif()
else()
     # set MATLAB_ROOT to an empty string but as a cached variable
     # this avoids CMake creating a local variable with the same name
     set(MATLAB_ROOT "" CACHE PATH "Location of MATLAB files")
endif()

if(WIN32)
  # Search for a version of Matlab available, starting from the most modern one to older versions
  foreach(MATVER "8.5" "8.4" "8.3" "8.2" "8.1" "8.0" "7.16" "7.15" "7.14" "7.13" "7.12" "7.11" "7.10" "7.9" "7.8" "7.7" "7.6" "7.5" "7.4")
    if((NOT DEFINED MATLAB_ROOT)
        OR ("${MATLAB_ROOT}" STREQUAL "")
        OR ("${MATLAB_ROOT}" STREQUAL "/registry"))
      get_filename_component(MATLAB_ROOT "[HKEY_LOCAL_MACHINE\\SOFTWARE\\MathWorks\\MATLAB\\${MATVER};MATLABROOT]" ABSOLUTE)
    endif()
  endforeach()
  if("${MATLAB_ROOT}" STREQUAL "/registry")
    set(MATLAB_ROOT "")
  endif()
  if ("${MATLAB_ROOT}" STREQUAL "")
    message(STATUS "MATLAB not found. Set MATLAB_ROOT")
    # TODO should really skip rest of configuration as it will all fail anyway.
  endif()

  # Directory name depending on whether the Windows architecture is 32
  # bit or 64 bit
  # set(CMAKE_SIZEOF_VOID_P 8) # Note: For some wierd reason this variable is undefined in my system...
  if(CMAKE_SIZEOF_VOID_P MATCHES "4")
    set(WINDIR "win32")
  elseif(CMAKE_SIZEOF_VOID_P MATCHES "8")
    set(WINDIR "win64")
  else()
    message(FATAL_ERROR
      "CMAKE_SIZEOF_VOID_P (${CMAKE_SIZEOF_VOID_P}) doesn't indicate a valid platform")
  endif()

  # Folder where the MEX libraries are, depending of the Windows compiler
  if(${CMAKE_GENERATOR} MATCHES "Visual Studio 6")
    set(MATLAB_LIBRARIES_DIR "${MATLAB_ROOT}/extern/lib/${WINDIR}/microsoft/msvc60")
  elseif(${CMAKE_GENERATOR} MATCHES "Visual Studio 7")
    # Assume people are generally using Visual Studio 7.1,
    # if using 7.0 need to link to: ../extern/lib/${WINDIR}/microsoft/msvc70
    set(MATLAB_LIBRARIES_DIR "${MATLAB_ROOT}/extern/lib/${WINDIR}/microsoft/msvc71")
    # set(MATLAB_LIBRARIES_DIR "${MATLAB_ROOT}/extern/lib/${WINDIR}/microsoft/msvc70")
  elseif(${CMAKE_GENERATOR} MATCHES "Borland")
    # Assume people are generally using Borland 5.4,
    # if using 7.0 need to link to: ../extern/lib/${WINDIR}/microsoft/msvc70
    set(MATLAB_LIBRARIES_DIR "${MATLAB_ROOT}/extern/lib/${WINDIR}/microsoft/bcc54")
    # set(MATLAB_LIBRARIES_DIR "${MATLAB_ROOT}/extern/lib/${WINDIR}/microsoft/bcc50")
    # set(MATLAB_LIBRARIES_DIR "${MATLAB_ROOT}/extern/lib/${WINDIR}/microsoft/bcc51")
  elseif(${CMAKE_GENERATOR} MATCHES "Visual Studio*")
    # If the compiler is Visual Studio, but not any of the specific
    # versions above, we try our luck with the microsoft directory
    set(MATLAB_LIBRARIES_DIR "${MATLAB_ROOT}/extern/lib/${WINDIR}/microsoft/")
  else()
    message(FATAL_ERROR "Generator not compatible: ${CMAKE_GENERATOR}")
  endif()

  # Get paths to the Matlab MEX libraries
  find_library(MATLAB_MEX_LIBRARY
    libmex
    ${MATLAB_LIBRARIES_DIR}
    )
  find_library(MATLAB_MX_LIBRARY
    libmx
    ${MATLAB_LIBRARIES_DIR}
    )
  find_library(MATLAB_MAT_LIBRARY
    libmat
    ${MATLAB_LIBRARIES_DIR}
    )
  find_library(MATLAB_ENG_LIBRARY
    libeng
    ${MATLAB_LIBRARIES_DIR}
    )

  # Get path to the include directory
  find_path(MATLAB_INCLUDE_DIR
    "mex.h"
    HINTS  "${MATLAB_ROOT}/extern/include"  "${MATLAB_ROOT}/include"
    )

  find_program( MATLAB_MEX_PATH mex.bat
             HINTS ${MATLAB_ROOT}/bin
             DOC "The mex program path"
            )

  find_program( MATLAB_MEXEXT_PATH mexext.bat
             HINTS ${MATLAB_ROOT}/bin
             DOC "The mexext program path"
            )
else()

  # Check if this is a Mac
  if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

    set(LIBRARY_EXTENSION .dylib)

    # If this is a Mac and the attempts to find MATLAB_ROOT have so far failed,
    # we look in the applications folder
    if((NOT DEFINED MATLAB_ROOT) OR ("${MATLAB_ROOT}" STREQUAL ""))

    # Search for a version of Matlab available, starting from the most modern one to older versions
      foreach(MATVER "2015b" "2015a" "R2014b" "R2014a" "R2013b" "R2013a" "R2012b" "R2012a" "R2011b" "R2011a" "R2010b" "R2010a" "R2009b" "R2009a" "R2008b")
        if((NOT DEFINED MATLAB_ROOT) OR ("${MATLAB_ROOT}" STREQUAL ""))
          if(EXISTS /Applications/MATLAB_${MATVER}.app)
            set(MATLAB_ROOT /Applications/MATLAB_${MATVER}.app)
          endif()
        endif()
      endforeach()
    endif()

  else()
    set(LIBRARY_EXTENSION .so)
  endif()

  # Get path to the MEX libraries
  execute_process(
    #COMMAND find "${MATLAB_ROOT}/extern/lib" -name libmex${LIBRARY_EXTENSION} # Peter
    COMMAND find "${MATLAB_ROOT}/bin" -name libmex${LIBRARY_EXTENSION} # Standard
    COMMAND xargs echo -n
    OUTPUT_VARIABLE MATLAB_MEX_LIBRARY
    )
  execute_process(
    #COMMAND find "${MATLAB_ROOT}/extern/lib" -name libmx${LIBRARY_EXTENSION} # Peter
    COMMAND find "${MATLAB_ROOT}/bin" -name libmx${LIBRARY_EXTENSION} # Standard
    COMMAND xargs echo -n
    OUTPUT_VARIABLE MATLAB_MX_LIBRARY
    )
  execute_process(
    #COMMAND find "${MATLAB_ROOT}/extern/lib" -name libmat${LIBRARY_EXTENSION} # Peter
    COMMAND find "${MATLAB_ROOT}/bin" -name libmat${LIBRARY_EXTENSION} # Standard
    COMMAND xargs echo -n
    OUTPUT_VARIABLE MATLAB_MAT_LIBRARY
    )
  execute_process(
    #COMMAND find "${MATLAB_ROOT}/extern/lib" -name libeng${LIBRARY_EXTENSION} # Peter
    COMMAND find "${MATLAB_ROOT}/bin" -name libeng${LIBRARY_EXTENSION} # Standard
    COMMAND xargs echo -n
    OUTPUT_VARIABLE MATLAB_ENG_LIBRARY
    )

  # Get path to the include directory
  find_path(MATLAB_INCLUDE_DIR
    "mex.h"
    HINTS "${MATLAB_ROOT}/include"  "${MATLAB_ROOT}/extern/include"
    )

  find_program( MATLAB_MEX_PATH mex
             HINTS "${MATLAB_ROOT}/bin"
             DOC "The mex program path"
            )

  find_program( MATLAB_MEXEXT_PATH mexext
             HINTS "${MATLAB_ROOT}/bin"
             DOC "The mexext program path"
            )

endif()

# This is common to UNIX and Win32:

execute_process(
    COMMAND ${MATLAB_MEXEXT_PATH}
    OUTPUT_STRIP_TRAILING_WHITESPACE
    OUTPUT_VARIABLE MATLAB_MEX_EXT
    )

set(MATLAB_LIBRARIES
  ${MATLAB_MEX_LIBRARY}
  ${MATLAB_MX_LIBRARY}
  ${MATLAB_ENG_LIBRARY}
)

######################### alternative to find flags using "mex -v"
#
# mex -v outputs all the settings used for building MEX files, so it
# we can use it to grab the important variables needed
execute_process(COMMAND ${MATLAB_MEX_PATH} -v -n ${PROJECT_SOURCE_DIR}/src/cmake/FindMATLAB_mextest.c
  OUTPUT_VARIABLE mexOut
  ERROR_VARIABLE mexErr)

# parse mex output line by line by turning file into CMake list of lines
string(REGEX REPLACE "\r?\n" ";" _mexOut "${mexOut}")
foreach(line ${_mexOut})  
if (WIN32)
  if("${line}" MATCHES "[\t ]*COMPDEFINES *:")
    string(REGEX REPLACE "[\t ]*COMPDEFINES *: *" "" mexCxxFlags "${line}")
    set(mexCFlags "${mexCxxFlags}")
  elseif("${line}" MATCHES "[\t ]*INCLUDE *:")
    string(REGEX REPLACE "[\t ]*INCLUDE *: *" "" mexIncludeFlags "${line}")
  elseif("${line}" MATCHES "[\t ]*LINKFLAGS *:")
    string(REGEX REPLACE "[\t ]*LINKFLAGS *: *" "" mexLdFlags "${line}")
  elseif("${line}" MATCHES "[\t ]*LINKLIBS *:")
    string(REGEX REPLACE "[\t ]*LINKLIBS *: *" "" mexLdLibs "${line}")
  elseif("${line}" MATCHES "[\t ]*LINKEXPORT *:")
    string(REGEX REPLACE "[\t ]*LINKEXPORT *: *" "" mexLdExport "${line}")
    # get rid of /implib statement (refers to temp file)
    string(REGEX REPLACE "/implib:\".*\"" "" mexLdFlags "${mexLdFlags}")
  endif()
else (WIN32)
  if("${line}" MATCHES " CXXFLAGS *=")
    string(REGEX REPLACE " *CXXFLAGS *= *" "" mexCxxFlags "${line}")
  elseif("${line}" MATCHES " CFLAGS *=")
    string(REGEX REPLACE " *CFLAGS *= *" "" mexCFlags "${line}")
  elseif("${line}" MATCHES " FFLAGS *=")
    string(REGEX REPLACE " *FFLAGS *= *" "" mexFFlags "${line}")
  elseif("${line}" MATCHES " CXXLIBS *=")
    string(REGEX REPLACE " *CXXLIBS *= *" "" mexCxxLibs "${line}")
  elseif("${line}" MATCHES " CLIBS *=")
    string(REGEX REPLACE " *CLIBS *= *" "" mexCLibs "${line}")
  elseif("${line}" MATCHES " FLIBS *=")
    string(REGEX REPLACE " *FLIBS *= *" "" mexFLibs "${line}")
  elseif("${line}" MATCHES " LDFLAGS *=")
    string(REGEX REPLACE " *LDFLAGS *= *" "" mexLdFlags "${line}")
  endif()
endif(WIN32)
endforeach()

set(MATLAB_CXXFLAGS "${mexCxxFlags}" CACHE PATH "Flags to compile C++ MATLAB Mex files (or libraries that link with them)")
set(MATLAB_CFLAGS "${mexCFlags}" CACHE PATH "Flags to compile C MATLAB Mex files (or libraries that link with them)")
set(MATLAB_FFLAGS "${mexFFlags}" CACHE PATH "Flags to compile Fortran MATLAB Mex files (or libraries that link with them)")
if (WIN32)
  # note: cannot use mexLdLibs for "target_link_libraries" as that gets confused 
  # by the flag that specifies where the matlab libraries are. So, instead we add
  # it to the linker flags
  set(MATLAB_CLINK_FLAGS "${mexLdFlags} ${mexLdLibs} ${mexLdExport}" CACHE PATH "Flags to link MATLAB Mex files")
  set(MATLAB_CXXLINK_FLAGS "${MATLAB_CLINK_FLAGS}" CACHE PATH "Flags to link MATLAB C++ Mex files")
  set(MATLAB_FLINK_FLAGS "${MATLAB_CLINK_FLAGS}" CACHE PATH "Flags to link MATLAB Fortran Mex files")
else()
  # note: cannot use mexCLibs for "target_link_libraries" as that gets confused 
  # by the flag that specifies where the matlab libraries are. So, instead we add
  # it to the linker flags
  set(MATLAB_CLINK_FLAGS "${mexLdFlags} ${mexCLibs} " CACHE PATH "Flags to link MATLAB Mex files")
  set(MATLAB_CXXLINK_FLAGS "${mexLdFlags} ${mexCxxLibs} " CACHE PATH "Flags to link MATLAB C++ Mex files")
  set(MATLAB_FLINK_FLAGS "${mexLdFlags} ${mexFLibs} " CACHE PATH "Flags to link MATLAB Fortran Mex files")
endif()

########################

# handle the QUIETLY and REQUIRED arguments and set MATLAB_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(MATLAB "MATLAB not found. If you do have it, set MATLAB_ROOT and reconfigure" 
  MATLAB_ROOT MATLAB_INCLUDE_DIR  MATLAB_LIBRARIES
  MATLAB_MEX_PATH
  MATLAB_MEXEXT_PATH
  MATLAB_MEX_EXT
)

mark_as_advanced(
  MATLAB_EXE_PATH
  MATLAB_LIBRARIES
  MATLAB_MEX_LIBRARY
  MATLAB_MX_LIBRARY
  MATLAB_ENG_LIBRARY
  MATLAB_INCLUDE_DIR
  MATLAB_FOUND
  MATLAB_ROOT
  MATLAB_MAT_LIBRARY
  MATLAB_MEX_PATH
  MATLAB_MEXEXT_PATH
  MATLAB_MEX_EXT
  MATLAB_CFLAGS
  MATLAB_CXXFLAGS
  MATLAB_FFLAGS
  MATLAB_CLINK_FLAGS
  MATLAB_CXXLINK_FLAGS
  MATLAB_FLINK_FLAGS
)