#-----------------------------------------------------------------------------
# External projects
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
    -DCMAKE_CXX_COMPILER:STRING=${CMAKE_CXX_COMPILER}
  #--Build step-----------------
  #--Install step-----------------
  # Don't perform installation at the end of the build
  INSTALL_COMMAND ""
  ) 
