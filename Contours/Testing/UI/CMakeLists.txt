set(KIT qSlicer${MODULE_NAME}ModuleWidgets)

#-----------------------------------------------------------------------------
set(KIT_TEST_SRCS
  qSlicerContoursModuleWidgetTest.cxx
  )

#-----------------------------------------------------------------------------
QT4_GENERATE_MOCS(
  qSlicerContoursModuleWidgetTest.cxx
  )

include_directories(${CMAKE_CURRENT_BINARY_DIR})

#-----------------------------------------------------------------------------
slicerMacroConfigureModuleCxxTestDriver(
  NAME ${KIT}
  SOURCES ${KIT_TEST_SRCS}
  TARGET_LIBRARIES qSlicer${MODULE_NAME}ModuleWidgets
  WITH_VTK_DEBUG_LEAKS_CHECK
  )

#-----------------------------------------------------------------------------
set(TEMP "${CMAKE_BINARY_DIR}/Testing/Temporary")

#-----------------------------------------------------------------------------
simple_test( 
  qSlicerContoursModuleWidgetTest
  -TestSceneFile ${CMAKE_CURRENT_SOURCE_DIR}/../../../Testing/Data/Scenes/EclipseProstate_Contours_Scene.mrml
  -TemporarySceneFile ${TEMP}/TestScene_Contours_EclipseProstate.mrml
)  