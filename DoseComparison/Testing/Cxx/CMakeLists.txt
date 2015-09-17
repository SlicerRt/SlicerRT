set(KIT qSlicer${MODULE_NAME}Module)

set(KIT_TEST_SRCS
  vtkSlicerDoseComparisonModuleLogicTest1.cxx
  )

slicerMacroConfigureModuleCxxTestDriver(
  NAME ${KIT}
  SOURCES ${KIT_TEST_SRCS}
  TARGET_LIBRARIES vtkSlicerDoseComparisonModuleLogic vtkSlicerSubjectHierarchyModuleLogic
  WITH_VTK_DEBUG_LEAKS_CHECK
  )

#-----------------------------------------------------------------------------
set(TEMP "${CMAKE_BINARY_DIR}/Testing/Temporary")

macro(TEST_WITH_DATA TestName TestExecutableName
      TestSceneFile TemporarySceneFile)
  add_test(
    NAME ${TestName}
    COMMAND ${Slicer_LAUNCH_COMMAND} $<TARGET_FILE:${KIT}CxxTests> ${TestExecutableName} ${ARGN}
    -TestSceneFile ${TestSceneFile}
    -TemporarySceneFile ${TemporarySceneFile}
  )
endmacro()

#-----------------------------------------------------------------------------
TEST_WITH_DATA(
  vtkSlicerDoseComparisonModuleLogicTest_EclipseEnt
  vtkSlicerDoseComparisonModuleLogicTest1
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../Testing/Data/Scenes/EclipseEnt_DoseComparison_Scene.mrml
  ${TEMP}/TestScene_DoseComparison_EclipseEnt.mrml
)
set_tests_properties(vtkSlicerDoseComparisonModuleLogicTest_EclipseEnt PROPERTIES FAIL_REGULAR_EXPRESSION "Error;ERROR;Warning;WARNING" )
