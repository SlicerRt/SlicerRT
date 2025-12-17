if(Slicer_USE_PYTHONQT)
  add_subdirectory(Python)
endif()

# Add C++ tests
if(EXTENSION_BUILDS_IPOPT OR (EXTENSION_BUILDS_ADAPTIVE_CPP AND NOT WIN32))
  add_subdirectory(Cxx)
endif()
