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

// .NAME vtkSlicerBeamsModuleLogic - slicer logic class for volumes manipulation
// .SECTION Description
// This class manages the logic associated with reading, saving,
// and changing propertied of the volumes


#ifndef __vtkSlicerBeamsModuleLogic_h
#define __vtkSlicerBeamsModuleLogic_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"

// STD includes
#include "vtkSlicerBeamsModuleLogicExport.h"

class vtkTransform;
class vtkPolyData;
class vtkDoubleArray;
class vtkMRMLRTPlanNode;
class vtkMRMLRTBeamNode;
class vtkMRMLSubjectHierarchyNode;

/// \ingroup SlicerRt_QtModules_Beams
class VTK_SLICER_BEAMS_LOGIC_EXPORT vtkSlicerBeamsModuleLogic :
  public vtkSlicerModuleLogic
{
public:
  static vtkSlicerBeamsModuleLogic *New();
  vtkTypeMacro(vtkSlicerBeamsModuleLogic,vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

public:
  /// TODO
  void SetAndObserveRTPlanNode(vtkMRMLRTPlanNode* node);

  /// Get the EBP Node
  vtkGetObjectMacro(RTPlanNode, vtkMRMLRTPlanNode);

  /// Create a RTPlanNode if it has not been created before
  /// and set up the SubjectHierarchyNode for it
  vtkMRMLRTPlanNode* CreateDefaultRTPlanNode(const char* nodeName);

  /// Create a new beam of default type
  vtkMRMLRTBeamNode* CreateDefaultRTBeamNode(const char*);

  // Create a default model, and attach to the supplied beam node
  static void AddDefaultModelToRTBeamNode(vtkMRMLScene*, vtkMRMLRTBeamNode*);

  /// Remove a beam with a specified beam name
  void RemoveRTBeamNodeInSubjectHierarchyByID(const char*);

  // Update the beam model for a new isocenter, gantry angle, etc.
  static void UpdateBeamTransform(vtkMRMLScene*, vtkMRMLRTBeamNode*);

  /// TODO
  void UpdateBeamTransformByID(const char*);

  /// TODO
  void UpdateBeamGeometryModelByID(const char*);

  /// Create beam model from beam parameters, supporting MLC leaves
  /// \param x/y jaw positions, SAD, leaf positions
  /// \return Poly data, null on fail
  static vtkSmartPointer<vtkPolyData> CreateBeamPolyData(double X1, double X2, double Y1, double Y2, double SAD, vtkDoubleArray* leafPositionsArray);

  /// Create square beam model from beam parameters
  /// \param x/y jaw positions and SAD
  /// \return Poly data, null on fail
  static vtkSmartPointer<vtkPolyData> CreateBeamPolyData(double X1, double X2, double Y1, double Y2, double SAD);

protected:
  vtkSlicerBeamsModuleLogic();
  virtual ~vtkSlicerBeamsModuleLogic();

  virtual void SetMRMLSceneInternal(vtkMRMLScene* newScene);

  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  virtual void RegisterNodes();

  virtual void UpdateFromMRMLScene();
  virtual void OnMRMLSceneNodeAdded(vtkMRMLNode* node);
  virtual void OnMRMLSceneNodeRemoved(vtkMRMLNode* node);
  virtual void OnMRMLSceneEndImport();
  virtual void OnMRMLSceneEndClose();

private:
  vtkSlicerBeamsModuleLogic(const vtkSlicerBeamsModuleLogic&); // Not implemented
  void operator=(const vtkSlicerBeamsModuleLogic&);               // Not implemented

protected:
  /// RTPlan node
  vtkMRMLRTPlanNode* RTPlanNode;
};

#endif

