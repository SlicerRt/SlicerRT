/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Kitware Inc.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Csaba Pinter, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

#ifndef __vtkPlanarContourToRibbonModelConversionRule_h
#define __vtkPlanarContourToRibbonModelConversionRule_h

// SegmentationCore includes
#include "vtkSegmentationConverterRule.h"
#include "vtkSegmentationConverter.h"

// SlicerRT includes
#include "SlicerRtCommon.h"

#include "vtkSlicerDicomRtImportExportConversionRulesExport.h"

class vtkPolyData;
class vtkPoints;
class vtkCell;
class vtkPlane;

/// \ingroup DicomRtImportImportExportConversionRules
/// \brief Convert planar contour representation (vtkPolyData type) to ribbon
///   model representation (also vtkPolyData) by thickening the contours along
///   a normal vector orthogonal to the planes
class VTK_SLICER_DICOMRTIMPORTEXPORT_CONVERSIONRULES_EXPORT vtkPlanarContourToRibbonModelConversionRule
  : public vtkSegmentationConverterRule
{
public:
  static vtkPlanarContourToRibbonModelConversionRule* New();
  vtkTypeMacro(vtkPlanarContourToRibbonModelConversionRule, vtkSegmentationConverterRule);
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
  virtual const char* GetName() { return "Planar contour to ribbon model"; };
  
  /// Human-readable name of the source representation
  virtual const char* GetSourceRepresentationName() { return vtkSegmentationConverter::GetSegmentationPlanarContourRepresentationName(); };
  
  /// Human-readable name of the target representation
  virtual const char* GetTargetRepresentationName() { return SlicerRtCommon::SEGMENTATION_RIBBON_MODEL_REPRESENTATION_NAME; };

protected:
  /// Compute plane for a given contour
  /// \param contourPoints Point list containing all the points (from which \sa contourCell selects a subset)
  /// \param contourCell Input contour cell, indices of the points in \sa contourPoints belonging to the contour
  /// \param contourPlane Computed contour plane (output argument)
  bool ComputePlaneForContour(vtkPoints* contourPoints, vtkCell* contourCell, vtkPlane* contourPlane);

  /// Determine the distance between contour planes based on the actual planar contour data
  /// \param planarContourPolyData Input poly data containing the planar contours
  /// \param contoursPlane Output argument for plane of the contours
  /// \return Computed plane spacing. 1mm in case of critical errors (so that the ribbon can be visualized in all cases)
  double ComputeContourPlaneSpacing(vtkPolyData* planarContourPolyData, vtkPlane* contoursPlane);

protected:
  vtkPlanarContourToRibbonModelConversionRule();
  ~vtkPlanarContourToRibbonModelConversionRule();
  void operator=(const vtkPlanarContourToRibbonModelConversionRule&);
};

#endif // __vtkPlanarContourToRibbonModelConversionRule_h
