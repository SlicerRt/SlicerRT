set(KIT qSlicer${MODULE_NAME}Module)

set(KIT_TEST_SRCS
  vtkSlicerDoseVolumeHistogramModuleLogicTest1.cxx
  )

slicerMacroConfigureModuleCxxTestDriver(
  NAME ${KIT}
  SOURCES ${KIT_TEST_SRCS}
  TARGET_LIBRARIES vtkSlicerDoseVolumeHistogramModuleLogic
  WITH_VTK_DEBUG_LEAKS_CHECK
  )

#-----------------------------------------------------------------------------
set(TEMP "${CMAKE_BINARY_DIR}/Testing/Temporary")

macro(TEST_WITH_DATA TestName TestExecutableName
      TestSceneFile BaselineDvhTableCsvFile BaselineDvhMetricCsvFile
      TemporarySceneFile TemporaryDvhTableCsvFile TemporaryDvhMetricCsvFile
      VolumeDifferenceCriterion DoseToAgreementCriterion AgreementAcceptancePercentageThreshold MetricDifferenceThreshold
      DvhStartValue DvhStepSize RasterizationOversamplingFactor)
  add_test(
    NAME ${TestName}
    COMMAND ${Slicer_LAUNCH_COMMAND} $<TARGET_FILE:${KIT}CxxTests> ${TestExecutableName} ${ARGN}
    -TestSceneFile ${TestSceneFile}
    -BaselineDvhTableCsvFile ${BaselineDvhTableCsvFile}
    -BaselineDvhMetricCsvFile ${BaselineDvhMetricCsvFile}
    -TemporarySceneFile ${TemporarySceneFile}
    -TemporaryDvhTableCsvFile ${TemporaryDvhTableCsvFile}
    -TemporaryDvhMetricCsvFile ${TemporaryDvhMetricCsvFile}
    -VolumeDifferenceCriterion ${VolumeDifferenceCriterion}
    -DoseToAgreementCriterion ${DoseToAgreementCriterion}
    -AgreementAcceptancePercentageThreshold ${AgreementAcceptancePercentageThreshold}
    -MetricDifferenceThreshold ${MetricDifferenceThreshold}
    -DvhStartValue ${DvhStartValue}
    -DvhStepSize ${DvhStepSize}
    -RasterizationOversamplingFactor ${RasterizationOversamplingFactor}
  )
endmacro()

#-----------------------------------------------------------------------------
TEST_WITH_DATA(
  vtkSlicerDoseVolumeHistogramModuleLogicTest_EclipseProstate
  vtkSlicerDoseVolumeHistogramModuleLogicTest1
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../Testing/Data/Scenes/EclipseProstate_Dvh_Scene.mrml
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../Testing/Data/EclipseProstate_DvhTable_SlicerRT.csv
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../Testing/Data/EclipseProstate_DvhMetrics_SlicerRT.csv
  ${TEMP}/TestScene_EclipseProstate.mrml
  ${TEMP}/TestDvhTable_EclipseProstate_SlicerRT.csv
  ${TEMP}/TestDvhMetrics_EclipseProstate_SlicerRT.csv
  0.0
  0.0
  100.0
  0.0
  0.0
  0.0
  2.0
)
set_tests_properties(vtkSlicerDoseVolumeHistogramModuleLogicTest_EclipseProstate PROPERTIES FAIL_REGULAR_EXPRESSION "Error;ERROR;Warning;WARNING" )

#-----------------------------------------------------------------------------
TEST_WITH_DATA(
  vtkSlicerDoseVolumeHistogramModuleLogicTest_EclipseProstate_CERR
  vtkSlicerDoseVolumeHistogramModuleLogicTest1
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../Testing/Data/Scenes/EclipseProstate_Dvh_Scene.mrml
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../Testing/Data/EclipseProstate_DvhTable_CERR.csv
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../Testing/Data/NoMetricComparison
  ${TEMP}/TestScene_EclipseProstate_CERR.mrml
  ${TEMP}/TestDvhTable_EclipseProstate_CERR_SlicerRT.csv
  ${TEMP}/TestDvhMetrics_EclipseProstate_CERR_SlicerRT.csv
  1.0
  1.0
  95.0
  3.0
  0.01
  0.01
  2.0
)
set_tests_properties(vtkSlicerDoseVolumeHistogramModuleLogicTest_EclipseProstate_CERR PROPERTIES FAIL_REGULAR_EXPRESSION "Error;ERROR;Warning;WARNING" )

#-----------------------------------------------------------------------------
TEST_WITH_DATA(
  vtkSlicerDoseVolumeHistogramModuleLogicTest_EclipseEnt_CERR
  vtkSlicerDoseVolumeHistogramModuleLogicTest1
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../Testing/Data/Scenes/EclipseEnt_Dvh_Scene.mrml
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../Testing/Data/EclipseEnt_DvhTable_CERR.csv
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../Testing/Data/NoMetricComparison
  ${TEMP}/TestScene_EclipseEnt_CERR.mrml
  ${TEMP}/TestDvhTable_EclipseEnt_CERR_SlicerRT.csv
  ${TEMP}/TestDvhMetrics_EclipseEnt_CERR_SlicerRT.csv
  1.0
  1.0
  94.0
  3.0
  0.01
  0.01
  2.0
)
set_tests_properties(vtkSlicerDoseVolumeHistogramModuleLogicTest_EclipseEnt_CERR PROPERTIES FAIL_REGULAR_EXPRESSION "Error;ERROR;Warning;WARNING" )

#-----------------------------------------------------------------------------
TEST_WITH_DATA(
  vtkSlicerDoseVolumeHistogramModuleLogicTest_EclipseProstate_Eclipse
  vtkSlicerDoseVolumeHistogramModuleLogicTest1
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../Testing/Data/Scenes/EclipseProstate_Dvh_Scene.mrml
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../Testing/Data/EclipseProstate_DvhTable_Eclipse.csv
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../Testing/Data/NoMetricComparison
  ${TEMP}/TestScene_EclipseProstate_Eclipse.mrml
  ${TEMP}/TestDvhTable_EclipseProstate_Eclipse_SlicerRT.csv
  ${TEMP}/TestDvhMetrics_EclipseProstate_Eclipse_SlicerRT.csv
  1.0
  1.0
  94.0
  3.0
  0.01
  0.01
  2.0
)
set_tests_properties(vtkSlicerDoseVolumeHistogramModuleLogicTest_EclipseProstate_Eclipse PROPERTIES FAIL_REGULAR_EXPRESSION "Error;ERROR;Warning;WARNING" )

#-----------------------------------------------------------------------------
TEST_WITH_DATA(
  vtkSlicerDoseVolumeHistogramModuleLogicTest_EclipseEnt_Eclipse
  vtkSlicerDoseVolumeHistogramModuleLogicTest1
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../Testing/Data/Scenes/EclipseEnt_Dvh_Scene.mrml
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../Testing/Data/EclipseEnt_DvhTable_Eclipse.csv
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../Testing/Data/NoMetricComparison
  ${TEMP}/TestScene_EclipseEnt_Eclipse.mrml
  ${TEMP}/TestDvhTable_EclipseEnt_Eclipse_SlicerRT.csv
  ${TEMP}/TestDvhMetrics_EclipseEnt_Eclipse_SlicerRT.csv
  1.0
  1.0
  92.0
  3.0
  0.01
  0.01
  2.0
)
set_tests_properties(vtkSlicerDoseVolumeHistogramModuleLogicTest_EclipseEnt_Eclipse PROPERTIES FAIL_REGULAR_EXPRESSION "Error;ERROR;Warning;WARNING" )
