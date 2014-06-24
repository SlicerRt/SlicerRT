set(KIT qSlicer${MODULE_NAME}Module)

set(KIT_TEST_SRCS
  vtkSlicerContourMorphologyModuleLogicTest1.cxx
  )

slicerMacroConfigureModuleCxxTestDriver(
  NAME ${KIT}
  SOURCES ${KIT_TEST_SRCS}
  TARGET_LIBRARIES vtkSlicerContourMorphologyModuleLogic
  WITH_VTK_DEBUG_LEAKS_CHECK
  )

#-----------------------------------------------------------------------------
set(TEMP "${CMAKE_BINARY_DIR}/Testing/Temporary")

macro(TEST_WITH_DATA TestName TestExecutableName
      DataDirectoryPath InputContourAFile
      InputContourBFile ReferenceVolumeFile BaselineContourLabelmapFile 
      TemporarySceneFile MorphologicalOperation
      MorphologicalParameter ApplySimpleTransformToInput VolumeDifferenceToleranceVoxel)
  add_test(
    NAME ${TestName}
    COMMAND ${Slicer_LAUNCH_COMMAND} $<TARGET_FILE:${KIT}CxxTests> ${TestExecutableName} ${ARGN}
    -DataDirectoryPath ${DataDirectoryPath}
    -InputContourAFile  ${InputContourAFile}
    -InputContourBFile  ${InputContourBFile}
    -ReferenceVolumeFile  ${ReferenceVolumeFile}
    -BaselineContourLabelmapFile  ${BaselineContourLabelmapFile}
    -TemporarySceneFile ${TemporarySceneFile}
    -MorphologicalOperation ${MorphologicalOperation}
    -MorphologicalParameter ${MorphologicalParameter}
    -ApplySimpleTransformToInput ${ApplySimpleTransformToInput}
    -VolumeDifferenceToleranceVoxel ${VolumeDifferenceToleranceVoxel}
  )
endmacro()

#-----------------------------------------------------------------------------
TEST_WITH_DATA(
  vtkSlicerContourMorphologyModuleLogicTest_EclipseProstate_Expand
  vtkSlicerContourMorphologyModuleLogicTest1
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../Testing/Data/
  EclipseProstate_PTV.ctr
  EclipseProstate_PTV.ctr
  EclipseProstate_Dose.nrrd
  EclipseProstate_ContourMorphology_Expand_Baseline.nrrd
  ${TEMP}/TestScene_ContourMorphology_EclipseProstate.mrml
  Expand
  5.0
  0
  100.0
)
set_tests_properties(vtkSlicerContourMorphologyModuleLogicTest_EclipseProstate_Expand PROPERTIES FAIL_REGULAR_EXPRESSION "Error;ERROR;Warning;WARNING" )

#-----------------------------------------------------------------------------
TEST_WITH_DATA(
  vtkSlicerContourMorphologyModuleLogicTest_EclipseProstate_Shrink
  vtkSlicerContourMorphologyModuleLogicTest1
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../Testing/Data/
  EclipseProstate_PTV.ctr
  EclipseProstate_PTV.ctr
  EclipseProstate_Dose.nrrd
  EclipseProstate_ContourMorphology_Shrink_Baseline.nrrd
  ${TEMP}/TestScene_ContourMorphology_EclipseProstate_Shrink.mrml
  Shrink
  5.0
  0
  100.0
)
set_tests_properties(vtkSlicerContourMorphologyModuleLogicTest_EclipseProstate_Shrink PROPERTIES FAIL_REGULAR_EXPRESSION "Error;ERROR;Warning;WARNING" )

#-----------------------------------------------------------------------------
TEST_WITH_DATA(
  vtkSlicerContourMorphologyModuleLogicTest_EclipseProstate_Union
  vtkSlicerContourMorphologyModuleLogicTest1
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../Testing/Data/
  EclipseProstate_PTV.ctr
  EclipseProstate_Bladder.ctr
  EclipseProstate_Dose.nrrd
  EclipseProstate_ContourMorphology_Union_Baseline.nrrd
  ${TEMP}/TestScene_ContourMorphology_EclipseProstate_Union.mrml
  Union
  0.0
  0
  100.0
)
set_tests_properties(vtkSlicerContourMorphologyModuleLogicTest_EclipseProstate_Union PROPERTIES FAIL_REGULAR_EXPRESSION "Error;ERROR;Warning;WARNING" )

#-----------------------------------------------------------------------------
TEST_WITH_DATA(
  vtkSlicerContourMorphologyModuleLogicTest_EclipseProstate_Intersect
  vtkSlicerContourMorphologyModuleLogicTest1
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../Testing/Data/
  EclipseProstate_PTV.ctr
  EclipseProstate_Bladder.ctr
  EclipseProstate_Dose.nrrd
  EclipseProstate_ContourMorphology_Intersect_Baseline.nrrd
  ${TEMP}/TestScene_ContourMorphology_EclipseProstate_Intersect.mrml
  Intersect
  0.0
  0
  100.0
)
set_tests_properties(vtkSlicerContourMorphologyModuleLogicTest_EclipseProstate_Intersect PROPERTIES FAIL_REGULAR_EXPRESSION "Error;ERROR;Warning;WARNING" )

#-----------------------------------------------------------------------------
TEST_WITH_DATA(
  vtkSlicerContourMorphologyModuleLogicTest_EclipseProstate_Subtract
  vtkSlicerContourMorphologyModuleLogicTest1
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../Testing/Data/
  EclipseProstate_PTV.ctr
  EclipseProstate_Bladder.ctr
  EclipseProstate_Dose.nrrd
  EclipseProstate_ContourMorphology_Subtract_Baseline.nrrd
  ${TEMP}/TestScene_ContourMorphology_EclipseProstate_Subtract.mrml
  Subtract
  0.0
  0
  100.0
)
set_tests_properties(vtkSlicerContourMorphologyModuleLogicTest_EclipseProstate_Subtract PROPERTIES FAIL_REGULAR_EXPRESSION "Error;ERROR;Warning;WARNING" )

#-----------------------------------------------------------------------------
TEST_WITH_DATA(
  vtkSlicerContourMorphologyModuleLogicTest_EclipseProstate_Intersect_ApplyTransform
  vtkSlicerContourMorphologyModuleLogicTest1
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../Testing/Data/
  EclipseProstate_PTV.ctr
  EclipseProstate_Bladder.ctr
  EclipseProstate_Dose.nrrd
  EclipseProstate_ContourMorphology_Intersect_ApplyTransform_Baseline.nrrd
  ${TEMP}/TestScene_ContourMorphology_EclipseProstate_Intersect_ApplyTransform.mrml
  Intersect
  0.0
  1
  100.0
)
set_tests_properties(vtkSlicerContourMorphologyModuleLogicTest_EclipseProstate_Intersect_ApplyTransform PROPERTIES FAIL_REGULAR_EXPRESSION "Error;ERROR;Warning;WARNING" )
