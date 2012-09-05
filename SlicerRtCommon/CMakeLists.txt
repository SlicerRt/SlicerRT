# --------------------------------------------------------------------------
# Sources

SET (SlicerRtCommon_SRCS 
  SlicerRtCommon.cxx
  SlicerRtCommon.h
  vtkMRMLContourNode.cxx
  vtkMRMLContourNode.h
  vtkPolyDataToLabelmapFilter.cxx
  vtkPolyDataToLabelmapFilter.h
  )

SET (SlicerRtCommon_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR} ${Slicer_Libs_INCLUDE_DIRS} CACHE INTERNAL "" FORCE)

# --------------------------------------------------------------------------
# Build the library

SET (SlicerRtCommon_LIBS ${SlicerRtCommon_LIBS} vtkCommon MRMLCore)

INCLUDE_DIRECTORIES( ${SlicerRtCommon_INCLUDE_DIRS} )
ADD_LIBRARY(vtkSlicerRtCommon STATIC ${SlicerRtCommon_SRCS})
TARGET_LINK_LIBRARIES( vtkSlicerRtCommon ${SlicerRtCommon_LIBS} )
