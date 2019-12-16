project(vtkSlicer${MODULE_NAME}ModuleMRML)

set(KIT ${PROJECT_NAME})

set(${KIT}_EXPORT_DIRECTIVE "VTK_SLICER_${MODULE_NAME_UPPER}_MODULE_MRML_EXPORT")

set(${KIT}_INCLUDE_DIRECTORIES
  ${SlicerRtCommon_INCLUDE_DIRS}
  ${Slicer_Base_INCLUDE_DIRS}
  ${MRMLCore_INCLUDE_DIRS}
  ${vtkSlicerSegmentationsModuleLogic_INCLUDE_DIRS}
  )

# --------------------------------------------------------------------------
# Sources

set(${KIT}_SRCS
  vtkMRMLRTPlanNode.cxx
  vtkMRMLRTPlanNode.h
  vtkMRMLRTBeamNode.cxx
  vtkMRMLRTBeamNode.h
  vtkMRMLRTIonBeamNode.cxx
  vtkMRMLRTIonBeamNode.h
  )

SET (${KIT}_INCLUDE_DIRS
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${vtkSlicerSegmentationsModuleLogic_INCLUDE_DIRS}
  ${Slicer_Base_INCLUDE_DIRS}
  ${MRMLCore_INCLUDE_DIRS}
  CACHE INTERNAL "" FORCE)

# --------------------------------------------------------------------------
# Build the library

set(${KIT}_TARGET_LIBRARIES
  vtkSlicerRtCommon
  vtkSegmentationCore
  vtkSlicerSegmentationsModuleMRML
  vtkSlicerSegmentationsModuleLogic
  MRMLCore
  vtkSlicerMarkupsModuleMRML
  )

SlicerMacroBuildModuleMRML(
  NAME ${KIT}
  EXPORT_DIRECTIVE ${${KIT}_EXPORT_DIRECTIVE}
  INCLUDE_DIRECTORIES ${${KIT}_INCLUDE_DIRECTORIES}
  SRCS ${${KIT}_SRCS}
  TARGET_LIBRARIES ${${KIT}_TARGET_LIBRARIES}
  )

# Set linker flags, needed for OpenMP
if (NOT ${PLASTIMATCH_LDFLAGS} STREQUAL "")
  set_target_properties (${KIT}
    PROPERTIES LINK_FLAGS ${PLASTIMATCH_LDFLAGS})
endif ()
