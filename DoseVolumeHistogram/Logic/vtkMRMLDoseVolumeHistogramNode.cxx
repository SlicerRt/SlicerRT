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

// vtkMRMLDoseVolumeHistogram includes
#include "vtkMRMLDoseVolumeHistogramNode.h"

// SlicerRT includes
#include "vtkSlicerDoseVolumeHistogramModuleLogicExport.h"

// Segmentations includes
#include "vtkMRMLSegmentationNode.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLChartNode.h>
#include <vtkMRMLDoubleArrayNode.h>
#include <vtkMRMLTableNode.h>

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkTable.h>

// STD includes
#include <sstream>

//------------------------------------------------------------------------------
const char* vtkMRMLDoseVolumeHistogramNode::DOSE_VOLUME_REFERENCE_ROLE = "doseVolumeRef";
const char* vtkMRMLDoseVolumeHistogramNode::SEGMENTATION_REFERENCE_ROLE = "segmentationRef";
const char* vtkMRMLDoseVolumeHistogramNode::DVH_METRICS_TABLE_REFERENCE_ROLE = "dvhMetricsTableRef";
static const char* CHART_REFERENCE_ROLE = "chartRef";

const std::string vtkMRMLDoseVolumeHistogramNode::DVH_ATTRIBUTE_PREFIX = "DoseVolumeHistogram.";

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLDoseVolumeHistogramNode);

//----------------------------------------------------------------------------
vtkMRMLDoseVolumeHistogramNode::vtkMRMLDoseVolumeHistogramNode()
{
  this->SelectedSegmentIDs.clear();
  this->ShowHideAll = 0;
  this->VDoseValues = NULL;
  this->ShowVMetricsCc = false;
  this->ShowVMetricsPercent = false;
  this->DVolumeValuesCc = NULL;
  this->DVolumeValuesPercent = NULL;
  this->ShowDMetrics = false;
  this->ShowDoseVolumesOnly = true;
  this->AutomaticOversampling = false;
  this->AutomaticOversamplingFactors.clear();

  this->HideFromEditors = false;
}

//----------------------------------------------------------------------------
vtkMRMLDoseVolumeHistogramNode::~vtkMRMLDoseVolumeHistogramNode()
{
  this->SelectedSegmentIDs.clear();
  this->SetVDoseValues(NULL);
  this->SetDVolumeValuesCc(NULL);
  this->SetDVolumeValuesPercent(NULL);
  this->AutomaticOversamplingFactors.clear();
}

//----------------------------------------------------------------------------
void vtkMRMLDoseVolumeHistogramNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);

  // Write all MRML node attributes into output stream
  vtkIndent indent(nIndent);

  of << indent << " SelectedSegmentIDs=\"";
  for (std::vector<std::string>::iterator it = this->SelectedSegmentIDs.begin(); it != this->SelectedSegmentIDs.end(); ++it)
    {
    of << (*it) << "|";
    }
  of << "\"";

  {
    std::stringstream ss;
    ss << this->ShowHideAll;
    of << indent << " ShowHideAll=\"" << ss.str() << "\"";
  }

  {
    std::stringstream ss;
    if ( this->VDoseValues )
      {
      ss << this->VDoseValues;
      of << indent << " VDoseValues=\"" << ss.str() << "\"";
      }
  }

  of << indent << " ShowVMetricsCc=\"" << (this->ShowVMetricsCc ? "true" : "false") << "\"";

  of << indent << " ShowVMetricsPercent=\"" << (this->ShowVMetricsPercent ? "true" : "false") << "\"";

  {
    std::stringstream ss;
    if ( this->DVolumeValuesCc )
      {
      ss << this->DVolumeValuesCc;
      of << indent << " DVolumeValuesCc=\"" << ss.str() << "\"";
      }
  }

  {
    std::stringstream ss;
    if ( this->DVolumeValuesPercent )
      {
      ss << this->DVolumeValuesPercent;
      of << indent << " DVolumeValuesPercent=\"" << ss.str() << "\"";
      }
  }

  of << indent << " ShowDMetrics=\"" << (this->ShowDMetrics ? "true" : "false") << "\"";

  of << indent << " ShowDoseVolumesOnly=\"" << (this->ShowDoseVolumesOnly ? "true" : "false") << "\"";

  of << indent << " AutomaticOversampling=\"" << (this->AutomaticOversampling ? "true" : "false") << "\"";
}

