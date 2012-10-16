#-----------------------------------------------------------------------------
configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/DicomRtImportSelfTest.py.in 
	${CMAKE_BINARY_DIR}/${Slicer_QTSCRIPTEDMODULES_LIB_DIR}/DicomRtImportSelfTest.py
	)
	
#-----------------------------------------------------------------------------
set(MODULE_PYTHON_SCRIPTS
  DicomRtImportSelfTest.py
  )

set(MODULE_PYTHON_RESOURCES
  )

ctkMacroCompilePythonScript(
  TARGET_NAME DicomRtImportSelfTest
  SCRIPTS "${MODULE_PYTHON_SCRIPTS}"
  RESOURCES "${MODULE_PYTHON_RESOURCES}"
  DESTINATION_DIR ${CMAKE_BINARY_DIR}/${Slicer_QTSCRIPTEDMODULES_LIB_DIR}
  INSTALL_DIR ${Slicer_INSTALL_QTSCRIPTEDMODULES_LIB_DIR}
  NO_INSTALL_SUBDIR
  )

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
  SCRIPT DicomRtImportSelfTest.py
  SLICER_ARGS #--no-main-window
              --disable-cli-modules
              --additional-module-paths
			  ${CMAKE_BINARY_DIR}/${Slicer_QTLOADABLEMODULES_LIB_DIR}
			  ${CMAKE_BINARY_DIR}/${Slicer_QTLOADABLEMODULES_LIB_DIR}/Debug
			  ${CMAKE_BINARY_DIR}/${Slicer_QTLOADABLEMODULES_LIB_DIR}/Release
			  ${CMAKE_BINARY_DIR}/${Slicer_QTSCRIPTEDMODULES_LIB_DIR} 
  )
