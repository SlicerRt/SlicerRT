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

#ifndef __vtkRibbonModelToBinaryLabelmapConversionRule_h
#define __vtkRibbonModelToBinaryLabelmapConversionRule_h

// SegmentationCore includes
#include "vtkClosedSurfaceToBinaryLabelmapConversionRule.h"
#include "vtkSegmentationConverter.h"

// SlicerRT includes
#include "SlicerRtCommon.h"

#include "vtkSlicerDicomRtImportExportConversionRulesExport.h"

/// \ingroup DicomRtImportImportExportConversionRules
/// \brief Convert ribbon model representation (vtkPolyData type) to binary
///   labelmap representation (vtkOrientedImageData type). The conversion algorithm
///   is the same as the base class \sa vtkClosedSurfaceToBinaryLabelmapConversionRule
class VTK_SLICER_DICOMRTIMPORTEXPORT_CONVERSIONRULES_EXPORT vtkRibbonModelToBinaryLabelmapConversionRule
  : public vtkClosedSurfaceToBinaryLabelmapConversionRule
{
public:
  static vtkRibbonModelToBinaryLabelmapConversionRule* New();
  vtkTypeMacro(vtkRibbonModelToBinaryLabelmapConversionRule, vtkSegmentationConverterRule);
  virtual vtkSegmentationConverterRule* CreateRuleInstance();

  /// Human-readable name of the converter rule
  virtual const char* GetName(){ return "Ribbon model to binary labelmap"; };
  
  /// Human-readable name of the source representation
  virtual const char* GetSourceRepresentationName() { return SlicerRtCommon::SEGMENTATION_RIBBON_MODEL_REPRESENTATION_NAME; };
  
  /// Human-readable name of the target representation
  virtual const char* GetTargetRepresentationName() { return vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName(); };

protected:
  vtkRibbonModelToBinaryLabelmapConversionRule();
  ~vtkRibbonModelToBinaryLabelmapConversionRule();
  void operator=(const vtkRibbonModelToBinaryLabelmapConversionRule&);
};

#endif // __vtkRibbonModelToBinaryLabelmapConversionRule_h
