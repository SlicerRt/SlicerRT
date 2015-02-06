#-----------------------------------------------------------------------------
# External projects
#-----------------------------------------------------------------------------

if (Plastimatch_DIR)
  # Plastimatch is built already, so just use that
  set(SLICERRT_PLASTIMATCH_DIR ${Plastimatch_DIR} CACHE INTERNAL "Path to store Plastimatch binaries.")
  message(STATUS "Use Plastimatch library at ${SLICERRT_PLASTIMATCH_DIR}")  
  # Define a "empty" project in case an external one is provided
  # Doing so allows to keep the external project dependency system happy.
  ExternalProject_Add(Plastimatch
    SOURCE_DIR ${CMAKE_BINARY_DIR}/PlastimatchSurrogate
    BINARY_DIR PlastimatchSurrogate-build
    DOWNLOAD_COMMAND ""
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    DEPENDS ""
    )
  return()
endif ()

# Download and build Plastimatch

SET (DCMTK_DIR "${Slicer_DIR}/../DCMTK-build")
SET (SLICERRT_PLASTIMATCH_SOURCE_DIR "${CMAKE_BINARY_DIR}/Plastimatch" CACHE INTERNAL "Path to store Plastimatch sources.")
SET (SLICERRT_PLASTIMATCH_DIR "${CMAKE_BINARY_DIR}/Plastimatch-build" CACHE INTERNAL "Path to store Plastimatch binaries.")

# Choose which Plastimatch components to build
set (PLASTIMATCH_EXTRA_LIBRARIES 
  -DPLM_CONFIG_LIBRARY_BUILD:BOOL=ON 
  -DPLMLIB_CONFIG_ENABLE_REGISTER:BOOL=TRUE)
if (SLICERRT_ENABLE_EXPERIMENTAL_MODULES)
  set (PLASTIMATCH_EXTRA_LIBRARIES 
    ${PLASTIMATCH_EXTRA_LIBRARIES} 
    -DPLMLIB_CONFIG_ENABLE_DOSE:BOOL=TRUE)
endif ()

# Choose which Plastimatch revision to build
set (PLM_SVN_REVISION "4961")

# With CMake 2.8.9 or later, the UPDATE_COMMAND is required for updates to occur.
# For earlier versions, we nullify the update state to prevent updates and
# undesirable rebuild.
set(Plastimatch_EP_DISABLED_UPDATE UPDATE_COMMAND "")
if(CMAKE_VERSION VERSION_LESS 2.8.9)
  set(Plastimatch_EP_UPDATE_IF_CMAKE_LATER_THAN_288 ${${PROJECT_NAME}_EP_DISABLED_UPDATE})
else()
  set(Plastimatch_EP_UPDATE_IF_CMAKE_LATER_THAN_288 LOG_UPDATE 1)
endif()

# Disable OpenMP on Windows until a better solution can be found
# http://www.na-mic.org/Bug/view.php?id=3823
set (DISABLE_OPENMP_ON_WINDOWS "")
if (WIN32)
    set (DISABLE_OPENMP_ON_WINDOWS "-DPLM_CONFIG_DISABLE_OPENMP:BOOL=ON")
endif ()

ExternalProject_Add( Plastimatch
  SOURCE_DIR "${SLICERRT_PLASTIMATCH_SOURCE_DIR}" 
  BINARY_DIR "${SLICERRT_PLASTIMATCH_DIR}"
  #--Download step--------------
  SVN_USERNAME "anonymous"
  SVN_PASSWORD "anonymous"
  SVN_REPOSITORY https://forge.abcd.harvard.edu/svn/plastimatch/plastimatch/trunk
  SVN_REVISION -r "${PLM_SVN_REVISION}"
  "${Plastimatch_EP_UPDATE_IF_CMAKE_LATER_THAN_288}"
  # Avoid "Server certificate verification failed: issuer is not trusted" error
  SVN_TRUST_CERT 1
  #--Configure step-------------
  CMAKE_ARGS 
    # If Plastimatch is build in library mode (PLM_CONFIG_LIBRARY_BUILD) then does not use Slicer libraries
    # -DSlicer_DIR:STRING=${Slicer_DIR}
    -DBUILD_SHARED_LIBS:BOOL=OFF 
    -DBUILD_TESTING:BOOL=OFF 
    -DPLM_CONFIG_INSTALL_LIBRARIES:BOOL=ON 
    # CUDA build is disabled until ticket #226 can be resolved.
    -DPLM_CONFIG_DISABLE_CUDA:BOOL=ON
    ${DISABLE_OPENMP_ON_WINDOWS}
    ${PLASTIMATCH_EXTRA_LIBRARIES}
    -DDCMTK_DIR:STRING=${DCMTK_DIR}
    -DITK_DIR:STRING=${ITK_DIR}
    -DVTK_DIR:STRING=${VTK_DIR}
    -DCMAKE_CXX_COMPILER:STRING=${CMAKE_CXX_COMPILER}
    -DCMAKE_OSX_DEPLOYMENT_TARGET:STRING=${CMAKE_OSX_DEPLOYMENT_TARGET}
    -DCMAKE_OSX_SYSROOT:PATH=${CMAKE_OSX_SYSROOT}
  #--Build step-----------------
  #--Install step-----------------
  # Don't perform installation at the end of the build
  INSTALL_COMMAND ""
  ) 
