include(ExternalProject)

PROJECT(SlicerRtSuperbuild)

OPTION(SLICERRT_ENABLE_EXPERIMENTAL_MODULES "Enable the building of work-in-progress, experimental modules." OFF)

#-----------------------------------------------------------------------------
SET (DCMTK_DIR "${Slicer_DIR}/../DCMTK-install")
SET (SLICERRT_PLASTIMATCH_SOURCE_DIR "${CMAKE_BINARY_DIR}/Plastimatch" CACHE INTERNAL "Path to store Plastimatch sources.")
SET (SLICERRT_PLASTIMATCH_DIR "${CMAKE_BINARY_DIR}/Plastimatch-build" CACHE INTERNAL "Path to store Plastimatch binaries.")
ExternalProject_Add( Plastimatch
  SOURCE_DIR "${SLICERRT_PLASTIMATCH_SOURCE_DIR}" 
  BINARY_DIR "${SLICERRT_PLASTIMATCH_DIR}"
  #--Download step--------------
  SVN_USERNAME "anonymous"
  SVN_PASSWORD "anonymous"
  SVN_REPOSITORY https://forge.abcd.harvard.edu/svn/plastimatch/plastimatch/trunk
  #--Configure step-------------
  CMAKE_ARGS 
    -DSlicer_DIR:STRING=${Slicer_DIR}
    -DBUILD_SHARED_LIBS:BOOL=OFF
    -DBUILD_TESTING:BOOL=OFF 
    -DPLM_CONFIG_LIBRARY_BUILD:BOOL=ON     
    -DPLM_CONFIG_INSTALL_LIBRARIES:BOOL=ON    
    -DDCMTK_DIR:STRING=${DCMTK_DIR}
  #--Build step-----------------
  #--Install step-----------------
  # Don't perform installation at the end of the build
  INSTALL_COMMAND ""
  )

#-----------------------------------------------------------------------------
ExternalProject_Add(SlicerRt
  DOWNLOAD_COMMAND ""
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}
  BINARY_DIR "SlicerRt-build"
  UPDATE_COMMAND ""
  CMAKE_ARGS
    -DSlicer_DIR:STRING=${Slicer_DIR}
    -DPlastimatch_DIR:STRING=${SLICERRT_PLASTIMATCH_DIR}
    -DSLICERRT_SUPERBUILD:BOOL=OFF
    -DSLICERRT_ENABLE_EXPERIMENTAL_MODULES:BOOL=${SLICERRT_ENABLE_EXPERIMENTAL_MODULES}
  # Don't perform installation at the end of the build
  INSTALL_COMMAND ""
  DEPENDS "Plastimatch"
)
