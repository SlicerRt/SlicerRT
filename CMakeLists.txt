cmake_minimum_required(VERSION 2.8.9)

#-----------------------------------------------------------------------------
set(EXTENSION_NAME SlicerRT)

# ------------------------------------------------------------------------
set(SLICERRT_VERSION_MAJOR "0")
set(SLICERRT_VERSION_MINOR "15")
set(SLICERRT_VERSION_PATCH "0")
set(SLICERRT_VERSION ${SLICERRT_VERSION_MAJOR}.${SLICERRT_VERSION_MINOR}.${SLICERRT_VERSION_PATCH})

#-----------------------------------------------------------------------------
set(EXTENSION_HOMEPAGE "http://slicerrt.org")
set(EXTENSION_CATEGORY "Radiotherapy")
set(EXTENSION_CONTRIBUTORS "Csaba Pinter (PerkLab, Queen's University), Andras Lasso (PerkLab, Queen's University), Kevin Wang (Radiation Medicine Program, Princess Margaret Hospital, University Health Network Toronto), Greg Sharp (Massachusetts General Hospital)")
set(EXTENSION_DESCRIPTION "Modules for radiation therapy research. Features include DICOM-RT import/export, dose volume histogram, dose accumulation, contour handling, contour comparison, isodose contour/surface generation, etc. Version ${SLICERRT_VERSION}")
set(EXTENSION_ICONURL "http://www.slicer.org/slicerWiki/images/2/28/SlicerRT_Logo_2.0_128x128.png")
set(EXTENSION_SCREENSHOTURLS "http://www.slicer.org/slicerWiki/images/8/87/SlicerRT_0.10_IsocenterShiftingEvaluation.png http://www.slicer.org/slicerWiki/images/e/ef/SlicerRT_0.11_ProstateLoaded_Beams_ThresholdedDose.png http://www.slicer.org/slicerWiki/images/4/40/SlicerRtFundingSources.png")
set(EXTENSION_STATUS "Beta")
set(EXTENSION_DEPENDS "NA") # Specified as a space separated list or 'NA' if any
set(EXTENSION_BUILD_SUBDIRECTORY inner-build)

#-----------------------------------------------------------------------------
find_package(Slicer COMPONENTS ConfigurePrerequisites REQUIRED)
project(SlicerRT)
find_package(Slicer REQUIRED)
include(${Slicer_USE_FILE})

#-----------------------------------------------------------------------------
option(${EXTENSION_NAME}_SUPERBUILD "Build ${EXTENSION_NAME} and the projects it depends on via SuperBuild.cmake." ON)
mark_as_advanced(${EXTENSION_NAME}_SUPERBUILD)
if(${EXTENSION_NAME}_SUPERBUILD)
  include("${CMAKE_CURRENT_SOURCE_DIR}/SuperBuild.cmake")
  return()
endif()

#-----------------------------------------------------------------------------
OPTION(SLICERRT_ENABLE_EXPERIMENTAL_MODULES "Enable the building of work-in-progress, experimental modules." OFF)

#-----------------------------------------------------------------------------
# Plastimatch_DIR is only used now by experimental modules. Print the
# Plastimatch_DIR, to avoid the warning "Manually-specified variables were not
# used by the project: Plastimatch_DIR" warning
message(STATUS "Plastimatch_DIR: " ${Plastimatch_DIR} )
message(STATUS "SLICERRT_ENABLE_EXPERIMENTAL_MODULES: " ${SLICERRT_ENABLE_EXPERIMENTAL_MODULES} )

#-----------------------------------------------------------------------------
add_subdirectory(SlicerRtCommon)
add_subdirectory(PlmCommon)
IF (WIN32)
  add_subdirectory(LeakFinder)
ENDIF()

add_subdirectory(Contours)
add_subdirectory(Beams)
add_subdirectory(Isodose)
add_subdirectory(PlanarImage)
add_subdirectory(DicomRtImportExport)
add_subdirectory(DoseAccumulation)
add_subdirectory(DoseVolumeHistogram)
add_subdirectory(DvhComparison)
add_subdirectory(DoseComparison)
add_subdirectory(ContourMorphology)
add_subdirectory(ContourComparison)
add_subdirectory(VffFileReader)
add_subdirectory(DicomSroExport)
add_subdirectory(DicomSroImport)
add_subdirectory(BatchProcessing)

add_subdirectory(PlastimatchPy)
add_subdirectory(PlmBspline)
add_subdirectory(PlmLandwarp)
add_subdirectory(PlmRegister)

if (SLICERRT_ENABLE_EXPERIMENTAL_MODULES)
  # In development
  add_subdirectory(PlmThreshbox)
  add_subdirectory(ExternalBeamPlanning)
  add_subdirectory(PinnacleDvfReader)

  # Registration validation routines
  add_subdirectory(PlmSynth)
  add_subdirectory(PlmVectorFieldAnalysis)
endif()

if(BUILD_TESTING)
  add_subdirectory(Testing)
endif()

#-----------------------------------------------------------------------------
set(CPACK_INSTALL_CMAKE_PROJECTS "${CPACK_INSTALL_CMAKE_PROJECTS};${CMAKE_BINARY_DIR};${EXTENSION_NAME};ALL;/")
#set(CPACK_INSTALL_CMAKE_PROJECTS "${CPACK_INSTALL_CMAKE_PROJECTS};${<EP>_DIR};<EP>;RuntimeLibraries;/")
INCLUDE(${Slicer_EXTENSION_CPACK})
