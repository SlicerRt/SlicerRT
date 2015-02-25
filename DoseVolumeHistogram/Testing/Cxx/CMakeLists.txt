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
      AutomaticOversamplingCalculation
      VolumeDifferenceCriterion DoseToAgreementCriterion
      AgreementAcceptancePercentageThreshold MetricDifferenceThreshold
      DvhStartValue DvhStepSize)
  add_test(
    NAME ${TestName}
    COMMAND ${Slicer_LAUNCH_COMMAND} $<TARGET_FILE:${KIT}CxxTests> ${TestExecutableName} ${ARGN}
    -TestSceneFile ${TestSceneFile}
    -BaselineDvhTableCsvFile ${BaselineDvhTableCsvFile}
    -BaselineDvhMetricCsvFile ${BaselineDvhMetricCsvFile}
    -TemporarySceneFile ${TemporarySceneFile}
    -TemporaryDvhTableCsvFile ${TemporaryDvhTableCsvFile}
    -TemporaryDvhMetricCsvFile ${TemporaryDvhMetricCsvFile}
    -AutomaticOversamplingCalculation ${AutomaticOversamplingCalculation}
    -VolumeDifferenceCriterion ${VolumeDifferenceCriterion}
    -DoseToAgreementCriterion ${DoseToAgreementCriterion}
    -AgreementAcceptancePercentageThreshold ${AgreementAcceptancePercentageThreshold}
    -MetricDifferenceThreshold ${MetricDifferenceThreshold}
    -DvhStartValue ${DvhStartValue}
    -DvhStepSize ${DvhStepSize}
  )
endmacro()

#-----------------------------------------------------------------------------
TEST_WITH_DATA(
  vtkSlicerDoseVolumeHistogramModuleLogicTest_EclipseProstate_Base
  vtkSlicerDoseVolumeHistogramModuleLogicTest1
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../Testing/Data/Scenes/EclipseProstate_Dvh_Scene.mrml
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../Testing/Data/EclipseProstate_DvhTable_SlicerRT.csv
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../Testing/Data/EclipseProstate_DvhMetrics_SlicerRT.csv
  ${TEMP}/TestScene_EclipseProstate.mrml
  ${TEMP}/TestDvhTable_EclipseProstate_SlicerRT.csv
  ${TEMP}/TestDvhMetrics_EclipseProstate_SlicerRT.csv
  0
  0.0
  0.0
  100.0
  0.0
  0.0
  0.0
)
set_tests_properties(vtkSlicerDoseVolumeHistogramModuleLogicTest_EclipseProstate_Base PROPERTIES FAIL_REGULAR_EXPRESSION "Error;ERROR;Warning;WARNING" )

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
  0
  1.0
  1.0
  97.0
  3.0
  0.01
  0.01
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
  0
  1.0
  1.0
  94.0
  3.0
  0.01
  0.01
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
  0
  1.0
  1.0
  94.0
  3.0
  0.01
  0.01
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
  0
  1.0
  1.0
  91.0
  3.0
  0.01
  0.01
)
set_tests_properties(vtkSlicerDoseVolumeHistogramModuleLogicTest_EclipseEnt_Eclipse PROPERTIES FAIL_REGULAR_EXPRESSION "Error;ERROR;Warning;WARNING" )

#-----------------------------------------------------------------------------
TEST_WITH_DATA(
  vtkSlicerDoseVolumeHistogramModuleLogicTest_EclipseProstate_CERR_AutomaticOversampling
  vtkSlicerDoseVolumeHistogramModuleLogicTest1
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../Testing/Data/Scenes/EclipseProstate_Dvh_Scene.mrml
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../Testing/Data/EclipseProstate_DvhTable_CERR.csv
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../Testing/Data/NoMetricComparison
  ${TEMP}/TestScene_EclipseProstate_CERR_AutomaticOversampling.mrml
  ${TEMP}/TestDvhTable_EclipseProstate_CERR_SlicerRT_AutomaticOversampling.csv
  ${TEMP}/TestDvhMetrics_EclipseProstate_CERR_SlicerRT_AutomaticOversampling.csv
  1
  1.0
  1.0
  99.0
  3.0
  0.01
  0.01
)
set_tests_properties(vtkSlicerDoseVolumeHistogramModuleLogicTest_EclipseProstate_CERR_AutomaticOversampling PROPERTIES FAIL_REGULAR_EXPRESSION "Error;ERROR;Warning;WARNING" )

