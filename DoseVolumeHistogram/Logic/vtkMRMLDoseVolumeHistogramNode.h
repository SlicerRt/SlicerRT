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

#ifndef __vtkMRMLDoseVolumeHistogramNode_h
#define __vtkMRMLDoseVolumeHistogramNode_h

// MRML includes
#include <vtkMRML.h>
#include <vtkMRMLNode.h>

// STD includes
#include <vector>
#include <set>

#include "vtkSlicerDoseVolumeHistogramModuleLogicExport.h"

class VTK_SLICER_DOSEVOLUMEHISTOGRAM_MODULE_LOGIC_EXPORT vtkMRMLDoseVolumeHistogramNode : public vtkMRMLNode
{
public:
  static vtkMRMLDoseVolumeHistogramNode *New();
  vtkTypeMacro(vtkMRMLDoseVolumeHistogramNode,vtkMRMLNode);
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
  virtual const char* GetNodeTagName() {return "DoseVolumeHistogram";};

public:
  /// Get/Set dose volume node ID
  vtkGetStringMacro(DoseVolumeNodeId);
  vtkSetStringMacro(DoseVolumeNodeId);

  /// Get/Set structure set node ID
  vtkGetStringMacro(StructureSetModelNodeId);
  vtkSetStringMacro(StructureSetModelNodeId);

  /// Get/Set chart node ID
  vtkGetStringMacro(ChartNodeId);
  vtkSetStringMacro(ChartNodeId);

  /// Get list of all the DVH double array node IDs in the scene
  std::set<std::string>* GetDvhDoubleArrayNodeIds()
  {
    return &this->DvhDoubleArrayNodeIds;
  }

  /// Get/Set Show/Hide all checkbox state
  vtkGetMacro(ShowHideAll, int);
  vtkSetMacro(ShowHideAll, int);

  /// Get show in chart check states
  std::vector<bool>* GetShowInChartCheckStates()
  {
    return &this->ShowInChartCheckStates;
  }

  /// Get/Set input dose values for V metrics
  vtkGetStringMacro(VDoseValues);
  vtkSetStringMacro(VDoseValues);

  /// Get/Set show Cc for V metrics checkbox state
  vtkGetMacro(ShowVMetricsCc, bool);
  vtkSetMacro(ShowVMetricsCc, bool);
  vtkBooleanMacro(ShowVMetricsCc, bool);

  /// Get/Set show % for V metrics checkbox state
  vtkGetMacro(ShowVMetricsPercent, bool);
  vtkSetMacro(ShowVMetricsPercent, bool);
  vtkBooleanMacro(ShowVMetricsPercent, bool);

  /// Get/Set input volume values for D metrics
  vtkGetStringMacro(DVolumeValues);
  vtkSetStringMacro(DVolumeValues);

  /// Get/Set show Gy for D metrics checkbox state
  vtkGetMacro(ShowDMetrics, bool);
  vtkSetMacro(ShowDMetrics, bool);
  vtkBooleanMacro(ShowDMetrics, bool);

  /// Update the stored reference to another node in the scene 
  virtual void UpdateReferenceID(const char *oldID, const char *newID);
 
protected:
  vtkMRMLDoseVolumeHistogramNode();
  ~vtkMRMLDoseVolumeHistogramNode();
  vtkMRMLDoseVolumeHistogramNode(const vtkMRMLDoseVolumeHistogramNode&);
  void operator=(const vtkMRMLDoseVolumeHistogramNode&);

protected:
  /// Selected dose volume MRML node object ID
  char* DoseVolumeNodeId;

  /// Selected structure set MRML node object ID. Can be model node or model hierarchy node
  char* StructureSetModelNodeId;

  /// Selected chart MRML node object ID
  char* ChartNodeId;

  /// Set of all the DVH double array MRML node IDs that are present in the scene
  std::set<std::string> DvhDoubleArrayNodeIds;

  /// State of Show/Hide all checkbox
  int ShowHideAll;

  /// Vector of checkbox states for the case the user makes the show/hide all checkbox state
  /// partially checked. Then the last configuration is restored
  std::vector<bool> ShowInChartCheckStates;

  /// Input dose values for V metrics
  char* VDoseValues;

  /// State of Show Cc for V metrics checkbox
  bool ShowVMetricsCc;

  /// State of Show % for V metrics checkbox
  bool ShowVMetricsPercent;

  /// Input volume values for D metrics
  char* DVolumeValues;

  /// State of Show Gy for D metrics checkbox
  bool ShowDMetrics;
};

#endif