//----------------------------------------------------------------------------
void vtkMRMLDoseVolumeHistogramNode::ReadXMLAttributes(const char** atts)
{
  vtkMRMLNode::ReadXMLAttributes(atts);

  // Read all MRML node attributes from two arrays of names and values
  const char* attName = NULL;
  const char* attValue = NULL;

  while (*atts != NULL) 
    {
    attName = *(atts++);
    attValue = *(atts++);

    if (!strcmp(attName, "SelectedSegmentIDs")) 
      {
      std::stringstream ss;
      ss << attValue;
      std::string valueStr = ss.str();
      std::string separatorCharacter("|");

      this->SelectedSegmentIDs.clear();
      size_t separatorPosition = valueStr.find( separatorCharacter );
      while (separatorPosition != std::string::npos)
        {
        this->SelectedSegmentIDs.push_back(valueStr.substr(0, separatorPosition));
        valueStr = valueStr.substr( separatorPosition+1 );
        separatorPosition = valueStr.find( separatorCharacter );
        }
      if (!valueStr.empty())
        {
        this->SelectedSegmentIDs.push_back(valueStr);
        }
      }
    else if (!strcmp(attName, "ShowHideAll")) 
      {
      std::stringstream ss;
      ss << attValue;
      int intAttValue;
      ss >> intAttValue;
      this->ShowHideAll = intAttValue;
      }
    else if (!strcmp(attName, "VDoseValues")) 
      {
      std::stringstream ss;
      ss << attValue;
      this->SetVDoseValues(ss.str().c_str());
      }
    else if (!strcmp(attName, "ShowVMetricsCc")) 
      {
      this->ShowVMetricsCc = 
        (strcmp(attValue,"true") ? false : true);
      }
    else if (!strcmp(attName, "ShowVMetricsPercent")) 
      {
      this->ShowVMetricsPercent = 
        (strcmp(attValue,"true") ? false : true);
      }
    else if (!strcmp(attName, "DVolumeValuesCc")) 
      {
      std::stringstream ss;
      ss << attValue;
      this->SetDVolumeValuesCc(ss.str().c_str());
      }
    else if (!strcmp(attName, "DVolumeValuesPercent")) 
      {
      std::stringstream ss;
      ss << attValue;
      this->SetDVolumeValuesPercent(ss.str().c_str());
      }
    else if (!strcmp(attName, "ShowDMetrics")) 
      {
      this->ShowDMetrics = (strcmp(attValue,"true") ? false : true);
      }
    else if (!strcmp(attName, "ShowDoseVolumesOnly")) 
      {
      this->ShowDoseVolumesOnly = (strcmp(attValue,"true") ? false : true);
      }
    else if (!strcmp(attName, "AutomaticOversampling")) 
      {
      this->AutomaticOversampling = (strcmp(attValue,"true") ? false : true);
      }
    }
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLDoseVolumeHistogramNode::Copy(vtkMRMLNode *anode)
{
  Superclass::Copy(anode);
  this->DisableModifiedEventOn();

  vtkMRMLDoseVolumeHistogramNode *node = (vtkMRMLDoseVolumeHistogramNode *) anode;

  this->SelectedSegmentIDs = node->SelectedSegmentIDs;
  this->ShowHideAll = node->ShowHideAll;

  this->SetVDoseValues(node->VDoseValues);
  this->ShowVMetricsCc = node->ShowVMetricsCc;
  this->ShowVMetricsPercent = node->ShowVMetricsPercent;

  this->SetDVolumeValuesCc(node->DVolumeValuesCc);
  this->SetDVolumeValuesPercent(node->DVolumeValuesPercent);
  this->ShowDMetrics = node->ShowDMetrics;
  this->ShowDoseVolumesOnly = node->ShowDoseVolumesOnly;
  this->AutomaticOversampling = node->AutomaticOversampling;

  this->DisableModifiedEventOff();
  this->InvokePendingModifiedEvent();
}

//----------------------------------------------------------------------------
void vtkMRMLDoseVolumeHistogramNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);

  os << indent << " SelectedSegmentIDs:   ";
  for (std::vector<std::string>::iterator it = this->SelectedSegmentIDs.begin(); it != this->SelectedSegmentIDs.end(); ++it)
    {
    os << indent << (*it) << "|";
    }
  os << "\n";

  os << indent << "ShowHideAll:   " << this->ShowHideAll << "\n";

  os << indent << "VDoseValues:   " << (this->VDoseValues ? this->VDoseValues : "") << "\n";
  os << indent << "ShowVMetricsCc:   " << (this->ShowVMetricsCc ? "true" : "false") << "\n";
  os << indent << "ShowVMetricsPercent:   " << (this->ShowVMetricsPercent ? "true" : "false") << "\n";

  os << indent << "DVolumeValuesCc:   " << (this->DVolumeValuesCc ? this->DVolumeValuesCc : "") << "\n";
  os << indent << "DVolumeValuesPercent:   " << (this->DVolumeValuesPercent ? this->DVolumeValuesPercent : "") << "\n";
  os << indent << "ShowDMetrics:   " << (this->ShowDMetrics ? "true" : "false") << "\n";
  os << indent << "ShowDoseVolumesOnly:   " << (this->ShowDoseVolumesOnly ? "true" : "false") << "\n";
  os << indent << "AutomaticOversampling:   " << (this->AutomaticOversampling ? "true" : "false") << "\n";
}

