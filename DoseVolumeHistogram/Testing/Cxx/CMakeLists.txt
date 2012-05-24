set(KIT qSlicer${MODULE_NAME}Module)

#-----------------------------------------------------------------------------
set(KIT_TEST_SRCS)
set(KIT_TEST_NAMES)
set(KIT_TEST_NAMES_CXX)
SlicerMacroConfigureGenericCxxModuleTests(${MODULE_NAME} KIT_TEST_SRCS KIT_TEST_NAMES KIT_TEST_NAMES_CXX)

#-----------------------------------------------------------------------------
#set(CMAKE_TESTDRIVER_BEFORE_TESTMAIN "DEBUG_LEAKS_ENABLE_EXIT_ERROR();" )
create_test_sourcelist(Tests ${MODULE_NAME}CxxTests.cxx
  ${KIT_TEST_NAMES_CXX}
  # Add source of your tests after this line.
  #EXTRA_INCLUDE vtkMRMLDebugLeaksMacro.h
  vtkSlicerDoseVolumeHistogramLogicTest1.cxx
  )

#-----------------------------------------------------------------------------
list(REMOVE_ITEM Tests ${KIT_TEST_NAMES_CXX})
list(APPEND Tests ${KIT_TEST_SRCS})

#-----------------------------------------------------------------------------
add_executable(${KIT}CxxTests ${Tests})
target_link_libraries(${KIT}CxxTests ${KIT})

#-----------------------------------------------------------------------------
foreach(testname ${KIT_TEST_NAMES})
  SIMPLE_TEST( ${testname} )
endforeach()

#-----------------------------------------------------------------------------
set(TEMP "${CMAKE_BINARY_DIR}/Testing/Temporary")

macro(TEST_WITH_DATA TestName DataDirectoryPath TemporaryDirectoryPath ToleranceMeanPercent ToleranceMaxPercent)
  add_test(
    NAME ${TestName}
    COMMAND ${Slicer_LAUNCH_COMMAND} $<TARGET_FILE:${KIT}CxxTests> ${TestName} ${ARGN}
    -DataDirectoryPath ${DataDirectoryPath}
    -TemporaryDirectoryPath ${TemporaryDirectoryPath}
    -ToleranceMeanPercent ${ToleranceMeanPercent}
    -ToleranceMaxPercent ${ToleranceMaxPercent}
  )
endmacro()

#-----------------------------------------------------------------------------
TEST_WITH_DATA(
  vtkSlicerDoseVolumeHistogramLogicTest_EclipseProstate
  ${CMAKE_CURRENT_SOURCE_DIR}/../Data/EclipseProstate
  ${TEMP}
  0.0
  0.0
)
set_tests_properties(vtkSlicerDoseVolumeHistogramLogicTest_EclipseProstate PROPERTIES FAIL_REGULAR_EXPRESSION "Error;Warning" )

# Compare exported DVH metrics with baseline
add_test(
  NAME vtkSlicerDoseVolumeHistogramLogicTest_EclipseProstate_CompareMetrics
  COMMAND ${CMAKE_COMMAND} -E compare_files ${TEMP}/BaselineDvhTestMetrics.csv ${CMAKE_CURRENT_SOURCE_DIR}/../Data/DvhMetrics.csv
)
set_tests_properties(vtkSlicerDoseVolumeHistogramLogicTest_EclipseProstate_CompareMetrics PROPERTIES DEPENDS vtkSlicerDoseVolumeHistogramLogicTest_EclipseProstate)

#-----------------------------------------------------------------------------
TEST_WITH_DATA(
  vtkSlicerDoseVolumeHistogramLogicTest_EclipseProstate_CERR
  ${CMAKE_CURRENT_SOURCE_DIR}/../Data/EclipseProstate_CERR
  ${TEMP}
  1.0
  5.0
)
set_tests_properties(vtkSlicerDoseVolumeHistogramLogicTest_EclipseProstate_CERR PROPERTIES FAIL_REGULAR_EXPRESSION "Error;Warning" )

#-----------------------------------------------------------------------------
TEST_WITH_DATA(
  vtkSlicerDoseVolumeHistogramLogicTest_EclipseEnt_CERR
  ${CMAKE_CURRENT_SOURCE_DIR}/../Data/EclipseEnt_CERR
  ${TEMP}
  1.0
  5.0
)
set_tests_properties(vtkSlicerDoseVolumeHistogramLogicTest_EclipseEnt_CERR PROPERTIES FAIL_REGULAR_EXPRESSION "Error;Warning" )

#-----------------------------------------------------------------------------
TEST_WITH_DATA(
  vtkSlicerDoseVolumeHistogramLogicTest_EclipseBreast_CERR
  ${CMAKE_CURRENT_SOURCE_DIR}/../Data/EclipseBreast_CERR
  ${TEMP}
  1.0
  5.0
)
set_tests_properties(vtkSlicerDoseVolumeHistogramLogicTest_EclipseBreast_CERR PROPERTIES FAIL_REGULAR_EXPRESSION "Error;Warning" )
