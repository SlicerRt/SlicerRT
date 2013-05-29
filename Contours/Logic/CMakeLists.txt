project(vtkSlicer${MODULE_NAME}ModuleLogic)

set(KIT ${PROJECT_NAME})

set(${KIT}_EXPORT_DIRECTIVE "VTK_SLICER_${MODULE_NAME_UPPER}_LOGIC_EXPORT")

set(${KIT}_INCLUDE_DIRECTORIES
  ${SlicerRtCommon_INCLUDE_DIRS}
  ${vtkSlicerContoursModuleMRML_INCLUDE_DIRS}
  ${vtkSlicerPatientHierarchyModuleLogic_INCLUDE_DIRS}
  ${vtkSlicerPatientHierarchyModulePlugins_INCLUDE_DIRS}
  )

set(${KIT}_SRCS
  vtkSlicer${MODULE_NAME}ModuleLogic.cxx
  vtkSlicer${MODULE_NAME}ModuleLogic.h
  vtkSlicer${MODULE_NAME}PatientHierarchyPlugin.cxx
  vtkSlicer${MODULE_NAME}PatientHierarchyPlugin.h
  )

set(${KIT}_TARGET_LIBRARIES
  vtkSlicerRtCommon
  vtkSlicerContoursModuleMRML
  vtkSlicerPatientHierarchyModuleLogic
  vtkSlicerPatientHierarchyModulePlugins
  )

#-----------------------------------------------------------------------------
SlicerMacroBuildModuleLogic(
  NAME ${KIT}
  EXPORT_DIRECTIVE ${${KIT}_EXPORT_DIRECTIVE}
  INCLUDE_DIRECTORIES ${${KIT}_INCLUDE_DIRECTORIES}
  SRCS ${${KIT}_SRCS}
  TARGET_LIBRARIES ${${KIT}_TARGET_LIBRARIES}
  )
  