//----------------------------------------------------------------------------
vtkMRMLScalarVolumeNode* vtkMRMLDoseVolumeHistogramNode::GetDoseVolumeNode()
{
  return vtkMRMLScalarVolumeNode::SafeDownCast( this->GetNodeReference(DOSE_VOLUME_REFERENCE_ROLE) );
}

//----------------------------------------------------------------------------
void vtkMRMLDoseVolumeHistogramNode::SetAndObserveDoseVolumeNode(vtkMRMLScalarVolumeNode* node)
{
  this->SetNodeReferenceID(DOSE_VOLUME_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}

//----------------------------------------------------------------------------
vtkMRMLSegmentationNode* vtkMRMLDoseVolumeHistogramNode::GetSegmentationNode()
{
  return vtkMRMLSegmentationNode::SafeDownCast( this->GetNodeReference(SEGMENTATION_REFERENCE_ROLE) );
}

//----------------------------------------------------------------------------
void vtkMRMLDoseVolumeHistogramNode::SetAndObserveSegmentationNode(vtkMRMLSegmentationNode* node)
{
  this->SetNodeReferenceID(SEGMENTATION_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}

//----------------------------------------------------------------------------
vtkMRMLTableNode* vtkMRMLDoseVolumeHistogramNode::GetMetricsTableNode()
{
  if (!this->Scene)
  {
    vtkErrorMacro("GetMetricsTableNode: Invalid MRML scene!");
    return NULL;
  }
  vtkMRMLTableNode* metricsTableNode = vtkMRMLTableNode::SafeDownCast( this->GetNodeReference(DVH_METRICS_TABLE_REFERENCE_ROLE) );
  // Metrics table node is unique and mandatory for each DVH node
  if (!metricsTableNode)
  {
    metricsTableNode = vtkMRMLTableNode::New();
    std::string metricsTableNodeName = this->Scene->GenerateUniqueName("DvhMetrics");
    metricsTableNode->SetName(metricsTableNodeName.c_str());
    this->Scene->AddNode(metricsTableNode);
    this->SetAndObserveMetricsTableNode(metricsTableNode);
    metricsTableNode->Delete(); // Release ownership to scene only
  }
  return metricsTableNode;

}

//----------------------------------------------------------------------------
void vtkMRMLDoseVolumeHistogramNode::SetAndObserveMetricsTableNode(vtkMRMLTableNode* node)
{
  this->SetNodeReferenceID(DVH_METRICS_TABLE_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}

//----------------------------------------------------------------------------
vtkMRMLChartNode* vtkMRMLDoseVolumeHistogramNode::GetChartNode()
{
  if (!this->Scene)
  {
    vtkErrorMacro("GetChartNode: Invalid MRML scene!");
    return NULL;
  }
  vtkMRMLChartNode* chartNode = vtkMRMLChartNode::SafeDownCast( this->GetNodeReference(CHART_REFERENCE_ROLE) );
  // Chart node is unique and mandatory for each DVH node
  if (!chartNode)
  {
    chartNode = vtkMRMLChartNode::New();
    std::string chartNodeName = this->Scene->GenerateUniqueName("DvhChart");
    chartNode->SetName(chartNodeName.c_str());
    this->Scene->AddNode(chartNode);
    this->SetAndObserveChartNode(chartNode);
    chartNode->Delete(); // Release ownership to scene only
  }
  return chartNode;
}

//----------------------------------------------------------------------------
void vtkMRMLDoseVolumeHistogramNode::SetAndObserveChartNode(vtkMRMLChartNode* node)
{
  this->SetNodeReferenceID(CHART_REFERENCE_ROLE, (node ? node->GetID() : NULL));

  // Set all visibility selection to false when chart node is changed
  // TODO: It could be better to actually set visibility according to contained DVH arrays in the selected chart view
  vtkMRMLTableNode* metricsTableNode = this->GetMetricsTableNode();
  if (metricsTableNode)
  {
    for (int row=0; row<metricsTableNode->GetNumberOfRows(); ++row)
    {
      metricsTableNode->GetTable()->SetValue(row, MetricColumnVisible, vtkVariant(0));
    }
    metricsTableNode->Modified();
  }
}

//----------------------------------------------------------------------------
std::string vtkMRMLDoseVolumeHistogramNode::AssembleDvhNodeReference(std::string segmentID)
{
  if (!this->GetSegmentationNode() || !this->GetDoseVolumeNode() || segmentID.empty())
  {
    vtkErrorMacro("AssembleDvhNodeReference: Invalid input selection!");
    return "";
  }
  
  std::string referenceRole = DVH_ATTRIBUTE_PREFIX;
  referenceRole.append(this->GetDoseVolumeNode()->GetID());
  referenceRole.append("_");
  referenceRole.append(this->GetSegmentationNode()->GetID());
  referenceRole.append("_");
  referenceRole.append(segmentID);
  return referenceRole;
}
