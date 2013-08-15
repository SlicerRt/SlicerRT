project(qSlicer${MODULE_NAME}ModuleWidgets)

set(KIT ${PROJECT_NAME})

set(${KIT}_EXPORT_DIRECTIVE "Q_SLICER_MODULE_${MODULE_NAME_UPPER}_WIDGETS_EXPORT")

set(${KIT}_INCLUDE_DIRECTORIES
  ${SlicerRtCommon_INCLUDE_DIRS}
  )

set(${KIT}_SRCS
  qMRMLPatientHierarchyTreeView.cxx
  qMRMLPatientHierarchyTreeView.h
  qMRMLScenePatientHierarchyModel.cxx
  qMRMLScenePatientHierarchyModel.h
  qMRMLScenePatientHierarchyModel_p.h
  qMRMLSortFilterPatientHierarchyProxyModel.cxx
  qMRMLSortFilterPatientHierarchyProxyModel.h
  qMRMLPotentialPatientHierarchyListView.cxx
  qMRMLPotentialPatientHierarchyListView.h
  qMRMLScenePotentialPatientHierarchyModel.cxx
  qMRMLScenePotentialPatientHierarchyModel.h
  qMRMLScenePotentialPatientHierarchyModel_p.h
  qMRMLSortFilterPotentialPatientHierarchyProxyModel.cxx
  qMRMLSortFilterPotentialPatientHierarchyProxyModel.h
  )

set(${KIT}_MOC_SRCS
  qMRMLPatientHierarchyTreeView.h
  qMRMLScenePatientHierarchyModel.h
  qMRMLSortFilterPatientHierarchyProxyModel.h
  qMRMLPotentialPatientHierarchyListView.h
  qMRMLScenePotentialPatientHierarchyModel.h
  qMRMLSortFilterPotentialPatientHierarchyProxyModel.h
  )

set(${KIT}_UI_SRCS
  )

set(${KIT}_RESOURCES
  Resources/${KIT}.qrc
  )

set(${KIT}_TARGET_LIBRARIES
  vtkSlicerRtCommon
  vtkSlicerPatientHierarchyModuleLogic
  vtkSlicerPatientHierarchyModulePlugins
  vtkSlicerAnnotationsModuleMRML
  )

#-----------------------------------------------------------------------------
SlicerMacroBuildModuleWidgets(
  NAME ${KIT}
  EXPORT_DIRECTIVE ${${KIT}_EXPORT_DIRECTIVE}
  INCLUDE_DIRECTORIES ${${KIT}_INCLUDE_DIRECTORIES}
  SRCS ${${KIT}_SRCS}
  MOC_SRCS ${${KIT}_MOC_SRCS}
  UI_SRCS ${${KIT}_UI_SRCS}
  TARGET_LIBRARIES ${${KIT}_TARGET_LIBRARIES}
  RESOURCES ${${KIT}_RESOURCES}
  WRAP_PYTHONQT
  )

#-----------------------------------------------------------------------------
if(BUILD_TESTING)
  #add_subdirectory(Testing)
endif()

#-----------------------------------------------------------------------------
add_subdirectory(DesignerPlugins)
