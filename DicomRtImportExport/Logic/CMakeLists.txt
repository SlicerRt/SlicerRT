project(vtkSlicer${MODULE_NAME}ModuleLogic)

set(KIT ${PROJECT_NAME})

set(${KIT}_EXPORT_DIRECTIVE "VTK_SLICER_${MODULE_NAME_UPPER}_LOGIC_EXPORT")

set(${KIT}_INCLUDE_DIRECTORIES
  ${SlicerRtCommon_INCLUDE_DIRS}
  ${vtkSlicerSubjectHierarchyModuleLogic_INCLUDE_DIRS}
  ${vtkSlicerSegmentationsModuleMRML_INCLUDE_DIRS}
  ${vtkSlicerSegmentationsModuleLogic_INCLUDE_DIRS}
  ${vtkSlicerSequencesModuleMRML_INCLUDE_DIRS}
  ${vtkSlicerSequencesModuleLogic_INCLUDE_DIRS}
  ${vtkSlicerDICOMLibModuleLogic_INCLUDE_DIRS}
  ${vtkSlicerBeamsModuleLogic_INCLUDE_DIRS}
  ${PlmCommon_INCLUDE_DIRS}
  ${PLASTIMATCH_INCLUDE_DIRS}
  )

set(${KIT}_SRCS
  vtkSlicerDicomRtImportExportModuleLogic.cxx
  vtkSlicerDicomRtImportExportModuleLogic.h
  vtkSlicerDicomRtReader.cxx
  vtkSlicerDicomRtReader.h
  vtkSlicerDicomRtWriter.cxx
  vtkSlicerDicomRtWriter.h
  )

set(${KIT}_TARGET_LIBRARIES
  vtkSlicerRtCommon
  vtkSlicer${MODULE_NAME}ConversionRules
  vtkSlicerSubjectHierarchyModuleLogic
  vtkSlicerSegmentationsModuleMRML
  vtkSlicerSegmentationsModuleLogic
  vtkSlicerSequencesModuleMRML
  vtkSlicerSequencesModuleLogic
  vtkSlicerIsodoseModuleLogic
  ${ITK_LIBRARIES}
  ${VTK_LIBRARIES}
  ${DCMTK_LIBRARIES}
  vtkSlicerVolumesModuleLogic
  vtkSlicerBeamsModuleLogic
  vtkSlicerPlanarImageModuleLogic
  vtkSlicerMarkupsModuleMRML
  vtkSlicerDICOMLibModuleLogic
  vtkPlmCommon
  ${PLASTIMATCH_LIBRARIES}
  )

SET (${KIT}_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${vtkSlicerDicomRtImportExportConversionRules_INCLUDE_DIRS} CACHE INTERNAL "" FORCE)

#-----------------------------------------------------------------------------
SlicerMacroBuildModuleLogic(
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
