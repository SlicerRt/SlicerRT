project(vtkSlicerRtCommon)

# --------------------------------------------------------------------------
# Sources

SET (SlicerRtCommon_SRCS 
  SlicerRtCommon.cxx
  SlicerRtCommon.h
  vtkMRMLContourNode.cxx
  vtkMRMLContourNode.h
  vtkMRMLContourHierarchyNode.cxx
  vtkMRMLContourHierarchyNode.h
  vtkPolyDataToLabelmapFilter.cxx
  vtkPolyDataToLabelmapFilter.h
  )

SET (SlicerRtCommon_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR} ${Slicer_Libs_INCLUDE_DIRS} CACHE INTERNAL "" FORCE)

# --------------------------------------------------------------------------
# Build the library

set(lib_name ${PROJECT_NAME})
SET (SlicerRtCommon_LIBS ${SlicerRtCommon_LIBS} vtkCommon MRMLCore)

INCLUDE_DIRECTORIES( ${SlicerRtCommon_INCLUDE_DIRS} )
ADD_LIBRARY(${lib_name} STATIC ${SlicerRtCommon_SRCS})
TARGET_LINK_LIBRARIES( ${lib_name} ${SlicerRtCommon_LIBS} )

IF (CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" AND NOT WIN32)
  SET_TARGET_PROPERTIES(${lib_name} PROPERTIES COMPILE_FLAGS "-fPIC")
ENDIF()
