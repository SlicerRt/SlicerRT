/*==============================================================================

  Copyright (c) Radiation Medicine Program, University Health Network,
  Princess Margaret Hospital, Toronto, ON, Canada. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Kevin Wang, Radiation Medicine Program, 
  University Health Network and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/

// .NAME vtkSlicerSegmentMorphologyModuleLogic - slicer logic class for volumes manipulation
// .SECTION Description
// This class manages the logic associated with reading, saving,
// and changing propertied of the volumes


#ifndef __vtkSlicerSegmentMorphologyModuleLogic_h
#define __vtkSlicerSegmentMorphologyModuleLogic_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"

#include "vtkSlicerSegmentMorphologyModuleLogicExport.h"

class vtkMRMLSegmentMorphologyNode;

/// \ingroup SlicerRt_QtModules_SegmentMorphology
class VTK_SLICER_SEGMENTMORPHOLOGY_MODULE_LOGIC_EXPORT vtkSlicerSegmentMorphologyModuleLogic :
  public vtkSlicerModuleLogic
{
public:
  static vtkSlicerSegmentMorphologyModuleLogic *New();
  vtkTypeMacro(vtkSlicerSegmentMorphologyModuleLogic, vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// Perform selected morphological operation
  /// \return Error message, empty string if no error
  std::string ApplyMorphologyOperation(vtkMRMLSegmentMorphologyNode* parameterNode);

protected:
  /// Generate output segment name from input segment names
  std::string GenerateOutputSegmentName(vtkMRMLSegmentMorphologyNode* parameterNode);

protected:
  vtkSlicerSegmentMorphologyModuleLogic();
  virtual ~vtkSlicerSegmentMorphologyModuleLogic();

  virtual void SetMRMLSceneInternal(vtkMRMLScene* newScene) override;

  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  virtual void RegisterNodes() override;

  virtual void UpdateFromMRMLScene() override;
  virtual void OnMRMLSceneNodeAdded(vtkMRMLNode* node) override;
  virtual void OnMRMLSceneNodeRemoved(vtkMRMLNode* node) override;
  virtual void OnMRMLSceneEndClose() override;

private:
  vtkSlicerSegmentMorphologyModuleLogic(const vtkSlicerSegmentMorphologyModuleLogic&); // Not implemented
  void operator=(const vtkSlicerSegmentMorphologyModuleLogic&);               // Not implemented
};

#endif
