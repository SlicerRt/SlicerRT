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
#include "vtkSlicerRtCommon.h"

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
  virtual vtkSegmentationConverterRule* CreateRuleInstance() override;

  /// Human-readable name of the converter rule
  virtual const char* GetName() override { return "Ribbon model to binary labelmap"; };
  
  /// Human-readable name of the source representation
  virtual const char* GetSourceRepresentationName() override { return vtkSlicerRtCommon::SEGMENTATION_RIBBON_MODEL_REPRESENTATION_NAME; };
  
  /// Human-readable name of the target representation
  virtual const char* GetTargetRepresentationName() override { return vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName(); };

protected:
  vtkRibbonModelToBinaryLabelmapConversionRule();
  ~vtkRibbonModelToBinaryLabelmapConversionRule();

private:
  vtkRibbonModelToBinaryLabelmapConversionRule(const vtkRibbonModelToBinaryLabelmapConversionRule&); // Not implemented
  void operator=(const vtkRibbonModelToBinaryLabelmapConversionRule&); // Not implemented
};

#endif // __vtkRibbonModelToBinaryLabelmapConversionRule_h
