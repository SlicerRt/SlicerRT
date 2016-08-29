/*==============================================================================

  Copyright (c) Laboratory for Percutaneous Surgery (PerkLab)
  Queen's University, Kingston, ON, Canada. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Kyle Sunderland, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

#ifndef __vtkClosedSurfaceToFractionalLabelmapConversionRule_h
#define __vtkClosedSurfaceToFractionalLabelmapConversionRule_h

// SegmentationCore includes
#include <vtkSegmentationConverterRule.h>
#include <vtkSegmentationConverter.h>
#include <vtkSegmentationCoreConfigure.h>
#include <vtkClosedSurfaceToBinaryLabelmapConversionRule.h>

// DicomRtImportExport includes
#include "vtkSlicerDicomRtImportExportConversionRulesExport.h"

// SlicerRtCommon includes
#include "vtkFractionalPolyDataToImageStencil.h"

// VTK includes
#include <vtkPolyDataToImageStencil.h>

class vtkPolyData;

/// \ingroup SegmentationCore
/// \brief Convert closed surface representation (vtkPolyData type) to fractional
///   labelmap representation (vtkOrientedImageData type). The conversion algorithm
///   is based on image stencil.
class VTK_SLICER_DICOMRTIMPORTEXPORT_CONVERSIONRULES_EXPORT vtkClosedSurfaceToFractionalLabelmapConversionRule
  : public vtkClosedSurfaceToBinaryLabelmapConversionRule
{

public:
  static vtkClosedSurfaceToFractionalLabelmapConversionRule* New();
  vtkTypeMacro(vtkClosedSurfaceToFractionalLabelmapConversionRule, vtkClosedSurfaceToBinaryLabelmapConversionRule);
  virtual vtkSegmentationConverterRule* CreateRuleInstance();

  /// Constructs representation object from representation name for the supported representation classes
  /// (typically source and target representation VTK classes, subclasses of vtkDataObject)
  /// Note: Need to take ownership of the created object! For example using vtkSmartPointer<vtkDataObject>::Take
  virtual vtkDataObject* ConstructRepresentationObjectByRepresentation(std::string representationName);

  /// Constructs representation object from class name for the supported representation classes
  /// (typically source and target representation VTK classes, subclasses of vtkDataObject)
  /// Note: Need to take ownership of the created object! For example using vtkSmartPointer<vtkDataObject>::Take
  virtual vtkDataObject* ConstructRepresentationObjectByClass(std::string className);

  /// Update the target representation based on the source representation
  virtual bool Convert(vtkDataObject* sourceRepresentation, vtkDataObject* targetRepresentation);

  /// Get the cost of the conversion.
  virtual unsigned int GetConversionCost(vtkDataObject* sourceRepresentation=NULL, vtkDataObject* targetRepresentation=NULL);

  /// Human-readable name of the converter rule
  virtual const char* GetName() { return "Closed surface to fractional labelmap (simple image stencil)"; };

  /// Human-readable name of the source representation
  virtual const char* GetSourceRepresentationName() { return vtkSegmentationConverter::GetSegmentationClosedSurfaceRepresentationName(); };

  /// Human-readable name of the target representation
  virtual const char* GetTargetRepresentationName() { return vtkSegmentationConverter::GetSegmentationFractionalLabelmapRepresentationName(); };

protected:

  /// Create a binary labelmap from a closed surface and add it to the fractional labelmap
  /// This function accepts a void pointer to a struct of the following format:
  /// \param closedSurface Closed surface that is being converted to a binary labelmap
  /// \param binaryLabelMap The binary labelmap that is created from the closed surface
  /// \param polyDataToImageStencil The vtkFractionalPolyDataToImageStencil containing cached information
  void CreateBinaryLabelMap(vtkPolyData* closedSurface, vtkOrientedImageData* binaryLabelMap, vtkFractionalPolyDataToImageStencil* polyDataToImageStencil);

  /// Add the values of the binary labelmap to the fractional labelmap.
  /// \param binaryLabelMap Binary labelmap that will be added to the fractional labelmap
  /// \param fractionalLabelMap The fractional labelmap that the binary labelmap is added to
  void AddBinaryLabelMapToFractionalLabelMap(vtkOrientedImageData* binaryLabelMap, vtkOrientedImageData* fractionalLabelMap);

protected:
  // Oversampling factor that will be used to calculate the size of the binary labelmap
  int NumberOfOffsets;

protected:
  vtkClosedSurfaceToFractionalLabelmapConversionRule();
  ~vtkClosedSurfaceToFractionalLabelmapConversionRule();
  void operator=(const vtkClosedSurfaceToFractionalLabelmapConversionRule&);
};

#endif // __vtkClosedSurfaceToFractionalLabelmapConversionRule_h
