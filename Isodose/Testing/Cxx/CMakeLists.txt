set(KIT qSlicer${MODULE_NAME}Module)

set(KIT_TEST_SRCS
  vtkSlicerIsodoseModuleLogicTest1.cxx
  )

slicerMacroConfigureModuleCxxTestDriver(
  NAME ${KIT}
  SOURCES ${KIT_TEST_SRCS}
  TARGET_LIBRARIES vtkSlicerIsodoseModuleLogic
  WITH_VTK_DEBUG_LEAKS_CHECK
  )

#-----------------------------------------------------------------------------
set(TEMP "${CMAKE_BINARY_DIR}/Testing/Temporary")

macro(TEST_WITH_DATA TestName TestExecutableName
      TestSceneFile TemporarySceneFile
      BaselineIsodoseSurfaceFile VolumeDifferenceToleranceCc)
  add_test(
    NAME ${TestName}
    COMMAND ${Slicer_LAUNCH_COMMAND} $<TARGET_FILE:${KIT}CxxTests> ${TestExecutableName} ${ARGN}
    -TestSceneFile ${TestSceneFile}
    -TemporarySceneFile ${TemporarySceneFile}
    -BaselineIsodoseSurfaceFile ${BaselineIsodoseSurfaceFile}
    -VolumeDifferenceToleranceCc ${VolumeDifferenceToleranceCc}
  )
endmacro()

#-----------------------------------------------------------------------------
TEST_WITH_DATA(
  vtkSlicerIsodoseModuleLogicTest_EclipseProstate
  vtkSlicerIsodoseModuleLogicTest1
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../Testing/Data/Scenes/EclipseProstate_Isodose_Scene.mrml
  ${TEMP}/TestScene_Isodose_EclipseProstate.mrml
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../Testing/Data/EclipseProstate_IsodoseBaseline.vtk
  1.0
)
set_tests_properties(vtkSlicerIsodoseModuleLogicTest_EclipseProstate PROPERTIES FAIL_REGULAR_EXPRESSION "Error;ERROR;Warning;WARNING" )
