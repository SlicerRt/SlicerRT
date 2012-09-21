#-----------------------------------------------------------------------------
# External projects
#-----------------------------------------------------------------------------

SET (DCMTK_DIR "${Slicer_DIR}/../DCMTK-install")
SET (SLICERRT_PLASTIMATCH_SOURCE_DIR "${CMAKE_BINARY_DIR}/Plastimatch" CACHE INTERNAL "Path to store Plastimatch sources.")
SET (SLICERRT_PLASTIMATCH_DIR "${CMAKE_BINARY_DIR}/Plastimatch-build" CACHE INTERNAL "Path to store Plastimatch binaries.")

if (SLICERRT_ENABLE_EXPERIMENTAL_MODULES)
  set (PLASTIMATCH_EXTRA_LIBRARIES 
    -DPLMLIB_CONFIG_ENABLE_DOSE:BOOL=TRUE 
    -DPLMLIB_CONFIG_ENABLE_REGISTER:BOOL=TRUE)
else ()
  set (PLASTIMATCH_EXTRA_LIBRARIES "")
endif ()

ExternalProject_Add( Plastimatch
  SOURCE_DIR "${SLICERRT_PLASTIMATCH_SOURCE_DIR}" 
  BINARY_DIR "${SLICERRT_PLASTIMATCH_DIR}"
  #--Download step--------------
  SVN_USERNAME "anonymous"
  SVN_PASSWORD "anonymous"
  SVN_REPOSITORY https://forge.abcd.harvard.edu/svn/plastimatch/plastimatch/trunk
  # Avoid "Server certificate verification failed: issuer is not trusted" error
  SVN_TRUST_CERT 1
  #--Configure step-------------
  CMAKE_ARGS 
    #If Plastimatch is build in library mode (PLM_CONFIG_LIBRARY_BUILD) then does not use Slicer libraries
    #-DSlicer_DIR:STRING=${Slicer_DIR}
    -DBUILD_SHARED_LIBS:BOOL=OFF 
    -DBUILD_TESTING:BOOL=OFF 
    -DPLM_CONFIG_LIBRARY_BUILD:BOOL=ON 
    -DPLM_CONFIG_INSTALL_LIBRARIES:BOOL=ON 
    ${PLASTIMATCH_EXTRA_LIBRARIES}
    -DDCMTK_DIR:STRING=${DCMTK_DIR}
    -DITK_DIR:STRING=${ITK_DIR}    
    -DCMAKE_CXX_COMPILER:STRING=${CMAKE_CXX_COMPILER}    
  #--Build step-----------------
  #--Install step-----------------
  # Don't perform installation at the end of the build
  INSTALL_COMMAND ""
  ) 
