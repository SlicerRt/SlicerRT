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

// Segmentations includes
#include "vtkMRMLSegmentationNode.h"

// MRML includes
#include <vtkMRMLPlotChartNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLTableNode.h>

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>

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
  this->UseFractionalLabelmap = false;
  this->DoseSurfaceHistogram = 0;
  this->UseInsideDoseSurface = true;

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
  of << " SelectedSegmentIDs=\"";
  for (std::vector<std::string>::iterator it = this->SelectedSegmentIDs.begin(); it != this->SelectedSegmentIDs.end(); ++it)
    {
    of << (*it) << "|";
    }
  of << "\"";

  of << " ShowHideAll=\"" << this->ShowHideAll << "\"";

  of << " VDoseValues=\"" << (this->VDoseValues ? this->VDoseValues : "") << "\"";
  of << " ShowVMetricsCc=\"" << (this->ShowVMetricsCc ? "true" : "false") << "\"";
  of << " ShowVMetricsPercent=\"" << (this->ShowVMetricsPercent ? "true" : "false") << "\"";

  of << " DVolumeValuesCc=\"" << (this->DVolumeValuesCc ? this->DVolumeValuesCc : "") << "\"";
  of << " DVolumeValuesPercent=\"" << (this->DVolumeValuesPercent ? this->DVolumeValuesPercent : "") << "\"";
  of << " ShowDMetrics=\"" << (this->ShowDMetrics ? "true" : "false") << "\"";

  of << " ShowDoseVolumesOnly=\"" << (this->ShowDoseVolumesOnly ? "true" : "false") << "\"";
  of << " AutomaticOversampling=\"" << (this->AutomaticOversampling ? "true" : "false") << "\"";
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
      std::string valueStr(attValue);
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
      this->ShowHideAll = vtkVariant(attValue).ToInt();;
      }
    else if (!strcmp(attName, "VDoseValues")) 
      {
      this->SetVDoseValues(attValue);
      }
    else if (!strcmp(attName, "ShowVMetricsCc")) 
      {
      this->ShowVMetricsCc = (strcmp(attValue,"true") ? false : true);
      }
    else if (!strcmp(attName, "ShowVMetricsPercent")) 
      {
      this->ShowVMetricsPercent = (strcmp(attValue,"true") ? false : true);
      }
    else if (!strcmp(attName, "DVolumeValuesCc")) 
      {
      this->SetDVolumeValuesCc(attValue);
      }
    else if (!strcmp(attName, "DVolumeValuesPercent")) 
      {
      this->SetDVolumeValuesPercent(attValue);
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
  if (node && this->Scene != node->GetScene())
    {
    vtkErrorMacro("Cannot set reference: the referenced and referencing node are not in the same scene");
    return;
    }

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
  if (node && this->Scene != node->GetScene())
    {
    vtkErrorMacro("Cannot set reference: the referenced and referencing node are not in the same scene");
    return;
    }

  this->SetNodeReferenceID(SEGMENTATION_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}

//----------------------------------------------------------------------------
vtkMRMLTableNode* vtkMRMLDoseVolumeHistogramNode::GetMetricsTableNode()
{
  if (!this->Scene)
  {
    vtkErrorMacro("GetMetricsTableNode: Invalid MRML scene");
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
  if (node && this->Scene != node->GetScene())
    {
    vtkErrorMacro("Cannot set reference: the referenced and referencing node are not in the same scene");
    return;
    }

  this->SetNodeReferenceID(DVH_METRICS_TABLE_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}

//----------------------------------------------------------------------------
vtkMRMLPlotChartNode* vtkMRMLDoseVolumeHistogramNode::GetChartNode()
{
  if (!this->Scene)
  {
    vtkErrorMacro("GetChartNode: Invalid MRML scene");
    return NULL;
  }
  vtkMRMLPlotChartNode* chartNode = vtkMRMLPlotChartNode::SafeDownCast( this->GetNodeReference(CHART_REFERENCE_ROLE) );
  // Chart node is unique and mandatory for each DVH node
  if (!chartNode)
  {
    chartNode = vtkMRMLPlotChartNode::New();
    std::string chartNodeName = this->Scene->GenerateUniqueName("DvhChart");
    chartNode->SetName(chartNodeName.c_str());
    chartNode->SetTitleFontSize(16); // Default: 20
    chartNode->SetLegendFontSize(12); // Default: 20
    chartNode->SetAxisTitleFontSize(12); // Default: 16
    chartNode->SetAxisLabelFontSize(10); // Default: 12
    this->Scene->AddNode(chartNode);
    this->SetAndObserveChartNode(chartNode);
    chartNode->Delete(); // Release ownership to scene only
  }
  return chartNode;
}

//----------------------------------------------------------------------------
void vtkMRMLDoseVolumeHistogramNode::SetAndObserveChartNode(vtkMRMLPlotChartNode* node)
{
  if (node && this->Scene != node->GetScene())
    {
    vtkErrorMacro("Cannot set reference: the referenced and referencing node are not in the same scene");
    return;
    }

  this->SetNodeReferenceID(CHART_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}

//----------------------------------------------------------------------------
std::string vtkMRMLDoseVolumeHistogramNode::AssembleDvhNodeReference(std::string segmentID)
{
  if (!this->GetSegmentationNode() || !this->GetDoseVolumeNode() || segmentID.empty())
  {
    vtkErrorMacro("AssembleDvhNodeReference: Invalid input selection");
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

//----------------------------------------------------------------------------
void vtkMRMLDoseVolumeHistogramNode::GetDvhTableNodes(std::vector<vtkMRMLTableNode*> &dvhTableNodes)
{
  dvhTableNodes.clear();

  vtkMRMLTableNode* metricsTableNode = this->GetMetricsTableNode();
  std::vector<std::string> roles;
  metricsTableNode->GetNodeReferenceRoles(roles);
  for (std::vector<std::string>::iterator roleIt=roles.begin(); roleIt!=roles.end(); ++roleIt)
  {
    if ( roleIt->substr(0, vtkMRMLDoseVolumeHistogramNode::DVH_ATTRIBUTE_PREFIX.size()).compare(
      vtkMRMLDoseVolumeHistogramNode::DVH_ATTRIBUTE_PREFIX ) )
    {
      // Not a DVH reference
      continue;
    }

    // Get DVH node
    vtkMRMLTableNode* dvhTableNode = vtkMRMLTableNode::SafeDownCast(metricsTableNode->GetNodeReference(roleIt->c_str()));
    if (!dvhTableNode)
    {
      vtkErrorMacro("GetDvhTableNodes: Metrics table node reference '" << (*roleIt) << "' does not contain DVH node");
      continue;
    }

    dvhTableNodes.push_back(dvhTableNode);
  }
}
