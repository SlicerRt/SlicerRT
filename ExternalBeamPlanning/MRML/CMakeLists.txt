project(vtkSlicer${MODULE_NAME}ModuleMRML)

set(KIT ${PROJECT_NAME})

set(${KIT}_EXPORT_DIRECTIVE "VTK_SLICER_${MODULE_NAME_UPPER}_MODULE_MRML_EXPORT")

set(${KIT}_INCLUDE_DIRECTORIES
  ${vtkSlicerContoursModuleMRML_INCLUDE_DIRS}
  ${SlicerRtCommon_INCLUDE_DIRS}
  )

# --------------------------------------------------------------------------
# Sources

set(${KIT}_SRCS
  vtkMRMLRTPlanNode.cxx
  vtkMRMLRTPlanNode.h
  vtkMRMLRTBeamNode.cxx
  vtkMRMLRTBeamNode.h
  vtkMRMLRTPlanHierarchyNode.cxx
  vtkMRMLRTPlanHierarchyNode.h
  )

SET (${KIT}_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR} CACHE INTERNAL "" FORCE)
 
# --------------------------------------------------------------------------
# Build the library

set(${KIT}_TARGET_LIBRARIES
  vtkSlicerRtCommon
  vtkSlicerMarkupsModuleMRML
  vtkSlicerContoursModuleMRML
  MRMLCore
  )

SlicerMacroBuildModuleMRML(
  NAME ${KIT}
  EXPORT_DIRECTIVE ${${KIT}_EXPORT_DIRECTIVE}
  INCLUDE_DIRECTORIES ${${KIT}_INCLUDE_DIRECTORIES}
  SRCS ${${KIT}_SRCS}
  TARGET_LIBRARIES ${${KIT}_TARGET_LIBRARIES}
  )
