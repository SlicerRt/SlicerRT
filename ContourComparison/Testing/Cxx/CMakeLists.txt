set(KIT qSlicer${MODULE_NAME}Module)

set(KIT_TEST_SRCS
  vtkSlicerContourComparisonModuleLogicTest1.cxx
  )

slicerMacroConfigureModuleCxxTestDriver(
  NAME ${KIT}
  SOURCES ${KIT_TEST_SRCS}
  TARGET_LIBRARIES vtkSlicerContourComparisonModuleLogic
  WITH_VTK_DEBUG_LEAKS_CHECK
  )

#-----------------------------------------------------------------------------
set(TEMP "${CMAKE_BINARY_DIR}/Testing/Temporary")

macro(TEST_WITH_DATA TestName TestExecutableName
      DataDirectoryPath InputLabelmapReference InputLabelmapCompare
      TemporarySceneFile ApplySimpleTransformToInputCompare
      HausdorffMaximumMm HausdorffAverageMm Hausdorff95PercentMm
      DiceCoefficient TruePositivesPercent TrueNegativesPercent
      FalsePositivesPercent FalseNegativesPercent)
  add_test(
    NAME ${TestName}
    COMMAND ${Slicer_LAUNCH_COMMAND} $<TARGET_FILE:${KIT}CxxTests> ${TestExecutableName} ${ARGN}
    -DataDirectoryPath ${DataDirectoryPath}
    -InputLabelmapReference ${InputLabelmapReference}
    -InputLabelmapCompare ${InputLabelmapCompare}
    -TemporarySceneFile ${TemporarySceneFile}
    -ApplySimpleTransformToInputCompare ${ApplySimpleTransformToInputCompare}
    -HausdorffMaximumMm ${HausdorffMaximumMm}
    -HausdorffAverageMm ${HausdorffAverageMm}
    -Hausdorff95PercentMm ${Hausdorff95PercentMm}
    -DiceCoefficient ${DiceCoefficient}
    -TruePositivesPercent ${TruePositivesPercent}
    -TrueNegativesPercent ${TrueNegativesPercent}
    -FalsePositivesPercent ${FalsePositivesPercent}
    -FalseNegativesPercent ${FalseNegativesPercent}
  )
endmacro()

#-----------------------------------------------------------------------------
TEST_WITH_DATA(
  vtkSlicerContourComparisonModuleLogicTest_EclipseProstate
  vtkSlicerContourComparisonModuleLogicTest1
  ${CMAKE_CURRENT_SOURCE_DIR}/../../Data/EclipseProstate
  Rectum_Contour_Labelmap.nrrd
  Rectum_Dilated_Contour_Labelmap.nrrd
  ${TEMP}/TestScene_ContourComparison_EclipseProstate.mrml
  0
  4.67707
  3.12857
  4.22989
  0.651661
  0.182964
  99.6214
  0.195603
  0.0
)
set_tests_properties(vtkSlicerContourComparisonModuleLogicTest_EclipseProstate PROPERTIES FAIL_REGULAR_EXPRESSION "Error;ERROR;Warning;WARNING" )

#-----------------------------------------------------------------------------
TEST_WITH_DATA(
  vtkSlicerContourComparisonModuleLogicTest_EclipseProstate_SameInput
  vtkSlicerContourComparisonModuleLogicTest1
  ${CMAKE_CURRENT_SOURCE_DIR}/../../Data/EclipseProstate
  Rectum_Contour_Labelmap.nrrd
  Rectum_Contour_Labelmap.nrrd
  ${TEMP}/TestScene_ContourComparison_EclipseProstate_SameInput.mrml
  0
  0.0
  0.0
  0.0
  1
  0.182964
  99.817
  0.0
  0.0
)
set_tests_properties(vtkSlicerContourComparisonModuleLogicTest_EclipseProstate PROPERTIES FAIL_REGULAR_EXPRESSION "Error;ERROR;Warning;WARNING" )

#-----------------------------------------------------------------------------
TEST_WITH_DATA(
  vtkSlicerContourComparisonModuleLogicTest_EclipseProstate_Transformed
  vtkSlicerContourComparisonModuleLogicTest1
  ${CMAKE_CURRENT_SOURCE_DIR}/../../Data/EclipseProstate
  Rectum_Contour_Labelmap.nrrd
  Rectum_Contour_Labelmap.nrrd
  ${TEMP}/TestScene_ContourComparison_EclipseProstate_Transformed.mrml
  1
  5.00
  2.14767
  4.83854
  0.693587
  0.126901
  99.761
  0.0560625
  0.0560625
)
set_tests_properties(vtkSlicerContourComparisonModuleLogicTest_EclipseProstate PROPERTIES FAIL_REGULAR_EXPRESSION "Error;ERROR;Warning;WARNING" )
