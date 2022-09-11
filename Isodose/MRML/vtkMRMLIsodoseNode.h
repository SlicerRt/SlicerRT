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

  This file was originally developed by Kevin Wang, Princess Margaret Cancer Centre 
  and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/


#ifndef __vtkMRMLIsodoseNode_h
#define __vtkMRMLIsodoseNode_h

// MRML includes
#include <vtkMRMLNode.h>

#include "vtkSlicerIsodoseModuleMRMLExport.h"

class vtkMRMLScalarVolumeNode;
class vtkMRMLModelNode;
class vtkMRMLColorNode;
class vtkMRMLColorTableNode;

/// \ingroup SlicerRt_QtModules_Isodose
class VTK_SLICER_ISODOSE_MODULE_MRML_EXPORT vtkMRMLIsodoseNode : public vtkMRMLNode
{
public:
  enum DoseUnitsType : int { Unknown = -1, Gy = 0, Relative = 1, DoseUnits_Last };
  static const char* COLOR_TABLE_REFERENCE_ROLE;

  static vtkMRMLIsodoseNode *New();
  vtkTypeMacro(vtkMRMLIsodoseNode, vtkMRMLNode);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// Create instance of a GAD node. 
  vtkMRMLNode* CreateNodeInstance() override;

  /// Set node attributes from name/value pairs 
  void ReadXMLAttributes(const char** atts) override;

  /// Write this node's information to a MRML file in XML format. 
  void WriteXML(ostream& of, int indent) override;

  /// Copy the node's attributes to this object 
  void Copy(vtkMRMLNode *node) override;

  /// Get unique node XML tag name (like Volume, Model) 
  const char* GetNodeTagName() override { return "Isodose"; };

public:
  /// Get dose volume node
  vtkMRMLScalarVolumeNode* GetDoseVolumeNode();
  /// Set and observe dose volume node
  void SetAndObserveDoseVolumeNode(vtkMRMLScalarVolumeNode* node);

  /// Get color table node (associated to dose volume node)
  vtkMRMLColorTableNode* GetColorTableNode();
  /// Set and observe color table node (associated to dose volume node)
  void SetAndObserveColorTableNode(vtkMRMLColorTableNode* node);

  /// Get isosurfaces model node
  vtkMRMLModelNode* GetIsosurfacesModelNode();
  /// Set and observe isosurfaces model node
  void SetAndObserveIsosurfacesModelNode(vtkMRMLModelNode* node);

  /// Get/Set show isodose lines checkbox state
  vtkGetMacro(ShowIsodoseLines, bool);
  vtkSetMacro(ShowIsodoseLines, bool);
  vtkBooleanMacro(ShowIsodoseLines, bool);

  /// Get/Set show isodose surfaces checkbox state
  vtkGetMacro(ShowIsodoseSurfaces, bool);
  vtkSetMacro(ShowIsodoseSurfaces, bool);
  vtkBooleanMacro(ShowIsodoseSurfaces, bool);

  /// Get/Set show dose volumes only checkbox state
  vtkGetMacro(ShowDoseVolumesOnly, bool);
  vtkSetMacro(ShowDoseVolumesOnly, bool);
  vtkBooleanMacro(ShowDoseVolumesOnly, bool);

  /// Get/Set reference dose value
  vtkGetMacro(ReferenceDoseValue, double);
  vtkSetMacro(ReferenceDoseValue, double);

  /// Get/Set dose units type
  vtkGetMacro(DoseUnits, DoseUnitsType);
  vtkSetMacro(DoseUnits, DoseUnitsType);

  /// Get/Set relative representation flag
  vtkGetMacro(RelativeRepresentationFlag, bool);
  vtkSetMacro(RelativeRepresentationFlag, bool);
  vtkBooleanMacro(RelativeRepresentationFlag, bool);

protected:
  vtkMRMLIsodoseNode();
  ~vtkMRMLIsodoseNode();
  vtkMRMLIsodoseNode(const vtkMRMLIsodoseNode&);
  void operator=(const vtkMRMLIsodoseNode&);

  static const char* GetDoseUnitsAsString(int id);
  static int GetDoseUnitsFromString(const char* name);
  void SetDoseUnits(int id);

  /// State of Show isodose lines checkbox
  bool ShowIsodoseLines;

  /// State of Show isodose surfaces checkbox
  bool ShowIsodoseSurfaces;

  /// State of Show dose volumes only checkbox
  bool ShowDoseVolumesOnly;

  /// Type of dose units
  DoseUnitsType DoseUnits;

  /// Reference dose value
  double ReferenceDoseValue;

  /// Whether use relative isolevels representation
  /// for absolute dose (Gy) and unknown units or not
  bool RelativeRepresentationFlag;
};

#endif
