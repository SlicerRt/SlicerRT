# --------------------------------------------------------------------------
# Sources

SET (SlicerRtCommon_SRCS 
  SlicerRtCommon.cxx
  )

SET (SlicerRtCommon_HDRS
  SlicerRtCommon.h
  )

SET (SlicerRtCommon_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR} CACHE INTERNAL "" FORCE)

# --------------------------------------------------------------------------
# Build the library

SET (SlicerRtCommon_LIBS ${SlicerRtCommon_LIBS} )

INCLUDE_DIRECTORIES( ${SlicerRtCommon_INCLUDE_DIRS} )
ADD_LIBRARY(vtkSlicerRtCommon ${SlicerRtCommon_SRCS} ${SlicerRtCommon_HDRS})
TARGET_LINK_LIBRARIES( vtkSlicerRtCommon ${SlicerRtCommon_LIBS} )
