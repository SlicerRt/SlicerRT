include(ExternalProject)

PROJECT(SlicerRtSuperbuild)

OPTION(SLICERRT_ENABLE_EXPERIMENTAL_MODULES "Enable the building of work-in-progress, experimental modules." OFF)

SET (DCMTK_DIR "${Slicer_DIR}/../DCMTK-install")
# These are needed temporarily only (until FindDCMTK in Plastimatch is getting fixed
SET (DCMTK_INCLUDE_DIR "${DCMTK_DIR}/include")
find_path( DCMTK_dcmjpeg_INCLUDE_DIR NAMES djutils.h PATHS "${DCMTK_INCLUDE_DIR}/dcmtk/dcmjpeg")
find_library( DCMTK_dcmjpeg_LIBRARY dcmjpeg PATHS "${DCMTK_DIR}/lib" NO_DEFAULT_PATH)
find_library( DCMTK_ijg8_LIBRARY ijg8 PATHS "${DCMTK_DIR}/lib" NO_DEFAULT_PATH)
find_library( DCMTK_ijg12_LIBRARY ijg12 PATHS "${DCMTK_DIR}/lib" NO_DEFAULT_PATH)
find_library( DCMTK_ijg16_LIBRARY ijg16 PATHS "${DCMTK_DIR}/lib" NO_DEFAULT_PATH)
find_library( DCMTK_dcmtls_LIBRARY dcmtls PATHS "${DCMTK_DIR}/lib" NO_DEFAULT_PATH)
find_library( DCMTK_ofstd_LIBRARY ofstd PATHS "${DCMTK_DIR}/lib" NO_DEFAULT_PATH)
find_library( DCMTK_dcmdata_LIBRARY dcmdata PATHS "${DCMTK_DIR}/lib" NO_DEFAULT_PATH)
find_library( DCMTK_dcmimgle_LIBRARY dcmimgle PATHS "${DCMTK_DIR}/lib" NO_DEFAULT_PATH)
find_library( DCMTK_dcmnet_LIBRARY dcmnet PATHS "${DCMTK_DIR}/lib" NO_DEFAULT_PATH)
find_library( DCMTK_imagedb_LIBRARY dcmimage PATHS "${DCMTK_DIR}/lib" NO_DEFAULT_PATH)
find_library( DCMTK_oflog_LIBRARY oflog PATHS "${DCMTK_DIR}/lib" NO_DEFAULT_PATH)

#-----------------------------------------------------------------------------
SET (SLICERRT_PLASTIMATCH_SOURCE_DIR "${CMAKE_BINARY_DIR}/Plastimatch" CACHE INTERNAL "Path to store Plastimatch sources.")
SET (SLICERRT_PLASTIMATCH_DIR "${CMAKE_BINARY_DIR}/Plastimatch-build" CACHE INTERNAL "Path to store Plastimatch binaries.")
ExternalProject_Add( Plastimatch
  SOURCE_DIR "${SLICERRT_PLASTIMATCH_SOURCE_DIR}" 
  BINARY_DIR "${SLICERRT_PLASTIMATCH_DIR}"
  #--Download step--------------
  SVN_USERNAME "anonymous"
  SVN_PASSWORD ""
  SVN_REPOSITORY https://forge.abcd.harvard.edu/svn/plastimatch/plastimatch/trunk
  #--Configure step-------------
  CMAKE_ARGS 
    -DSlicer_DIR:STRING=${Slicer_DIR}
    -DBUILD_SHARED_LIBS:BOOL=OFF
    -DBUILD_TESTING:BOOL=OFF 
    -DPLM_CONFIG_LIBRARY_BUILD:BOOL=ON     
    -DPLM_CONFIG_INSTALL_LIBRARIES:BOOL=ON    
    -DDCMTK_DIR:STRING=${DCMTK_DIR}
    -DDCMTK_INCLUDE_DIR:STRING=${DCMTK_INCLUDE_DIR}
    -DDCMTK_dcmjpeg_INCLUDE_DIR:STRING=${DCMTK_dcmjpeg_INCLUDE_DIR}
    -DDCMTK_dcmjpeg_LIBRARY:STRING=${DCMTK_dcmjpeg_LIBRARY}
    -DDCMTK_ijg8_LIBRARY:STRING=${DCMTK_ijg8_LIBRARY}
    -DDCMTK_ijg12_LIBRARY:STRING=${DCMTK_ijg12_LIBRARY}
    -DDCMTK_ijg16_LIBRARY:STRING=${DCMTK_ijg16_LIBRARY}
    -DDCMTK_dcmtls_LIBRARY:STRING=${DCMTK_dcmtls_LIBRARY}
    -DDCMTK_ofstd_LIBRARY:STRING=${DCMTK_ofstd_LIBRARY}
    -DDCMTK_dcmdata_LIBRARY:STRING=${DCMTK_dcmdata_LIBRARY}
    -DDCMTK_dcmimgle_LIBRARY:STRING=${DCMTK_dcmimgle_LIBRARY}
    -DDCMTK_dcmnet_LIBRARY:STRING=${DCMTK_dcmnet_LIBRARY}
    -DDCMTK_imagedb_LIBRARY:STRING=${DCMTK_imagedb_LIBRARY}
    -DDCMTK_oflog_LIBRARY:STRING=${DCMTK_oflog_LIBRARY}    
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
