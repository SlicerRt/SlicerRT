#-----------------------------------------------------------------------------
set(KIT_UNITTEST_SCRIPTS)
SlicerMacroConfigureGenericPythonModuleTests("${MODULE_NAME}" KIT_UNITTEST_SCRIPTS)

#-----------------------------------------------------------------------------
foreach(script_name ${KIT_UNITTEST_SCRIPTS})
  slicer_add_python_unittest(
    SCRIPT ${script_name}
    SLICER_ARGS --no-main-window --disable-cli-modules
                --additional-module-path ${CMAKE_BINARY_DIR}/${Slicer_QTSCRIPTEDMODULES_LIB_DIR}
    TESTNAME_PREFIX nomainwindow_
    )
endforeach()

#-----------------------------------------------------------------------------
slicer_add_python_unittest(
  SCRIPT DicomRtImportTest1.py
  SLICER_ARGS --no-main-window TESTNAME_PREFIX nomainwindow_)
