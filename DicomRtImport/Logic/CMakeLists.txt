project(vtkSlicer${MODULE_NAME}ModuleLogic)

set(KIT ${PROJECT_NAME})

set(${KIT}_EXPORT_DIRECTIVE "VTK_SLICER_${MODULE_NAME_UPPER}_LOGIC_EXPORT")

set(${KIT}_INCLUDE_DIRECTORIES
  ${QT_QTSQL_INCLUDE_DIR}
  ${SlicerRtCommon_INCLUDE_DIRS}
  ${vtkSlicerContoursModuleMRML_INCLUDE_DIRS}
  ${vtkSlicerPatientHierarchyModuleLogic_INCLUDE_DIRS}
  ${vtkSlicerBeamsModuleLogic_INCLUDE_DIRS}
  )

set(${KIT}_SRCS
  vtkSlicer${MODULE_NAME}ModuleLogic.cxx
  vtkSlicer${MODULE_NAME}ModuleLogic.h
  vtkDICOMImportInfo.cxx
  vtkDICOMImportInfo.h
  vtkSlicerDicomRtReader.cxx
  vtkSlicerDicomRtReader.h
  vtkSlicerDicomRtReader.txx
  vtkTopologicalHierarchy.cxx
  vtkTopologicalHierarchy.h
  )

# DCMTK's dcmrt module is not part of the default list of modules
# so we need to add it manually
find_library( DCMTK_dcmrt_LIBRARY dcmrt
  PATHS
    ${DCMTK_DIR}/lib
    ${DCMTK_DIR}/lib/Release
    ${DCMTK_DIR}/lib/Debug
  NO_DEFAULT_PATH
)   

set(${KIT}_TARGET_LIBRARIES
  vtkSlicerRtCommon
  vtkSlicerPatientHierarchyModuleLogic
  vtkSlicerContoursModuleMRML
  ${ITK_LIBRARIES}
  ${VTK_LIBRARIES}
  ${DCMTK_LIBRARIES}
  ${DCMTK_dcmrt_LIBRARY}
  vtkSlicerVolumesModuleLogic
  vtkSlicerBeamsModuleLogic
  vtkSlicerAnnotationsModuleMRML
  )

#-----------------------------------------------------------------------------
SlicerMacroBuildModuleLogic(
  NAME ${KIT}
  EXPORT_DIRECTIVE ${${KIT}_EXPORT_DIRECTIVE}
  INCLUDE_DIRECTORIES ${${KIT}_INCLUDE_DIRECTORIES}
  SRCS ${${KIT}_SRCS}
  TARGET_LIBRARIES ${${KIT}_TARGET_LIBRARIES}
  )
  
