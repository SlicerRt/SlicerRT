#-----------------------------------------------------------------------------
set(MODULE_NAME PolyDataDistanceHistogramFilterTest)

#-----------------------------------------------------------------------------
set(MODULE_TEST_PYTHON_SCRIPTS
  PolyDataDistanceHistogramFilterTest.py
  )

set(MODULE_TEST_PYTHON_RESOURCES
  )

#-----------------------------------------------------------------------------
set(KIT_UNITTEST_SCRIPTS)
SlicerMacroConfigureGenericPythonModuleTests("${MODULE_NAME}" KIT_UNITTEST_SCRIPTS)

#-----------------------------------------------------------------------------
if(CMAKE_CONFIGURATION_TYPES)
  set(MODULE_BUILD_DIR "")
  foreach(config ${CMAKE_CONFIGURATION_TYPES})
    list(APPEND MODULE_BUILD_DIR "${CMAKE_BINARY_DIR}/${Slicer_QTLOADABLEMODULES_LIB_DIR}/${config}")
  endforeach()
else()
  set(MODULE_BUILD_DIR "${CMAKE_BINARY_DIR}/${Slicer_QTLOADABLEMODULES_LIB_DIR}")
endif()

slicer_add_python_unittest(
  SCRIPT PolyDataDistanceHistogramFilterTest.py
  SLICER_ARGS --disable-cli-modules
              --no-main-window
              --additional-module-paths
                ${MODULE_BUILD_DIR}
                ${CMAKE_BINARY_DIR}/${Slicer_QTSCRIPTEDMODULES_LIB_DIR} 
  TESTNAME_PREFIX nomainwindow_
  )

