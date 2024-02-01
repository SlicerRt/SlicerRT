#-----------------------------------------------------------------------------
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/PlanOptimizers.__init__.py.in
  ${CMAKE_BINARY_DIR}/${Slicer_QTSCRIPTEDMODULES_LIB_DIR}/PlanOptimizers/__init__.py
  )

#-----------------------------------------------------------------------------
set(PlanOptimizers_PYTHON_SCRIPTS
  AbstractScriptedPlanOptimizer
  pyRadPlanPlanOptimizer
  )

set(PlanOptimizers_PYTHON_RESOURCES
  )

ctkMacroCompilePythonScript(
  TARGET_NAME PlanOptimizers
  SCRIPTS "${PlanOptimizers_PYTHON_SCRIPTS}"
  RESOURCES "${PlanOptimizers_PYTHON_RESOURCES}"
  DESTINATION_DIR ${CMAKE_BINARY_DIR}/${Slicer_QTSCRIPTEDMODULES_LIB_DIR}/PlanOptimizers
  INSTALL_DIR ${Slicer_INSTALL_QTSCRIPTEDMODULES_LIB_DIR}/PlanOptimizers
  )