#-----------------------------------------------------------------------------
TEST_WITH_DATA(
  vtkSlicerDoseVolumeHistogramModuleLogicTest_EclipseEnt_CERR_AutomaticOversampling
  vtkSlicerDoseVolumeHistogramModuleLogicTest1
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../Testing/Data/Scenes/EclipseEnt_Dvh_Scene.mrml
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../Testing/Data/EclipseEnt_DvhTable_CERR.csv
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../Testing/Data/NoMetricComparison
  ${TEMP}/TestScene_EclipseEnt_CERR_AutomaticOversampling.mrml
  ${TEMP}/TestDvhTable_EclipseEnt_CERR_SlicerRT_AutomaticOversampling.csv
  ${TEMP}/TestDvhMetrics_EclipseEnt_CERR_SlicerRT_AutomaticOversampling.csv
  1
  1.0
  1.0
  96.0
  3.0
  0.01
  0.01
)
set_tests_properties(vtkSlicerDoseVolumeHistogramModuleLogicTest_EclipseEnt_CERR_AutomaticOversampling PROPERTIES FAIL_REGULAR_EXPRESSION "Error;ERROR;Warning;WARNING" )


#-----------------------------------------------------------------------------
TEST_WITH_DATA(
  vtkSlicerDoseVolumeHistogramModuleLogicTest_EclipseProstate_Eclipse_AutomaticOversampling
  vtkSlicerDoseVolumeHistogramModuleLogicTest1
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../Testing/Data/Scenes/EclipseProstate_Dvh_Scene.mrml
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../Testing/Data/EclipseProstate_DvhTable_Eclipse.csv
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../Testing/Data/NoMetricComparison
  ${TEMP}/TestScene_EclipseProstate_Eclipse_AutomaticOversampling.mrml
  ${TEMP}/TestDvhTable_EclipseProstate_Eclipse_SlicerRT_AutomaticOversampling.csv
  ${TEMP}/TestDvhMetrics_EclipseProstate_Eclipse_SlicerRT_AutomaticOversampling.csv
  1
  1.0
  1.0
  95.0
  3.0
  0.01
  0.01
)
set_tests_properties(vtkSlicerDoseVolumeHistogramModuleLogicTest_EclipseProstate_Eclipse_AutomaticOversampling PROPERTIES FAIL_REGULAR_EXPRESSION "Error;ERROR;Warning;WARNING" )

#-----------------------------------------------------------------------------
TEST_WITH_DATA(
  vtkSlicerDoseVolumeHistogramModuleLogicTest_EclipseEnt_Eclipse_AutomaticOversampling
  vtkSlicerDoseVolumeHistogramModuleLogicTest1
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../Testing/Data/Scenes/EclipseEnt_Dvh_Scene.mrml
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../Testing/Data/EclipseEnt_DvhTable_Eclipse.csv
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../Testing/Data/NoMetricComparison
  ${TEMP}/TestScene_EclipseEnt_Eclipse_AutomaticOversampling.mrml
  ${TEMP}/TestDvhTable_EclipseEnt_Eclipse_SlicerRT_AutomaticOversampling.csv
  ${TEMP}/TestDvhMetrics_EclipseEnt_Eclipse_SlicerRT_AutomaticOversampling.csv
  1
  1.0
  1.0
  93.0
  3.0
  0.01
  0.01
)
set_tests_properties(vtkSlicerDoseVolumeHistogramModuleLogicTest_EclipseEnt_Eclipse_AutomaticOversampling PROPERTIES FAIL_REGULAR_EXPRESSION "Error;ERROR;Warning;WARNING" )
