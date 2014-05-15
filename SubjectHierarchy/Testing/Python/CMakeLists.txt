include(SlicerMacroBuildScriptedModule)
set(SUBJECTHIERARCHY_PYTHON_RESOURCES
  )

# Test subject hierarchy
slicerMacroBuildScriptedModule(
  NAME SubjectHierarchyGenericSelfTest
  SCRIPTS SubjectHierarchyGenericSelfTest.py
  RESOURCES ${SUBJECTHIERARCHY_PYTHON_RESOURCES}
  )
slicer_add_python_unittest(SCRIPT SubjectHierarchyGenericSelfTest.py
                           SLICER_ARGS --no-main-window --disable-cli-modules)
