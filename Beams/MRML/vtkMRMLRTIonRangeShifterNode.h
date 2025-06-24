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

#ifndef __vtkMRMLRTIonRangeShifterNode_h
#define __vtkMRMLRTIonRangeShifterNode_h

// Beams includes
#include "vtkSlicerBeamsModuleMRMLExport.h"
#include "vtkMRMLRTIonBeamNode.h"

// MRML includes
#include <vtkMRMLModelNode.h>

/// \ingroup SlicerRt_QtModules_Beams
class VTK_SLICER_BEAMS_MODULE_MRML_EXPORT vtkMRMLRTIonRangeShifterNode : public vtkMRMLModelNode
{

public:
  enum RangeShifterType : int {
    ANALOG,
    BINARY,
    RangeShifter_Last
  };
  
  static vtkMRMLRTIonRangeShifterNode *New();
  vtkTypeMacro(vtkMRMLRTIonRangeShifterNode,vtkMRMLModelNode);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// Create instance of a GAD node. 
  vtkMRMLNode* CreateNodeInstance() override;

  /// Set node attributes from name/value pairs 
  void ReadXMLAttributes(const char** atts) override;

  /// Write this node's information to a MRML file in XML format. 
  void WriteXML(ostream& of, int indent) override;

  /// Copy the node's attributes to this object 
  void Copy(vtkMRMLNode *node) override;

  /// Copy node content (excludes basic data, such a name and node reference)
  vtkMRMLCopyContentMacro(vtkMRMLRTIonRangeShifterNode);

  /// Make sure display node and transform node are present and valid
  void SetScene(vtkMRMLScene* scene) override;

  /// Get unique node XML tag name (like Volume, Model) 
  const char* GetNodeTagName() override { return "RTIonRangeShifter"; };

  /// Create and observe default display node
  void CreateDefaultDisplayNodes() override;

  /// Update range shifter poly data based on beam parametes (setting, WET,
  ///  isocenterToRangeShifterDistance)
  /// \param highlightedScanSponRows IntArray of rows. If none given, then nothing to highlight 
  void UpdateGeometry();

public:
  /// Get range shifter number
  vtkGetMacro(Number, int);
  /// Set range shifter number
  vtkSetMacro(Number, int);

  /// Get range shifter ID
  vtkGetStringMacro(RangeShifterID);
  /// Set range shifter ID
  vtkSetStringMacro(RangeShifterID);

  /// Get range shifter type
  vtkGetMacro(Type, RangeShifterType);
  /// Set range shifter type
  vtkSetMacro(Type, RangeShifterType);

  /// Get parent ion beam node
  vtkMRMLRTIonBeamNode* GetParentBeamNode();
  /// Set and observe parent ion beam node
  void SetAndObserveParentBeamNode(vtkMRMLRTIonBeamNode* node);

  vtkGetStringMacro(AccessoryCode);
  vtkSetStringMacro(AccessoryCode);
  vtkGetStringMacro(Description);
  vtkSetStringMacro(Description);
  vtkGetStringMacro(MaterialID);
  vtkSetStringMacro(MaterialID);
  vtkGetMacro(MaterialDensity, double);
  vtkSetMacro(MaterialDensity, double);

protected:
  /// Create range shifter model from range shifter parameters and
  /// range shifter settings from ion beam data
  /// \param rangeShifterModelPolyData Output polydata. If none given then the range shifter node's own polydata is used
  virtual void CreateRangeShifterPolyData(vtkPolyData* rangeShifterModelPolyData=nullptr);

protected:
  vtkMRMLRTIonRangeShifterNode();
  ~vtkMRMLRTIonRangeShifterNode();
  vtkMRMLRTIonRangeShifterNode(const vtkMRMLRTIonRangeShifterNode&);
  void operator=(const vtkMRMLRTIonRangeShifterNode&);

  static const char* GetTypeAsString(int id);
  static int GetTypeFromString(const char* name);
  void SetType(int id);

  // Range shifter properties
  int Number{ -1 };
  char* RangeShifterID{ nullptr };
  char* AccessoryCode{ nullptr };
  char* Description{ nullptr };
  RangeShifterType Type{ vtkMRMLRTIonRangeShifterNode::RangeShifter_Last };
  char* MaterialID{ nullptr };
  double MaterialDensity{ -1. };
};

#endif // __vtkMRMLRTIonRangeShifterNode_h
