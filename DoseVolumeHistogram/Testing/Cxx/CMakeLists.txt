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

macro(SIMPLE_TEST_WITH_DATA TESTNAME)
  add_test(
    NAME ${TESTNAME}
    COMMAND ${Slicer_LAUNCH_COMMAND} $<TARGET_FILE:${KIT}CxxTests> ${TESTNAME} ${ARGN}
    -D ${CMAKE_CURRENT_SOURCE_DIR}/../
    -T ${TEMP}
  )
endmacro()

SIMPLE_TEST_WITH_DATA(
  vtkSlicerDoseVolumeHistogramLogicTest1
)
set_tests_properties(vtkSlicerDoseVolumeHistogramLogicTest1 PROPERTIES FAIL_REGULAR_EXPRESSION "Error;Warning" )

#-----------------------------------------------------------------------------
# Compare exported DVH table with baseline
add_test(
  NAME vtkSlicerDoseVolumeHistogramLogicTest2
  COMMAND ${CMAKE_COMMAND} -E compare_files ${TEMP}/DvhTestTable.csv ${CMAKE_CURRENT_SOURCE_DIR}/../Data/DvhTable.csv
)
set_tests_properties(vtkSlicerDoseVolumeHistogramLogicTest2 PROPERTIES DEPENDS vtkSlicerDoseVolumeHistogramLogicTest1)

#-----------------------------------------------------------------------------
# Compare exported DVH table with baseline
add_test(
  NAME vtkSlicerDoseVolumeHistogramLogicTest3
  COMMAND ${CMAKE_COMMAND} -E compare_files ${TEMP}/DvhTestMetrics.csv ${CMAKE_CURRENT_SOURCE_DIR}/../Data/DvhMetrics.csv
)
set_tests_properties(vtkSlicerDoseVolumeHistogramLogicTest3 PROPERTIES DEPENDS vtkSlicerDoseVolumeHistogramLogicTest1)
