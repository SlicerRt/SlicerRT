project(vtkLeakFinder)

# --------------------------------------------------------------------------
# Sources

SET (LeakFinder_SRCS 
  vtkLeakFinder.cxx
  vtkLeakFinder.h
  StackWalker.cxx
  StackWalker.h
  )

SET (LeakFinder_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR} ${Slicer_Libs_INCLUDE_DIRS} CACHE INTERNAL "" FORCE)

# --------------------------------------------------------------------------
# Build the library

set(lib_name ${PROJECT_NAME})
SET (LeakFinder_LIBS ${LeakFinder_LIBS}
	vtkCommon
)

INCLUDE_DIRECTORIES( ${LeakFinder_INCLUDE_DIRS} )
ADD_LIBRARY(${lib_name} STATIC ${LeakFinder_SRCS})
TARGET_LINK_LIBRARIES( ${lib_name} ${LeakFinder_LIBS} )

IF (CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" AND NOT WIN32)
  SET_TARGET_PROPERTIES(${lib_name} PROPERTIES COMPILE_FLAGS "-fPIC")
ENDIF()
