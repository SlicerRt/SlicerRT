/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Kevin Wang, Princess Margaret Cancer Centre 
  and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/


#ifndef __vtkMRMLIsodoseNode_h
#define __vtkMRMLIsodoseNode_h

// MRML includes
#include <vtkMRML.h>
#include <vtkMRMLNode.h>

#include "vtkSlicerIsodoseModuleLogicExport.h"

class vtkMRMLScalarVolumeNode;
class vtkMRMLModelHierarchyNode;
class vtkMRMLColorTableNode;

/// \ingroup SlicerRt_QtModules_Isodose
class VTK_SLICER_ISODOSE_LOGIC_EXPORT vtkMRMLIsodoseNode : public vtkMRMLNode
{
public:
  static vtkMRMLIsodoseNode *New();
  vtkTypeMacro(vtkMRMLIsodoseNode, vtkMRMLNode);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// Create instance of a GAD node. 
  virtual vtkMRMLNode* CreateNodeInstance();

  /// Set node attributes from name/value pairs 
  virtual void ReadXMLAttributes( const char** atts);

  /// Write this node's information to a MRML file in XML format. 
  virtual void WriteXML(ostream& of, int indent);

  /// Copy the node's attributes to this object 
  virtual void Copy(vtkMRMLNode *node);

  /// Get unique node XML tag name (like Volume, Model) 
  virtual const char* GetNodeTagName() {return "Isodose";};

public:
  /// Get dose volume node
  vtkMRMLScalarVolumeNode* GetDoseVolumeNode();
  /// Set and observe dose volume node
  void SetAndObserveDoseVolumeNode(vtkMRMLScalarVolumeNode* node);

  /// Get isodose surface models parent hierarchy node
  vtkMRMLModelHierarchyNode* GetIsodoseSurfaceModelsParentHierarchyNode();
  /// Set and observe isodose surface models parent hierarchy node
  void SetAndObserveIsodoseSurfaceModelsParentHierarchyNode(vtkMRMLModelHierarchyNode* node);

  /// Get color table node
  vtkMRMLColorTableNode* GetColorTableNode();
  /// Set and observe color table node
  void SetAndObserveColorTableNode(vtkMRMLColorTableNode* node);

  /// Get/Set show Gy for D metrics checkbox state
  vtkGetMacro(ShowIsodoseLines, bool);
  vtkSetMacro(ShowIsodoseLines, bool);
  vtkBooleanMacro(ShowIsodoseLines, bool);

  /// Get/Set Save labelmaps checkbox state
  vtkGetMacro(ShowIsodoseSurfaces, bool);
  vtkSetMacro(ShowIsodoseSurfaces, bool);
  vtkBooleanMacro(ShowIsodoseSurfaces, bool);

  /// Get/Set Save labelmaps checkbox state
  vtkGetMacro(ShowScalarBar, bool);
  vtkSetMacro(ShowScalarBar, bool);
  vtkBooleanMacro(ShowScalarBar, bool);

  /// Get/Set show dose volumes only checkbox state
  vtkGetMacro(ShowDoseVolumesOnly, bool);
  vtkSetMacro(ShowDoseVolumesOnly, bool);
  vtkBooleanMacro(ShowDoseVolumesOnly, bool);

protected:
  vtkMRMLIsodoseNode();
  ~vtkMRMLIsodoseNode();
  vtkMRMLIsodoseNode(const vtkMRMLIsodoseNode&);
  void operator=(const vtkMRMLIsodoseNode&);

protected:
  /// State of Show isodose lines checkbox
  bool ShowIsodoseLines;

  /// State of Show isodose surfaces checkbox
  bool ShowIsodoseSurfaces;

  /// State of Show scalar bar checkbox
  bool ShowScalarBar;

  /// State of Show dose volumes only checkbox
  bool ShowDoseVolumesOnly;
};

#endif
