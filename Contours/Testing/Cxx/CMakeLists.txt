set(KIT qSlicer${MODULE_NAME}Module)

set(KIT_TEST_SRCS
  vtkSlicerContoursModuleLogicTestConversions.cxx
  )

slicerMacroConfigureModuleCxxTestDriver(
  NAME ${KIT}
  SOURCES ${KIT_TEST_SRCS}
  TARGET_LIBRARIES vtkSlicer${MODULE_NAME}ModuleLogic vtkSlicer${MODULE_NAME}ModuleMRML
  WITH_VTK_DEBUG_LEAKS_CHECK
  )

#-----------------------------------------------------------------------------
set(TEMP "${CMAKE_BINARY_DIR}/Testing/Temporary")

macro(TEST_WITH_DATA TestName TestExecutableName
      TestSceneFile TemporarySceneFile NonZeroVoxelCount
      XMinExtent XMaxExtent YMinExtent
      YMaxExtent ZMinExtent ZMaxExtent)
  add_test(
    NAME ${TestName}
    COMMAND ${Slicer_LAUNCH_COMMAND} $<TARGET_FILE:${KIT}CxxTests> ${TestExecutableName} ${ARGN}
    -TestSceneFile ${TestSceneFile}
    -TemporarySceneFile ${TemporarySceneFile}
    -NonZeroVoxelCount ${NonZeroVoxelCount}
    -XMinExtent ${XMinExtent}
    -XMaxExtent ${XMaxExtent}
    -YMinExtent ${YMinExtent}
    -YMaxExtent ${YMaxExtent}
    -ZMinExtent ${ZMinExtent}
    -ZMaxExtent ${ZMaxExtent}
  )
endmacro()

#-----------------------------------------------------------------------------
TEST_WITH_DATA(
  vtkSlicerContoursModuleLogicTest_EclipseProstate
  vtkSlicerContoursModuleLogicTestConversions
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../Testing/Data/Scenes/EclipseProstate_Contours_Scene.mrml
  ${TEMP}/TestScene_Contours_EclipseProstate.mrml 
  699679
  -1
  135
  -1
  85
  -1
  92
)
set_tests_properties(vtkSlicerContoursModuleLogicTest_EclipseProstate PROPERTIES FAIL_REGULAR_EXPRESSION "Error;ERROR;Warning;WARNING" )