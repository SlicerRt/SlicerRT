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

// SlicerRtCommon includes
#include "SlicerRtCommon.h"
#include "vtkLabelmapToModelFilter.h"
#include "vtkPolyDataToLabelmapFilter.h"

// SubjectHierarchy includes
#include "vtkMRMLSubjectHierarchyNode.h"
#include "vtkMRMLSubjectHierarchyConstants.h"
#include "vtkSlicerSubjectHierarchyModuleLogic.h"

// Contours includes
#include "vtkConvertContourRepresentations.h"
#include "vtkMRMLContourModelDisplayNode.h"
#include "vtkMRMLContourStorageNode.h"

// MRML includes
#include <vtkMRMLColorTableNode.h>
#include <vtkMRMLDisplayNode.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLTransformNode.h>

// VTK includes
#include <vtkAlgorithmOutput.h>
#include <vtkCollection.h>
#include <vtkEventBroker.h>
#include <vtkGeneralTransform.h>
#include <vtkImageData.h>
#include <vtkImageReslice.h>
#include <vtkImageResliceMask.h>
#include <vtkIntArray.h>
#include <vtkMath.h>
#include <vtkMathUtilities.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPlane.h>
#include <vtkPlaneCollection.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkTrivialProducer.h>
#include <vtksys/SystemTools.hxx>

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLContourNode);

//------------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkMRMLContourNode, RibbonModelPolyData, vtkPolyData);
vtkCxxSetObjectMacro(vtkMRMLContourNode, ClosedSurfacePolyData, vtkPolyData);
vtkCxxSetObjectMacro(vtkMRMLContourNode, LabelmapImageData, vtkImageData);

//----------------------------------------------------------------------------
vtkMRMLContourNode::vtkMRMLContourNode()
: RibbonModelPolyData(NULL)
, ClosedSurfacePolyData(NULL)
, LabelmapImageData(NULL)
, DicomRtRoiPoints(NULL)
, RasterizationOversamplingFactor(-1.0)
, DecimationTargetReductionFactor(-1.0)
, CreatedFromIndexLabelmap(false)
{
  this->OrderedContourPlanes.clear();

  this->HideFromEditorsOff();

  // Register parent transform modified event so that the representations
  //   can be put under the same transform node
  vtkSmartPointer<vtkIntArray> events = vtkSmartPointer<vtkIntArray>::New();
  events->InsertNextValue(vtkMRMLTransformableNode::TransformModifiedEvent);
  vtkObserveMRMLObjectEventsMacro(this, events);

  int i,j;

  for(i=0; i<3; i++)
  {
    for(j=0; j<3; j++)
    {
      this->IJKToRASDirections[i][j] = (i == j) ? 1.0 : 0.0;
    }
  }

  for(i=0; i<3; i++)
  {
    this->Spacing[i] = 1.0;
  }

  for(i=0; i<3; i++)
  {
    this->Origin[i] = 0.0;
  }
}

//----------------------------------------------------------------------------
vtkMRMLContourNode::~vtkMRMLContourNode()
{
  this->SetAndObserveRibbonModelPolyData(NULL);
  this->SetAndObserveClosedSurfacePolyData(NULL);
  this->SetAndObserveLabelmapImageData(NULL);
  this->SetDicomRtRoiPoints(NULL);

  this->OrderedContourPlanes.clear();
}

//----------------------------------------------------------------------------
void vtkMRMLContourNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);

  // Write all MRML node attributes into output stream
  vtkIndent indent(nIndent);

  std::stringstream ss;
  for(int i=0; i<3; i++)
  {
    for(int j=0; j<3; j++)
    {
      ss << this->IJKToRASDirections[i][j] << " ";
      if ( i != 2 && j != 2 )
      {
        ss << "  ";
      }
    }
  }
  of << indent << " ijkToRASDirections=\"" << ss.str() << "\"";

  of << indent << " spacing=\""
    << this->Spacing[0] << " " << this->Spacing[1] << " " << this->Spacing[2] << "\"";

  of << indent << " origin=\""
    << this->Origin[0] << " " << this->Origin[1] << " " << this->Origin[2] << "\"";

  of << indent << " RasterizationOversamplingFactor=\"" << this->RasterizationOversamplingFactor << "\"";
  of << indent << " DecimationTargetReductionFactor=\"" << this->DecimationTargetReductionFactor << "\"";
  of << indent << " CreatedFromLabelmap=\"" << (this->CreatedFromIndexLabelmap ? "TRUE" : "FALSE") << "\"";
}

//----------------------------------------------------------------------------
void vtkMRMLContourNode::ReadXMLAttributes(const char** atts)
{
  // Read all MRML node attributes from two arrays of names and values
  int disabledModify = this->StartModify();

  Superclass::ReadXMLAttributes(atts);

  const char* attName;
  const char* attValue;
  while (*atts != NULL)
  {
    attName = *(atts++);
    attValue = *(atts++);

    if (!strcmp(attName, "ijkToRASDirections"))
    {
      std::stringstream ss;
      double val;
      ss << attValue;
      double dirs[3][3];
      for(int i=0; i<3; i++)
      {
        for(int j=0; j<3; j++)
        {
          ss >> val;
          dirs[i][j] = val;
        }
      }
      this->SetIJKToRASDirections(dirs);
    }
    else if (!strcmp(attName, "spacing"))
    {
      std::stringstream ss;
      double val;
      double spacing[3];
      ss << attValue;
      for(int i=0; i<3; i++)
      {
        ss >> val;
        spacing[i] = val;
      }
      this->SetSpacing(spacing);
    }
    else if (!strcmp(attName, "CreatedFromLabelmap"))
    {
      std::stringstream ss;
      bool val = (strcmp("TRUE", attValue) == 0 ? true : false);
      this->SetCreatedFromIndexLabelmap(val);
    }
    else if (!strcmp(attName, "origin"))
    {
      std::stringstream ss;
      double val;
      double origin[3];
      ss << attValue;
      for(int i=0; i<3; i++)
      {
        ss >> val;
        origin[i] = val;
      }
      this->SetOrigin(origin);
    }
    else if (!strcmp(attName, "RasterizationReferenceVolumeNodeId")) 
    {
      this->SetAndObserveRasterizationReferenceVolumeNodeId(attValue);
    }
    else if (!strcmp(attName, "RasterizationOversamplingFactor")) 
    {
      std::stringstream ss;
      ss << attValue;
      double doubleAttValue;
      ss >> doubleAttValue;
      this->RasterizationOversamplingFactor = doubleAttValue;
    }
    else if (!strcmp(attName, "DecimationTargetReductionFactor")) 
    {
      std::stringstream ss;
      ss << attValue;
      double doubleAttValue;
      ss >> doubleAttValue;
      this->DecimationTargetReductionFactor = doubleAttValue;
    }
  }

  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLContourNode::Copy(vtkMRMLNode *anode)
{
  this->DisableModifiedEventOn();

  Superclass::Copy(anode);

  vtkMRMLContourNode *otherNode = vtkMRMLContourNode::SafeDownCast(anode);

  // Observers must be removed here, otherwise MRML updates would activate nodes on the undo stack
  if (otherNode && otherNode->HasRepresentation(RibbonModel))
  {
    this->SetAndObserveRibbonModelPolyData(otherNode->GetRibbonModelPolyData());
  }
  if (otherNode && otherNode->HasRepresentation(ClosedSurfaceModel))
  {
    this->SetAndObserveClosedSurfacePolyData(otherNode->GetClosedSurfacePolyData());
  }
  if (otherNode && otherNode->HasRepresentation(IndexedLabelmap))
  {
    this->SetAndObserveLabelmapImageData( otherNode->GetLabelmapImageData() );
  }

  if (otherNode->GetAddToScene())
  {
    this->CopyOrientation(otherNode);
  }

  this->SetCreatedFromIndexLabelmap(otherNode->HasBeenCreatedFromIndexedLabelmap());

  this->SetDicomRtRoiPoints( otherNode->GetDicomRtRoiPoints() );

  this->SetRasterizationOversamplingFactor( otherNode->GetRasterizationOversamplingFactor() );
  this->SetDecimationTargetReductionFactor( otherNode->GetDecimationTargetReductionFactor() );

  this->DisableModifiedEventOff();
  this->InvokePendingModifiedEvent();
}

//----------------------------------------------------------------------------
void vtkMRMLContourNode::DeepCopy(vtkMRMLNode* aNode)
{
  this->DisableModifiedEventOn();

  Superclass::Copy(aNode);

  vtkMRMLContourNode *otherNode = vtkMRMLContourNode::SafeDownCast(aNode);

  if (otherNode && otherNode->HasRepresentation(RibbonModel))
  {
    this->SetAndObserveRibbonModelPolyData(vtkSmartPointer<vtkPolyData>::New());
    this->RibbonModelPolyData->DeepCopy(otherNode->GetRibbonModelPolyData());
  }
  if (otherNode && otherNode->HasRepresentation(ClosedSurfaceModel))
  {
    this->SetAndObserveClosedSurfacePolyData(vtkSmartPointer<vtkPolyData>::New());
    this->ClosedSurfacePolyData->DeepCopy(otherNode->GetClosedSurfacePolyData());
  }
  if (otherNode && otherNode->HasRepresentation(IndexedLabelmap))
  {
    this->SetAndObserveLabelmapImageData(vtkSmartPointer<vtkImageData>::New());
    this->LabelmapImageData->DeepCopy(otherNode->GetLabelmapImageData());
  }

  if (otherNode->GetAddToScene())
  {
    this->CopyOrientation(otherNode);
  }

  this->SetCreatedFromIndexLabelmap(otherNode->HasBeenCreatedFromIndexedLabelmap());

  this->SetDicomRtRoiPoints( vtkSmartPointer<vtkPolyData>::New() );
  this->DicomRtRoiPoints->DeepCopy(otherNode->GetDicomRtRoiPoints());

  this->SetRasterizationOversamplingFactor( otherNode->GetRasterizationOversamplingFactor() );
  this->SetDecimationTargetReductionFactor( otherNode->GetDecimationTargetReductionFactor() );

  this->DisableModifiedEventOff();
  this->InvokePendingModifiedEvent();
}

//----------------------------------------------------------------------------
void vtkMRMLContourNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);
  // Matrices
  os << "IJKToRASDirections:\n";

  int i,j;

  for(i=0; i<3; i++)
  {
    for(j=0; j<3; j++)
    {
      os << indent << " " << this->IJKToRASDirections[i][j];
    }
    os << indent << "\n";
  }
  os << "\n";

  os << "Origin:";
  for(j=0; j<3; j++)
  {
    os << indent << " " << this->Origin[j];
  }
  os << "\n";
  os << "Spacing:";
  for(j=0; j<3; j++)
  {
    os << indent << " " << this->Spacing[j];
  }
  os << "\n";

  if (this->LabelmapImageData != NULL)
  {
    os << indent << "ImageData:\n";
    this->LabelmapImageData->PrintSelf(os, indent.GetNextIndent());
  }

  if( this->RibbonModelPolyData != NULL )
  {
    this->RibbonModelPolyData->PrintSelf(os, indent);
  }
  if( this->ClosedSurfacePolyData != NULL )
  {
    this->ClosedSurfacePolyData->PrintSelf(os, indent);
  }
  os << indent << "RasterizationOversamplingFactor:   " << this->RasterizationOversamplingFactor << std::endl;
  os << indent << "DecimationTargetReductionFactor:   " << this->DecimationTargetReductionFactor << std::endl;
  os << indent << "CreatedFromLabelmap:   " << (this->CreatedFromIndexLabelmap ? "TRUE" : "FALSE") << std::endl;
}

//----------------------------------------------------------------------------
void vtkMRMLContourNode::ProcessMRMLEvents(vtkObject *caller, unsigned long eventID, void *callData)
{
  Superclass::ProcessMRMLEvents(caller, eventID, callData);

  if (!this->Scene)
  {
    vtkErrorMacro("ProcessMRMLEvents: Invalid MRML scene!");
    return;
  }
  if (this->Scene->IsBatchProcessing())
  {
    return;
  }

  if (eventID == vtkCommand::ModifiedEvent)
  {
    /// TODO : if data is modified, other representations need to be changed/deleted accordingly
    ///  if reference node is larger than 1, update other representations
    this->StorableModifiedTime.Modified();
    if( vtkPolyData::SafeDownCast(caller) != NULL )
    {
      if( caller == this->RibbonModelPolyData )
      {
        this->InvokeEvent(vtkMRMLContourNode::RibbonModelPolyDataModifiedEvent, NULL);
        // Invalidate labelmap and closed surface model
        this->SetAndObserveLabelmapImageData(NULL);
        this->SetAndObserveClosedSurfacePolyData(NULL);
      }
      else
      {
        this->InvokeEvent(vtkMRMLContourNode::ClosedSurfacePolyDataModifiedEvent, NULL);
      }
    }
    else if( vtkImageData::SafeDownCast(caller) != NULL )
    {
      this->InvokeEvent(vtkMRMLContourNode::LabelmapImageDataModifiedEvent, NULL);
      // TODO : what to do with the ribbon model? the dicom rt roi points?
      this->SetAndObserveClosedSurfacePolyData(NULL);
    }
  }
  else if (eventID == vtkMRMLTransformableNode::TransformModifiedEvent)
  {
    // Parent transform changed (of this contour node or one of the representations)
    vtkMRMLTransformableNode* callerNode = vtkMRMLTransformableNode::SafeDownCast(caller);
    if ( !callerNode || !caller->IsA("vtkMRMLContourNode") )
    {
      return;
    }

    const char* newTransformNodeId = callerNode->GetTransformNodeID();

    // Set the parent transform to this contour node
    if ( ( (!this->GetTransformNodeID() || !newTransformNodeId) && (this->GetTransformNodeID() != newTransformNodeId) )
      || ( this->GetTransformNodeID() && newTransformNodeId && STRCASECMP(this->GetTransformNodeID(), newTransformNodeId) ) )
    {
      this->SetAndObserveTransformNodeID(newTransformNodeId);
    }
  }
}

//----------------------------------------------------------------------------
void vtkMRMLContourNode::SetAndObserveRasterizationReferenceVolumeNodeId(const char* id)
{
  if ( this->GetNodeReference(SlicerRtCommon::CONTOUR_RASTERIZATION_VOLUME_REFERENCE_ROLE.c_str()) != NULL &&
    strcmp(this->GetNodeReference(SlicerRtCommon::CONTOUR_RASTERIZATION_VOLUME_REFERENCE_ROLE.c_str())->GetID(), id) == 0 )
  {
    // The same reference volume is to be set as the current one - no action necessary
    return;
  }
  if ( this->HasBeenCreatedFromIndexedLabelmap() )
  {
    // If the contour has been created from a labelmap, then another reference volume cannot be set
    // (especially that the reference being the labelmap itself indicates the fact that it has been created from labelmap)
    vtkWarningMacro("SetAndObserveRasterizationReferenceVolumeNodeId: Cannot set rasterization reference volume to a contour that has been created from an indexed labelmap");
    return;
  }

  // Invalidate indexed labelmap representation if it exists and rasterization reference volume has changed (from a value other than the default invalid value),
  // because it is assumed that the current reference volume was used when creating the indexed labelmap, and allowing a reference volume change without
  // invalidating the labelmap would introduce inconsistency.
  if (this->LabelmapImageData && this->GetNodeReference(SlicerRtCommon::CONTOUR_RASTERIZATION_VOLUME_REFERENCE_ROLE.c_str()) != NULL)
  {
    vtkWarningMacro("SetAndObserveRasterizationReferenceVolumeNodeId: Invalidating current indexed labelmap as the rasterization reference volume has been explicitly changed!");

    // Invalidate representation
    this->SetAndObserveLabelmapImageData(NULL);
  }

  this->SetAndObserveNodeReferenceID(SlicerRtCommon::CONTOUR_RASTERIZATION_VOLUME_REFERENCE_ROLE.c_str(), id);
}

//----------------------------------------------------------------------------
void vtkMRMLContourNode::SetRasterizationOversamplingFactor(double oversamplingFactor)
{
  if (this->RasterizationOversamplingFactor == oversamplingFactor)
  {
    // The oversampling factor is to be set as the current one - no action necessary
    return;
  }
  if (this->HasBeenCreatedFromIndexedLabelmap()) 
  {
    // If the contour has been created from a labelmap, then another oversampling factor cannot be set
    vtkWarningMacro("SetAndObserveRasterizationReferenceVolumeNodeId: Cannot set rasterization oversampling factor to a contour that has been created from an indexed labelmap");
    return;
  }

  // Invalidate indexed labelmap representation if it exists and rasterization oversampling factor has changed (from a value other than the default invalid value),
  // because it is assumed that the current oversampling factor was used when creating the indexed labelmap, and allowing an oversampling factor change without
  // invalidating the labelmap would introduce inconsistency.
  if (this->LabelmapImageData && this->RasterizationOversamplingFactor != -1)
  {
    vtkWarningMacro("SetRasterizationOversamplingFactor: Invalidating current indexed labelmap as the rasterization oversampling factor has been explicitly changed!");

    // Invalidate representation
    this->SetAndObserveLabelmapImageData(NULL);
  }

  this->RasterizationOversamplingFactor = oversamplingFactor;
}

//----------------------------------------------------------------------------
void vtkMRMLContourNode::SetDecimationTargetReductionFactor(double targetReductionFactor)
{
  if (this->DecimationTargetReductionFactor == targetReductionFactor)
  {
    // The target reduction factor is to be set as the current one - no action necessary
    return;
  }

  // Invalidate closed surface model representation if it exists and decimation target reduction factor has changed (from a value other than the default invalid value),
  // because it is assumed that the current target reduction factor was used when creating the closed surface model, and allowing a target reduction factor change
  // without invalidating the surface model would introduce inconsistency.
  if (this->ClosedSurfacePolyData && this->DecimationTargetReductionFactor != -1)
  {
    vtkWarningMacro("SetDecimationTargetReductionFactor: Invalidating current closed surface model as the decimation target reduction factor has been explicitly changed!");

    // Invalidate representation
    this->SetAndObserveClosedSurfacePolyData(NULL);
  }

  this->DecimationTargetReductionFactor = targetReductionFactor;
}

//----------------------------------------------------------------------------
void vtkMRMLContourNode::SetOrderedContourPlanes( std::map<double, vtkSmartPointer<vtkPlane> >& orderedContourPlanes )
{
  this->OrderedContourPlanes = orderedContourPlanes;
}

//----------------------------------------------------------------------------
const std::map<double, vtkSmartPointer<vtkPlane> >& vtkMRMLContourNode::GetOrderedContourPlanes() const
{
  return this->OrderedContourPlanes;
}

//----------------------------------------------------------------------------
vtkPlaneCollection* vtkMRMLContourNode::GetOrderedContourPlanesVtk() const
{
  vtkPlaneCollection* planeCollection = vtkPlaneCollection::New();
  std::map<double, vtkSmartPointer<vtkPlane> >::const_iterator planesIt;
  for (planesIt = this->OrderedContourPlanes.begin(); planesIt != this->OrderedContourPlanes.end(); ++planesIt)
  {
    planeCollection->AddItem(planesIt->second.GetPointer());
  }
  return planeCollection;
}

//----------------------------------------------------------------------------
void vtkMRMLContourNode::ShowRepresentation(ContourRepresentationType representationType, bool show)
{
  vtkMRMLDisplayNode* displayNode(NULL);
  for( int i = 0; i < this->GetNumberOfDisplayNodes(); ++i )
  {
    displayNode = this->GetNthDisplayNode(i);
    if( vtkMRMLContourModelDisplayNode::SafeDownCast(displayNode)  != NULL )
    {
      if( vtkMRMLContourModelDisplayNode::SafeDownCast(displayNode)->GetInputPolyData() == this->RibbonModelPolyData && representationType == RibbonModel )
      {
        break;
      }
      else if( vtkMRMLContourModelDisplayNode::SafeDownCast(displayNode)->GetInputPolyData() == this->ClosedSurfacePolyData && representationType == ClosedSurfaceModel )
      {
        break;
      }
    }
    // TODO : when 2d visualization is added back in, control it here
  }

  if( displayNode != NULL )
  {
    displayNode->SetVisibility(show?1:0);
  }
  else
  {
    vtkErrorMacro("Can't show representation of type <" << vtkMRMLContourNode::GetRepresentationTypeAsString(representationType) << ">. Node Id: " << this->GetID());
  }
}

//----------------------------------------------------------------------------
bool vtkMRMLContourNode::HasRepresentation( ContourRepresentationType type )
{
  switch (type)
  {
  case RibbonModel:
    return this->RibbonModelPolyData != NULL ? true : false;
  case IndexedLabelmap:
    return this->LabelmapImageData != NULL ? true : false;
  case ClosedSurfaceModel:
    return this->ClosedSurfacePolyData != NULL ? true : false;
  default:
    return false;
  }

  return false;
}

//----------------------------------------------------------------------------
void vtkMRMLContourNode::RemoveRepresentation(ContourRepresentationType type)
{
  switch (type)
  {
  case RibbonModel:
    this->SetAndObserveRibbonModelPolyData(NULL);
    break;
  case IndexedLabelmap:
    this->SetAndObserveLabelmapImageData(NULL);
    break;
  case ClosedSurfaceModel:
    this->SetAndObserveClosedSurfacePolyData(NULL);
    break;
  default:
    vtkErrorMacro("Unknown contour representation type sent to vtkMRMLContourNode::RemoveRepresentatio. Nothing done.");
  }
}

//----------------------------------------------------------------------------
const char* vtkMRMLContourNode::GetStructureName()
{
  // Get associated subject hierarchy node
  vtkMRMLSubjectHierarchyNode* contourSubjectHierarchyNode = vtkMRMLSubjectHierarchyNode::GetAssociatedSubjectHierarchyNode(this);
  if (!contourSubjectHierarchyNode)
  {
    vtkErrorMacro("GetStructureName: No subject hierarchy node found for contour '" << this->Name << "'");
    return NULL;
  }

  return contourSubjectHierarchyNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_STRUCTURE_NAME_ATTRIBUTE_NAME.c_str());
}

//----------------------------------------------------------------------------
void vtkMRMLContourNode::GetColor(int &colorIndex, vtkMRMLColorTableNode* &colorNode, vtkMRMLScene* scene/*=NULL*/)
{
  colorNode = NULL;

  if (!scene)
  {
    scene = this->Scene;
    if (!scene)
    {
      vtkErrorMacro("GetColor: No MRML scene available (not given as argument, and the associated node has no scene)!");
      return;
    }
  }

  // Get associated subject hierarchy node and its parent
  vtkMRMLSubjectHierarchyNode* contourSubjectHierarchyNode = vtkMRMLSubjectHierarchyNode::GetAssociatedSubjectHierarchyNode(this, scene);
  if (!contourSubjectHierarchyNode)
  {
    vtkDebugMacro("GetColorIndex: No subject hierarchy node found for contour '" << this->Name << "'");
    colorIndex = SlicerRtCommon::COLOR_INDEX_INVALID;
    return;
  }
  vtkMRMLSubjectHierarchyNode* parentContourSubjectHierarchyNode = vtkMRMLSubjectHierarchyNode::SafeDownCast(contourSubjectHierarchyNode->GetParentNode());
  if (!parentContourSubjectHierarchyNode)
  {
    vtkDebugMacro("GetColorIndex: No contour set subject hierarchy node found for contour '" << this->Name << "'");
    colorIndex = SlicerRtCommon::COLOR_INDEX_INVALID;
    return;
  }

  // Get color node created for the contour set
  colorNode = vtkMRMLColorTableNode::SafeDownCast( parentContourSubjectHierarchyNode->GetNodeReference(
    SlicerRtCommon::CONTOUR_SET_COLOR_TABLE_REFERENCE_ROLE.c_str()) );
  if (!colorNode)
  {
    vtkErrorMacro("GetColorIndex: No color table found for contour '" << this->Name << "'");
    return;
  }

  // Do not continue to look for the color index if it was invalid
  // It is a feature of the function, that if the colorIndex was initialized as invalid, then only the color node is acquired
  if (colorIndex == SlicerRtCommon::COLOR_INDEX_INVALID)
  {
    vtkDebugMacro("GetColorIndex: Input color index was set to invalid, so the color index is not acquired.")
      return;
  }

  const char* structureName = contourSubjectHierarchyNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_STRUCTURE_NAME_ATTRIBUTE_NAME.c_str());
  if (!structureName)
  {
    vtkErrorMacro("GetColorIndex: No structure name found for contour '" << this->Name << "'");
    return;
  }

  // Initialize output color index with Gray 'invalid' color (value 1)
  colorIndex = SlicerRtCommon::COLOR_INDEX_INVALID;

  int foundColorIndex = -1;
  if ( (foundColorIndex = colorNode->GetColorIndexByName(structureName)) != -1 )
  {
    colorIndex = foundColorIndex;
  }
  else
  {
    vtkErrorMacro("GetColorIndex: No matching entry found in the color table '" << colorNode->GetName() << "' for contour '" << this->Name
      << "' (structure '" << structureName <<"')");
  }
}

//----------------------------------------------------------------------------
void vtkMRMLContourNode::SetDefaultConversionParametersForRepresentation(ContourRepresentationType type)
{
  if (type == IndexedLabelmap || type == ClosedSurfaceModel)
  {
    if (this->RasterizationOversamplingFactor == -1.0)
    {
      this->SetRasterizationOversamplingFactor(SlicerRtCommon::DEFAULT_RASTERIZATION_OVERSAMPLING_FACTOR);
    }
  }
  else if (type == ClosedSurfaceModel)
  {
    if (this->DecimationTargetReductionFactor == -1.0)
    {
      this->SetDecimationTargetReductionFactor(SlicerRtCommon::DEFAULT_DECIMATION_TARGET_REDUCTION_FACTOR);
    }
  }
}

//---------------------------------------------------------------------------
bool vtkMRMLContourNode::IsRepresentationVisible(ContourRepresentationType type)
{
  vtkMRMLDisplayNode* displayNode(NULL);
  for( int i = 0; i < this->GetNumberOfDisplayNodes(); ++i )
  {
    displayNode = this->GetNthDisplayNode(i);
    if( vtkMRMLContourModelDisplayNode::SafeDownCast(displayNode)  != NULL )
    {
      // Closed surface or ribbon model?
      if( vtkMRMLContourModelDisplayNode::SafeDownCast(displayNode)->GetInputPolyData() == this->RibbonModelPolyData && type == RibbonModel )
      {
        return true;
      }
      else if( vtkMRMLContourModelDisplayNode::SafeDownCast(displayNode)->GetInputPolyData() == this->ClosedSurfacePolyData && type == ClosedSurfaceModel )
      {
        return true;
      }
    }
    // TODO : when 2d visualization is added back in, respond to query here
  }

  return false;
}

//---------------------------------------------------------------------------
void vtkMRMLContourNode::SetName(const char* newName)
{
  // Strip postfix
  std::string contourNameNoPostfix = this->Name ? std::string(this->Name) : "";
  vtksys::SystemTools::ReplaceString(contourNameNoPostfix, SlicerRtCommon::DICOMRTIMPORT_CONTOUR_NODE_NAME_POSTFIX.c_str(), "");
  std::string newContourNameNoPostfix = newName ? std::string(newName) : "";
  vtksys::SystemTools::ReplaceString(newContourNameNoPostfix, SlicerRtCommon::DICOMRTIMPORT_CONTOUR_NODE_NAME_POSTFIX.c_str(), "");
  bool updateContourName = true;
  if (this->Name && newName && contourNameNoPostfix == newContourNameNoPostfix)
  {
    updateContourName = false;
  }
  if (updateContourName)
  {
    if (this->Name) { delete [] this->Name; }
    if (newName)
    {
      std::string newContourName = newContourNameNoPostfix + SlicerRtCommon::DICOMRTIMPORT_CONTOUR_NODE_NAME_POSTFIX;
      size_t n = newContourName.length() + 1;
      char *cp1 =  new char[n];
      const char *cp2 = (newContourName.c_str());
      this->Name = cp1;
      do { *cp1++ = *cp2++; } while ( --n );
    }
    else
    {
      this->Name = NULL;
    }
    this->Modified();
  }
  else
  {
    vtkMRMLSubjectHierarchyNode* contourSubjectHierarchyNode = vtkMRMLSubjectHierarchyNode::GetAssociatedSubjectHierarchyNode(this);
    if (contourSubjectHierarchyNode)
    {
      std::string newContourShName = newContourNameNoPostfix + SlicerRtCommon::DICOMRTIMPORT_CONTOUR_NODE_NAME_POSTFIX + vtkMRMLSubjectHierarchyConstants::SUBJECTHIERARCHY_NODE_NAME_POSTFIX;
      if (strcmp(contourSubjectHierarchyNode->GetName(), newContourShName.c_str()))
      {
        contourSubjectHierarchyNode->SetName(newContourShName.c_str());
      }
    }
  }
}

//---------------------------------------------------------------------------
bool vtkMRMLContourNode::RibbonModelContainsEmptyPolydata()
{
  return this->RibbonModelPolyData == NULL || 
    this->RibbonModelPolyData->GetNumberOfPoints() == 0;
}

//---------------------------------------------------------------------------
void vtkMRMLContourNode::UpdateRepresentations()
{
  this->SetAndObserveLabelmapImageData(this->LabelmapImageData);
  this->SetAndObserveRibbonModelPolyData(this->RibbonModelPolyData);
  this->SetAndObserveClosedSurfacePolyData(this->ClosedSurfacePolyData);
}

//---------------------------------------------------------------------------
std::string vtkMRMLContourNode::GetRepresentationTypeAsString( ContourRepresentationType type )
{
  switch ( type )
  {
  case RibbonModel:
    return "RibbonModel";
    break;
  case IndexedLabelmap:
    return "IndexedLabelmap";
    break;
  case ClosedSurfaceModel:
    return "ClosedSurfaceModel";
    break;
  default:
    return "Unknown";
    break;
  }

  return "";
}

//---------------------------------------------------------------------------
void vtkMRMLContourNode::OnNodeReferenceAdded( vtkMRMLNodeReference *reference )
{
  if (std::string(reference->GetReferenceRole()) == this->DisplayNodeReferenceRole)
  {
    vtkMRMLContourModelDisplayNode* modelDisplayNode = vtkMRMLContourModelDisplayNode::SafeDownCast(reference->ReferencedNode);
    if( modelDisplayNode )
    {
      this->SetPolyDataToDisplayNode(modelDisplayNode->GetInputPolyData(), modelDisplayNode);
    }

    //TODO: When 2d vis is added back in, set the image data to the vis node
    //vtkMRMLContourLabelmapDisplayNode* labelmapDisplayNode = vtkMRMLContourLabelmapDisplayNode::SafeDownCast(reference->ReferencedNode);
    //if( labelmapDisplayNode )
    //{
//      this->SetImageDataToDisplayNode(labelmapDisplayNode);
    //}
  }

  Superclass::OnNodeReferenceAdded(reference);
}

//---------------------------------------------------------------------------
void vtkMRMLContourNode::OnNodeReferenceModified( vtkMRMLNodeReference *reference )
{
  vtkMRMLContourModelDisplayNode* modelDisplayNode = vtkMRMLContourModelDisplayNode::SafeDownCast(reference->ReferencedNode);
  if( modelDisplayNode )
  {
    this->SetPolyDataToDisplayNode(modelDisplayNode->GetInputPolyData(), modelDisplayNode);
  }

  // When 2d vis is added back in, set the image data to the vis node
  //vtkMRMLContourLabelmapDisplayNode* labelmapDisplayNode = vtkMRMLContourLabelmapDisplayNode::SafeDownCast(reference->ReferencedNode);
  //if( labelmapDisplayNode )
  //{
//    this->SetImageDataToDisplayNode(labelmapDisplayNode);
//  }

  Superclass::OnNodeReferenceModified(reference);
}

//---------------------------------------------------------------------------
void vtkMRMLContourNode::SetPolyDataToDisplayNodes(vtkPolyData* polyData, ContourRepresentationType type)
{
  int ndisp = this->GetNumberOfDisplayNodes();
  for (int n=0; n<ndisp; n++)
  {
    vtkMRMLContourModelDisplayNode *dnode = vtkMRMLContourModelDisplayNode::SafeDownCast( this->GetNthDisplayNode(n) );
    if (dnode && type == ClosedSurfaceModel && this->GetClosedSurfaceModelDisplayNode() == dnode )
    {
      this->SetPolyDataToDisplayNode(polyData, dnode);
    }
    else if( dnode && type == RibbonModel && this->GetRibbonModelDisplayNode() == dnode )
    {
      this->SetPolyDataToDisplayNode(polyData, dnode);
    }
  }
}

//---------------------------------------------------------------------------
void vtkMRMLContourNode::SetPolyDataToDisplayNode(vtkPolyData* polyData, vtkMRMLContourModelDisplayNode* modelDisplayNode)
{
  assert(modelDisplayNode); //TODO: No assert please. "If" check and then vtkErrorMacro

#if (VTK_MAJOR_VERSION <= 5)
  modelDisplayNode->SetInputPolyData(polyData);
#else
  //TODO: Use GetProducerPort()->GetProducer() once [member] becomes [member]Connection
  vtkSmartPointer<vtkTrivialProducer> polyDataProducer = vtkSmartPointer<vtkTrivialProducer>::New();
  polyDataProducer->SetOutput(polyData);
  modelDisplayNode->SetInputPolyDataConnection(polyDataProducer->GetOutputPort());
#endif
}

//---------------------------------------------------------------------------
void vtkMRMLContourNode::SetImageDataToDisplayNodes()
{
  int ndisp = this->GetNumberOfDisplayNodes();
  for (int n=0; n<ndisp; n++)
  {
    // TODO : 2d vis readdition
    //vtkMRMLContourLabelmapDisplayNode *dnode = vtkMRMLContourLabelmapDisplayNode::SafeDownCast( this->GetNthDisplayNode(n) );
    //if (dnode)
    //{
//      this->SetImageDataToDisplayNode(dnode);
    //}
  }
}

//---------------------------------------------------------------------------
//void vtkMRMLContourNode::SetImageDataToDisplayNode(vtkMRMLContourLabelmapDisplayNode* labelmapDisplayNode)
//{
//  assert(labelmapDisplayNode);
//  labelmapDisplayNode->SetInputImageData( this->GetLabelmapImageData() );
//}

//---------------------------------------------------------------------------
void vtkMRMLContourNode::SetAndObserveRibbonModelPolyData( vtkPolyData *polyData )
{
  if (polyData == this->RibbonModelPolyData)
  {
    return;
  }

  vtkPolyData* oldPolyData = this->RibbonModelPolyData;

  this->SetRibbonModelPolyData(polyData);

  if (this->RibbonModelPolyData != NULL)
  {
    vtkEventBroker::GetInstance()->AddObservation(
      this->RibbonModelPolyData, vtkCommand::ModifiedEvent, this, this->MRMLCallbackCommand );
    this->RibbonModelPolyData->Register(this);
  }

  this->SetPolyDataToDisplayNodes(this->RibbonModelPolyData, RibbonModel);

  if (oldPolyData != NULL)
  {
    vtkEventBroker::GetInstance()->RemoveObservations (
      oldPolyData, vtkCommand::ModifiedEvent, this, this->MRMLCallbackCommand );
    oldPolyData->UnRegister(this);
  }

  this->StorableModifiedTime.Modified();
  this->Modified();
  this->InvokeEvent( vtkMRMLContourNode::RibbonModelPolyDataModifiedEvent , this);
}

//---------------------------------------------------------------------------
void vtkMRMLContourNode::SetAndObserveClosedSurfacePolyData( vtkPolyData *polyData )
{
  if (polyData == this->ClosedSurfacePolyData)
  {
    return;
  }

  vtkPolyData* oldPolyData = this->ClosedSurfacePolyData;

  this->SetClosedSurfacePolyData(polyData);

  if (this->ClosedSurfacePolyData != NULL)
  {
    vtkEventBroker::GetInstance()->AddObservation(
      this->ClosedSurfacePolyData, vtkCommand::ModifiedEvent, this, this->MRMLCallbackCommand );
    this->ClosedSurfacePolyData->Register(this);
  }

  this->SetPolyDataToDisplayNodes(this->ClosedSurfacePolyData, ClosedSurfaceModel);

  if (oldPolyData != NULL)
  {
    vtkEventBroker::GetInstance()->RemoveObservations (
      oldPolyData, vtkCommand::ModifiedEvent, this, this->MRMLCallbackCommand );
    oldPolyData->UnRegister(this);
  }

  this->StorableModifiedTime.Modified();
  this->Modified();
  this->InvokeEvent( vtkMRMLContourNode::ClosedSurfacePolyDataModifiedEvent , this);
}

//---------------------------------------------------------------------------
void vtkMRMLContourNode::SetAndObserveLabelmapImageData( vtkImageData *imageData )
{
  if (imageData == this->LabelmapImageData)
  {
    return;
  }

  vtkImageData* oldImageData = this->LabelmapImageData;

  this->SetLabelmapImageData(imageData);

  if (this->LabelmapImageData != NULL)
  {
    vtkEventBroker::GetInstance()->AddObservation(
      this->LabelmapImageData, vtkCommand::ModifiedEvent, this, this->MRMLCallbackCommand );
    this->LabelmapImageData->Register(this);
  }

  this->SetImageDataToDisplayNodes();

  if (oldImageData != NULL)
  {
    vtkEventBroker::GetInstance()->RemoveObservations (
      oldImageData, vtkCommand::ModifiedEvent, this, this->MRMLCallbackCommand );
    oldImageData->UnRegister(this);
  }

  this->StorableModifiedTime.Modified();
  this->Modified();
  this->InvokeEvent( vtkMRMLContourNode::LabelmapImageDataModifiedEvent , this);
}

//----------------------------------------------------------------------------
vtkMRMLStorageNode* vtkMRMLContourNode::CreateDefaultStorageNode()
{
  return vtkMRMLStorageNode::SafeDownCast(vtkMRMLContourStorageNode::New());
}

//----------------------------------------------------------------------------
void vtkMRMLContourNode::ApplyTransform( vtkAbstractTransform* transform )
{
  // Do not call superclass ApplyTransform

  // TODO : maybe deform only the ribbon model / closed surface model
  // then regenerate the labelmap if it existed before?

  vtkHomogeneousTransform* linearTransform = vtkHomogeneousTransform::SafeDownCast(transform);
  if (linearTransform)
  {
    // linear labelmap transformation
    vtkNew<vtkMatrix4x4> ijkToRASMatrix;
    vtkNew<vtkMatrix4x4> newIJKToRASMatrix;
    this->GetIJKToRASMatrix(ijkToRASMatrix.GetPointer());
    vtkMatrix4x4::Multiply4x4(linearTransform->GetMatrix(), ijkToRASMatrix.GetPointer(), newIJKToRASMatrix.GetPointer());
    this->SetIJKToRASMatrix(newIJKToRASMatrix.GetPointer());
  }
  else
  {
    // Non-linear labelmap transformation
    if ( this->HasRepresentation(IndexedLabelmap) )
    {
      int extent[6];
      this->GetLabelmapImageData()->GetExtent(extent);

      vtkNew<vtkMatrix4x4> rasToIJK;
      vtkNew<vtkImageResliceMask> reslice;

      vtkNew<vtkGeneralTransform> resampleXform;
      resampleXform->Identity();
      resampleXform->PostMultiply();

      this->GetRASToIJKMatrix(rasToIJK.GetPointer());

      vtkNew<vtkMatrix4x4> IJKToRAS;
      IJKToRAS->DeepCopy(rasToIJK.GetPointer());
      IJKToRAS->Invert();
      transform->Inverse();

      resampleXform->Concatenate(IJKToRAS.GetPointer());
      resampleXform->Concatenate(transform);
      resampleXform->Concatenate(rasToIJK.GetPointer());

      reslice->SetResliceTransform(resampleXform.GetPointer());

#if (VTK_MAJOR_VERSION <= 5)
      reslice->SetInput(this->LabelmapImageData);
#else
      reslice->SetInputData(this->LabelmapImageData);
#endif
      reslice->SetInterpolationModeToLinear();
      reslice->SetBackgroundColor(0, 0, 0, 0);
      reslice->AutoCropOutputOff();
      reslice->SetOptimization(1);
      reslice->SetOutputOrigin( this->GetLabelmapImageData()->GetOrigin() );
      reslice->SetOutputSpacing( this->GetLabelmapImageData()->GetSpacing() );
      reslice->SetOutputDimensionality( 3 );
      reslice->SetOutputExtent( extent);
#if (VTK_MAJOR_VERSION <= 5)
      reslice->GetBackgroundMask()->SetUpdateExtentToWholeExtent();
#else
      reslice->GetBackgroundMaskPort()->GetProducer()->SetUpdateExtentToWholeExtent();
#endif
      reslice->Update();

      vtkNew<vtkImageData> resampleImage;
      resampleImage->DeepCopy(reslice->GetOutput());

      this->SetAndObserveLabelmapImageData(resampleImage.GetPointer());
    }
  }

  // Ribbon model
  if( this->HasRepresentation(RibbonModel) )
  {
    vtkTransformPolyDataFilter* transformFilter = vtkTransformPolyDataFilter::New();

#if (VTK_MAJOR_VERSION <= 5)
    transformFilter->SetInput(this->RibbonModelPolyData);
#else
    transformFilter->SetInputData(this->RibbonModelPolyData);
#endif
    transformFilter->SetTransform(transform);
    transformFilter->Update();

#if (VTK_MAJOR_VERSION <= 5)
    bool isInPipeline = !vtkTrivialProducer::SafeDownCast(
      this->RibbonModelPolyData ? this->RibbonModelPolyData->GetProducerPort()->GetProducer() : 0);
#else
    //TODO: Use GetProducerPort()->GetProducer() once [member] becomes [member]Connection
    vtkSmartPointer<vtkTrivialProducer> ribbonModelProducer = vtkSmartPointer<vtkTrivialProducer>::New();
    ribbonModelProducer->SetOutput(this->RibbonModelPolyData);
    bool isInPipeline = !vtkTrivialProducer::SafeDownCast(
      this->RibbonModelPolyData ? ribbonModelProducer->GetOutputPort()->GetProducer() : 0);
#endif

    vtkSmartPointer<vtkPolyData> polyData;
    if (isInPipeline)
    {
      polyData = vtkSmartPointer<vtkPolyData>::New();
    }
    else
    {
      polyData = this->RibbonModelPolyData;
    }
    polyData->DeepCopy(transformFilter->GetOutput());
    if (isInPipeline)
    {
      this->SetAndObserveRibbonModelPolyData(polyData);
    }
    transformFilter->Delete();
  }

  // Closed surface model
  if( this->HasRepresentation(ClosedSurfaceModel) )
  {
    vtkTransformPolyDataFilter* transformFilter = vtkTransformPolyDataFilter::New();

#if (VTK_MAJOR_VERSION <= 5)
    transformFilter->SetInput(this->ClosedSurfacePolyData);
#else
    transformFilter->SetInputData(this->ClosedSurfacePolyData);
#endif
    transformFilter->SetTransform(transform);
    transformFilter->Update();

#if (VTK_MAJOR_VERSION <= 5)
    bool isInPipeline = !vtkTrivialProducer::SafeDownCast(
      this->ClosedSurfacePolyData ? this->ClosedSurfacePolyData->GetProducerPort()->GetProducer() : 0);
#else
    //TODO: Use GetProducerPort()->GetProducer() once [member] becomes [member]Connection
    vtkSmartPointer<vtkTrivialProducer> closedSurfaceModelProducer = vtkSmartPointer<vtkTrivialProducer>::New();
    closedSurfaceModelProducer->SetOutput(this->ClosedSurfacePolyData);
    bool isInPipeline = !vtkTrivialProducer::SafeDownCast(
      this->ClosedSurfacePolyData ? closedSurfaceModelProducer->GetOutputPort()->GetProducer() : 0);
#endif

    vtkSmartPointer<vtkPolyData> polyData;
    if (isInPipeline)
    {
      polyData = vtkSmartPointer<vtkPolyData>::New();
    }
    else
    {
      polyData = this->ClosedSurfacePolyData;
    }
    polyData->DeepCopy(transformFilter->GetOutput());
    if (isInPipeline)
    {
      this->SetAndObserveClosedSurfacePolyData(polyData);
    }
    transformFilter->Delete();
  }
}

//----------------------------------------------------------------------------
bool vtkMRMLContourNode::CanApplyNonLinearTransforms() const
{
  return true;
}

//----------------------------------------------------------------------------
bool vtkMRMLContourNode::HasBeenCreatedFromIndexedLabelmap()
{
  return this->CreatedFromIndexLabelmap;
}

// TODO : 2d vis readdition
//----------------------------------------------------------------------------
//vtkMRMLContourLabelmapDisplayNode* vtkMRMLContourNode::GetLabelmapVolumeDisplayNode()
//{
//  vtkMRMLDisplayNode* displayNode(NULL);
//  for( int i = 0; i < this->GetNumberOfDisplayNodes(); ++i )
//  {
    //displayNode = this->GetNthDisplayNode(i);
    //if( vtkMRMLContourLabelmapDisplayNode::SafeDownCast(displayNode) != NULL )
    //{
//      return vtkMRMLContourLabelmapDisplayNode::SafeDownCast(displayNode);
    //}
  //}

  //return NULL;
//}

//----------------------------------------------------------------------------
vtkMRMLContourModelDisplayNode* vtkMRMLContourNode::GetRibbonModelDisplayNode()
{
  vtkMRMLDisplayNode* displayNode(NULL);
  for( int i = 0; i < this->GetNumberOfDisplayNodes(); ++i )
  {
    displayNode = this->GetNthDisplayNode(i);
    vtkMRMLContourModelDisplayNode* modelDisplayNode = vtkMRMLContourModelDisplayNode::SafeDownCast(displayNode);
    if( modelDisplayNode && modelDisplayNode->GetContourDisplayType() == RibbonModel )
    {
      return modelDisplayNode;
    }
  }

  return NULL;
}

//----------------------------------------------------------------------------
vtkMRMLContourModelDisplayNode* vtkMRMLContourNode::GetClosedSurfaceModelDisplayNode()
{
  vtkMRMLDisplayNode* displayNode(NULL);
  for( int i = 0; i < this->GetNumberOfDisplayNodes(); ++i )
  {
    displayNode = this->GetNthDisplayNode(i);
    vtkMRMLContourModelDisplayNode* modelDisplayNode = vtkMRMLContourModelDisplayNode::SafeDownCast(displayNode);
    if( modelDisplayNode && modelDisplayNode->GetContourDisplayType() == ClosedSurfaceModel )
    {
      return modelDisplayNode;
    }
  }

  return NULL;
}

//----------------------------------------------------------------------------
vtkPolyData* vtkMRMLContourNode::GetClosedSurfacePolyData()
{
  if( this->ClosedSurfacePolyData != NULL )
  {
    return this->ClosedSurfacePolyData;
  }
  if (!this->Scene)
  {
    vtkErrorMacro("GetClosedSurfacePolyData: Invalid MRML scene!");
    return NULL;
  }

  vtkSmartPointer<vtkConvertContourRepresentations> converter = vtkSmartPointer<vtkConvertContourRepresentations>::New();
  converter->SetContourNode(this);
  if (converter->ConvertToRepresentation(ClosedSurfaceModel))
  {
    return this->ClosedSurfacePolyData;
  }
  else
  {
    vtkErrorMacro("Conversion to closed surface model failed!");
  }

  return NULL;
}

//----------------------------------------------------------------------------
vtkImageData* vtkMRMLContourNode::GetLabelmapImageData()
{
  if( this->LabelmapImageData != NULL )
  {
    return this->LabelmapImageData;
  }
  if (!this->Scene)
  {
    vtkErrorMacro("GetClosedSurfacePolyData: Invalid MRML scene!");
    return NULL;
  }

  vtkSmartPointer<vtkConvertContourRepresentations> converter = vtkSmartPointer<vtkConvertContourRepresentations>::New();
  converter->SetContourNode(this);
  if (converter->ConvertToRepresentation(IndexedLabelmap))
  {
    // TODO : 2d vis readdition -- Create display node if it doesn't exist
    return this->LabelmapImageData;
  }
  else
  {
    vtkErrorMacro("Conversion to closed surface model failed!");
  }

  return NULL;
}

//----------------------------------------------------------------------------
void vtkMRMLContourNode::GetIJKToRASMatrix(vtkMatrix4x4* mat)
{
  // this is the full matrix including the spacing and origin
  mat->Identity();
  int row, col;
  for (row=0; row<3; row++)
  {
    for (col=0; col<3; col++)
    {
      mat->SetElement(row, col, this->Spacing[col] * IJKToRASDirections[row][col]);
    }
    mat->SetElement(row, 3, this->Origin[row]);
  }
}

//----------------------------------------------------------------------------
void vtkMRMLContourNode::GetRASToIJKMatrix(vtkMatrix4x4* mat)
{
  this->GetIJKToRASMatrix( mat );
  mat->Invert();
}

//----------------------------------------------------------------------------
void vtkMRMLContourNode::SetIJKToRASMatrix(vtkMatrix4x4* argMat)
{
  if (argMat == NULL)
  {
    return;
  }
  vtkNew<vtkMatrix4x4> mat;
  mat->DeepCopy(argMat);

  // normalize direction vectors
  double spacing[3];
  int col;
  for (col=0; col<3; col++)
  {
    double len =0;
    int row;
    for (row=0; row<3; row++)
    {
      len += mat->GetElement(row, col) * mat->GetElement(row, col);
    }
    len = sqrt(len);
    spacing[col] = len;
    for (row=0; row<3; row++)
    {
      mat->SetElement(row, col,  mat->GetElement(row, col)/len);
    }
  }

  double dirs[3][3];
  double origin[3];
  for (int row=0; row<3; row++)
  {
    for (int col=0; col<3; col++)
    {
      dirs[row][col] = mat->GetElement(row, col);
    }
    origin[row] = mat->GetElement(row, 3);
  }

  int disabledModify = this->StartModify();
  this->SetIJKToRASDirections(dirs);
  this->SetSpacing(spacing);
  this->SetOrigin(origin);
  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLContourNode::SetRASToIJKMatrix(vtkMatrix4x4* mat)
{
  vtkNew<vtkMatrix4x4> m;
  m->Identity();
  if (mat)
  {
    m->DeepCopy(mat);
  }
  m->Invert();
  this->SetIJKToRASMatrix(m.GetPointer());
}

//----------------------------------------------------------------------------
void vtkMRMLContourNode::SetIJKToRASDirections(double dirs[3][3])
{
  bool isModified = false;
  for (int i=0; i<3; i++)
  {
    for (int j=0; j<3; j++)
    {
      if (!vtkMathUtilities::FuzzyCompare<double>(this->IJKToRASDirections[i][j], dirs[i][j]))
      {
        this->IJKToRASDirections[i][j] = dirs[i][j];
        isModified = true;
      }
    }
  }
  if (isModified)
  {
    this->StorableModifiedTime.Modified();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkMRMLContourNode::SetSpacing(double arg1, double arg2, double arg3)
{
  if (!vtkMathUtilities::FuzzyCompare<double>(this->Spacing[0], arg1) ||
    !vtkMathUtilities::FuzzyCompare<double>(this->Spacing[1], arg2) ||
    !vtkMathUtilities::FuzzyCompare<double>(this->Spacing[2], arg3))
  {
    this->Spacing[0] = arg1;
    this->Spacing[1] = arg2;
    this->Spacing[2] = arg3;
    this->StorableModifiedTime.Modified();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkMRMLContourNode::SetSpacing(double arg[3])
{
  this->SetSpacing(arg[0], arg[1], arg[2]);
}

//----------------------------------------------------------------------------
void vtkMRMLContourNode::SetOrigin(double arg1, double arg2, double arg3)
{
  if (!vtkMathUtilities::FuzzyCompare<double>(this->Origin[0], arg1) ||
    !vtkMathUtilities::FuzzyCompare<double>(this->Origin[1], arg2) ||
    !vtkMathUtilities::FuzzyCompare<double>(this->Origin[2], arg3))
  {
    this->Origin[0] = arg1;
    this->Origin[1] = arg2;
    this->Origin[2] = arg3;
    this->StorableModifiedTime.Modified();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkMRMLContourNode::SetOrigin(double arg[3])
{
  this->SetOrigin(arg[0], arg[1], arg[2]);
}

//----------------------------------------------------------------------------
void vtkMRMLContourNode::CopyOrientation(vtkMRMLContourNode *node)
{
  double dirs[3][3];
  node->GetIJKToRASDirections(dirs);

  int disabledModify = this->StartModify();
  this->SetIJKToRASDirections(dirs);
  this->SetOrigin(node->GetOrigin());
  this->SetSpacing(node->GetSpacing());
  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLContourNode::CopyOrientation(vtkMRMLScalarVolumeNode *node)
{
  double dirs[3][3];
  node->GetIJKToRASDirections(dirs);

  int disabledModify = this->StartModify();
  this->SetIJKToRASDirections(dirs);
  this->SetOrigin(node->GetOrigin());
  this->SetSpacing(node->GetSpacing());
  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLContourNode::GetIJKToRASDirections(double dirs[3][3])
{
  for (int i=0; i<3; i++)
  {
    for (int j=0; j<3; j++)
    {
      dirs[i][j] = IJKToRASDirections[i][j];
    }
  }
}

//---------------------------------------------------------------------------
void vtkMRMLContourNode::SetMetaDataDictionary( const itk::MetaDataDictionary& dictionary )
{
  this->Dictionary = dictionary;
  this->Modified();
}

//---------------------------------------------------------------------------
const
itk::MetaDataDictionary&
vtkMRMLContourNode::GetMetaDataDictionary() const
{
  return this->Dictionary;
}

//----------------------------------------------------------------------------
bool vtkMRMLContourNode::ResampleInputContourNodeToReferenceVolumeNode(vtkMRMLScene* scene, vtkMRMLContourNode* inContourNode, vtkMRMLScalarVolumeNode* refVolumeNode, vtkMRMLContourNode* outContourNode)
{
  int dimensions[3] = {0, 0, 0};

  // Make sure inputs are initialized
  if (!scene || !inContourNode || !refVolumeNode || !outContourNode)
  {
    vtkGenericWarningMacro("vtkVolumesOrientedResampleUtility::ResampleInputVolumeNodeToReferenceVolumeNode: Inputs are not specified!");
    return false;
  }

  // Make sure input volume node is in the scene
  if (!inContourNode->GetScene())
  {
    vtkErrorWithObjectMacro(inContourNode, "vtkVolumesOrientedResampleUtility::ResampleInputVolumeNodeToReferenceVolumeNode: Input volume node is not in a MRML scene!");
    return false;
  }

  vtkSmartPointer<vtkMatrix4x4> inputVolumeIJK2RASMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  inContourNode->GetIJKToRASMatrix(inputVolumeIJK2RASMatrix);
  vtkSmartPointer<vtkMatrix4x4> referenceVolumeRAS2IJKMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  refVolumeNode->GetRASToIJKMatrix(referenceVolumeRAS2IJKMatrix);
  refVolumeNode->GetImageData()->GetDimensions(dimensions);

  vtkSmartPointer<vtkTransform> outputVolumeResliceTransform = vtkSmartPointer<vtkTransform>::New();
  outputVolumeResliceTransform->Identity();
  outputVolumeResliceTransform->PostMultiply();
  outputVolumeResliceTransform->SetMatrix(inputVolumeIJK2RASMatrix);

  vtkSmartPointer<vtkMRMLTransformNode> inputVolumeNodeTransformNode = vtkMRMLTransformNode::SafeDownCast(
    scene->GetNodeByID(inContourNode->GetTransformNodeID()));
  vtkSmartPointer<vtkMatrix4x4> inputVolumeRAS2RASMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  if (inputVolumeNodeTransformNode!=NULL)
  {
    inputVolumeNodeTransformNode->GetMatrixTransformToWorld(inputVolumeRAS2RASMatrix);
    outputVolumeResliceTransform->Concatenate(inputVolumeRAS2RASMatrix);
  }
  vtkSmartPointer<vtkMRMLTransformNode> referenceVolumeNodeTransformNode = vtkMRMLTransformNode::SafeDownCast(
    scene->GetNodeByID(refVolumeNode->GetTransformNodeID()));
  vtkSmartPointer<vtkMatrix4x4> referenceVolumeRAS2RASMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  if (referenceVolumeNodeTransformNode!=NULL)
  {
    inputVolumeNodeTransformNode->GetMatrixTransformToWorld(referenceVolumeRAS2RASMatrix);
    referenceVolumeRAS2RASMatrix->Invert();
    outputVolumeResliceTransform->Concatenate(referenceVolumeRAS2RASMatrix);
  }
  outputVolumeResliceTransform->Concatenate(referenceVolumeRAS2IJKMatrix);
  outputVolumeResliceTransform->Inverse();

  vtkSmartPointer<vtkImageReslice> resliceFilter = vtkSmartPointer<vtkImageReslice>::New();
#if (VTK_MAJOR_VERSION <= 5)
  resliceFilter->SetInput(inContourNode->GetLabelmapImageData());
#else
  resliceFilter->SetInputData(inContourNode->GetLabelmapImageData());
#endif
  resliceFilter->SetOutputOrigin(0, 0, 0);
  resliceFilter->SetOutputSpacing(1, 1, 1);
  resliceFilter->SetOutputExtent(0, dimensions[0]-1, 0, dimensions[1]-1, 0, dimensions[2]-1);
  resliceFilter->SetResliceTransform(outputVolumeResliceTransform);
  resliceFilter->Update();

  outContourNode->CopyOrientation(refVolumeNode);
  outContourNode->SetAndObserveLabelmapImageData(resliceFilter->GetOutput());

  return true;
}

//---------------------------------------------------------------------------
bool vtkMRMLContourNode::DoVolumeLatticesMatch(vtkMRMLContourNode* contour1, vtkMRMLScalarVolumeNode* volume2)
{
  if (!contour1 || !volume2)
  {
    std::cerr << "SlicerRtCommon::DoVolumeLatticesMatch: Invalid (NULL) argument!" << std::endl;
    return false;
  }

  // Get VTK image data objects
  vtkImageData* imageData1 = contour1->GetLabelmapImageData();
  vtkImageData* imageData2 = volume2->GetImageData();
  if (!imageData1 || !imageData2)
  {
    vtkErrorWithObjectMacro(contour1, "SlicerRtCommon::DoVolumeLatticesMatch: At least one of the input volume nodes does not have a valid image data!");
    return false;
  }

  // Check parent transforms (they have to be in the same branch)
  if (contour1->GetParentTransformNode() != volume2->GetParentTransformNode())
  {
    vtkDebugWithObjectMacro(contour1, "SlicerRtCommon::DoVolumeLatticesMatch: Parent transform nodes are not the same for the two input volumes");
    return false;
  }

  // Compare IJK to RAS matrices (involves checking the spacing and origin too)
  vtkSmartPointer<vtkMatrix4x4> ijkToRasMatrix1 = vtkSmartPointer<vtkMatrix4x4>::New();
  vtkSmartPointer<vtkMatrix4x4> ijkToRasMatrix2 = vtkSmartPointer<vtkMatrix4x4>::New();
  contour1->GetIJKToRASMatrix(ijkToRasMatrix1);
  volume2->GetIJKToRASMatrix(ijkToRasMatrix2);
  for (int row=0; row<3; ++row)
  {
    for (int col=0; col<3; ++col)
    {
      if ( fabs(ijkToRasMatrix1->GetElement(row, col) - ijkToRasMatrix2->GetElement(row, col)) > EPSILON )
      {
        vtkDebugWithObjectMacro(contour1, "SlicerRtCommon::DoVolumeLatticesMatch: IJK to RAS matrices differ");
        return false;
      }
    }
  }

  // Check image data properties
  int dimensions1[3] = {0,0,0};
  int dimensions2[3] = {0,0,0};
  imageData1->GetDimensions(dimensions1);
  imageData2->GetDimensions(dimensions2);
  if ( dimensions1[0] != dimensions2[0]
  || dimensions1[1] != dimensions2[1]
  || dimensions1[2] != dimensions2[2] )
  {
    vtkDebugWithObjectMacro(contour1, "SlicerRtCommon::DoVolumeLatticesMatch: VTK image data dimensions differ!!");
    return false;
  }

  int extent1[6] = {0,0,0,0,0,0};
  int extent2[6] = {0,0,0,0,0,0};
  imageData1->GetExtent(extent1);
  imageData2->GetExtent(extent2);
  if ( extent1[0] != extent2[0] || extent1[1] != extent2[1]
  || extent1[2] != extent2[2] || extent1[3] != extent2[3]
  || extent1[4] != extent2[4] || extent1[5] != extent2[5] )
  {
    vtkDebugWithObjectMacro(contour1, "SlicerRtCommon::DoVolumeLatticesMatch: VTK image data extents differ!!");
    return false;
  }

  // All of the tests passed, declare the lattices the same
  return true;
}

//---------------------------------------------------------------------------
void vtkMRMLContourNode::GetRASBounds(double bounds[6])
{
  vtkMath::UninitializeBounds(bounds);

  vtkPolyData* modelData(ClosedSurfacePolyData);
  if (this->ClosedSurfacePolyData == NULL)
  {
    modelData = this->RibbonModelPolyData;
  }
  if( modelData == NULL )
  {
    // Get volume bounds
    this->GetLabelmapRASBounds(bounds);
  }
  else
  {
    modelData->ComputeBounds();

    double boundsLocal[6];
    modelData->GetBounds(boundsLocal);

    this->TransformBoundsToRAS(boundsLocal, bounds);
  }
}

//---------------------------------------------------------------------------
bool vtkMRMLContourNode::GetModifiedSinceRead()
{
  return this->Superclass::GetModifiedSinceRead() ||
    (this->LabelmapImageData && this->LabelmapImageData->GetMTime() > this->GetStoredTime()) ||
    (this->RibbonModelPolyData && this->RibbonModelPolyData->GetMTime() > this->GetStoredTime()) ||
    (this->ClosedSurfacePolyData && this->ClosedSurfacePolyData->GetMTime() > this->GetStoredTime());
}

//---------------------------------------------------------------------------
vtkMRMLContourModelDisplayNode* vtkMRMLContourNode::CreateRibbonModelDisplayNode()
{
  if( this->GetRibbonModelDisplayNode() != NULL )
  {
    return this->GetRibbonModelDisplayNode();
  }

  vtkSmartPointer<vtkMRMLContourModelDisplayNode> displayNode = vtkSmartPointer<vtkMRMLContourModelDisplayNode>::New();
  this->GetScene()->AddNode(displayNode);
  std::string displayName = std::string(this->GetName()) + SlicerRtCommon::CONTOUR_RIBBON_MODEL_NODE_NAME_POSTFIX + SlicerRtCommon::CONTOUR_DISPLAY_NODE_SUFFIX;
  displayNode->SetName(displayName.c_str());
#if (VTK_MAJOR_VERSION <= 5)
  displayNode->SetInputPolyData(this->RibbonModelPolyData);
#else
  //TODO: Use GetProducerPort()->GetProducer() once [member] becomes [member]Connection
  vtkSmartPointer<vtkTrivialProducer> ribbonModelProducer = vtkSmartPointer<vtkTrivialProducer>::New();
  ribbonModelProducer->SetOutput(this->RibbonModelPolyData);
  displayNode->SetInputPolyDataConnection(ribbonModelProducer->GetOutputPort());
#endif
  displayNode->SliceIntersectionVisibilityOn();
  displayNode->VisibilityOn();
  displayNode->SetBackfaceCulling(0);
  displayNode->SetContourDisplayType(RibbonModel);

  this->AddAndObserveDisplayNodeID(displayNode->GetID());

  return displayNode;
}

//---------------------------------------------------------------------------
vtkMRMLContourModelDisplayNode* vtkMRMLContourNode::CreateClosedSurfaceDisplayNode()
{
  if( this->GetClosedSurfaceModelDisplayNode() != NULL )
  {
    return this->GetClosedSurfaceModelDisplayNode();
  }

  vtkSmartPointer<vtkMRMLContourModelDisplayNode> displayNode = vtkSmartPointer<vtkMRMLContourModelDisplayNode>::New();
  this->GetScene()->AddNode(displayNode);
  std::string displayName = std::string(this->GetName()) + SlicerRtCommon::CONTOUR_CLOSED_SURFACE_MODEL_NODE_NAME_POSTFIX + SlicerRtCommon::CONTOUR_DISPLAY_NODE_SUFFIX;
  displayNode->SetName(displayName.c_str());
#if (VTK_MAJOR_VERSION <= 5)
  displayNode->SetInputPolyData(this->ClosedSurfacePolyData);
#else
  //TODO: Use GetProducerPort()->GetProducer() once [member] becomes [member]Connection
  vtkSmartPointer<vtkTrivialProducer> closedSurfaceModelProducer = vtkSmartPointer<vtkTrivialProducer>::New();
  closedSurfaceModelProducer->SetOutput(this->ClosedSurfacePolyData);
  displayNode->SetInputPolyDataConnection(closedSurfaceModelProducer->GetOutputPort());
#endif
  displayNode->SliceIntersectionVisibilityOn();
  displayNode->VisibilityOn();
  displayNode->SetBackfaceCulling(0);
  displayNode->SetContourDisplayType(ClosedSurfaceModel);

  this->AddAndObserveDisplayNodeID(displayNode->GetID());

  return displayNode;
}

//---------------------------------------------------------------------------
void vtkMRMLContourNode::TransformBoundsToRAS( double inputBounds_Local[6], double outputBounds_RAS[6] )
{
  vtkMRMLTransformNode *transformNode = this->GetParentTransformNode();
  if ( !transformNode )
  {
    // node is not transformed, therefore RAS=local
    for (int i=0; i<6; i++)
    {
      outputBounds_RAS[i]=inputBounds_Local[i];
    }
    return;
  }

  vtkNew<vtkGeneralTransform> transformLocalToRAS;
  transformNode->GetTransformToWorld(transformLocalToRAS.GetPointer());

  double cornerPoints_Local[8][4] =
  {
    {inputBounds_Local[0], inputBounds_Local[2], inputBounds_Local[4], 1},
    {inputBounds_Local[0], inputBounds_Local[3], inputBounds_Local[4], 1},
    {inputBounds_Local[0], inputBounds_Local[2], inputBounds_Local[5], 1},
    {inputBounds_Local[0], inputBounds_Local[3], inputBounds_Local[5], 1},
    {inputBounds_Local[1], inputBounds_Local[2], inputBounds_Local[4], 1},
    {inputBounds_Local[1], inputBounds_Local[3], inputBounds_Local[4], 1},
    {inputBounds_Local[1], inputBounds_Local[2], inputBounds_Local[5], 1},
    {inputBounds_Local[1], inputBounds_Local[3], inputBounds_Local[5], 1}
  };

  // initialize bounds with point 0
  double* cornerPoint_RAS = transformLocalToRAS->TransformDoublePoint(cornerPoints_Local[0]);
  for ( int i=0; i<3; i++)
  {
    outputBounds_RAS[2*i]   = cornerPoint_RAS[i];
    outputBounds_RAS[2*i+1] = cornerPoint_RAS[i];
  }

  // update bounds with the rest of the points
  for ( int i=1; i<8; i++)
  {
    cornerPoint_RAS = transformLocalToRAS->TransformPoint( cornerPoints_Local[i] );
    for (int n=0; n<3; n++)
    {
      if (cornerPoint_RAS[n] < outputBounds_RAS[2*n]) // min bound
      {
        outputBounds_RAS[2*n] = cornerPoint_RAS[n];
      }
      if (cornerPoint_RAS[n] > outputBounds_RAS[2*n+1]) // max bound
      {
        outputBounds_RAS[2*n+1] = cornerPoint_RAS[n];
      }
    }
  }
}

//---------------------------------------------------------------------------
void vtkMRMLContourNode::GetLabelmapRASBounds( double bounds[6] )
{
  vtkImageData *volumeImage;
  if ( !(volumeImage = this->GetLabelmapImageData()) )
  {
    return;
  }

  //
  // Get the size of the volume in RAS space
  // - map the size of the volume in IJK into RAS
  // - map the middle of the volume to RAS for the center
  //   (IJK space always has origin at first pixel)
  //

  vtkNew<vtkGeneralTransform> transform;
  transform->PostMultiply();
  transform->Identity();

  vtkNew<vtkMatrix4x4> ijkToRAS;
  this->GetIJKToRASMatrix(ijkToRAS.GetPointer());
  transform->Concatenate(ijkToRAS.GetPointer());

  vtkMRMLTransformNode *transformNode = this->GetParentTransformNode();

  if ( transformNode )
  {
    vtkNew<vtkGeneralTransform> worldTransform;
    worldTransform->Identity();
    //transformNode->GetTransformFromWorld(worldTransform);
    transformNode->GetTransformToWorld(worldTransform.GetPointer());
    transform->Concatenate(worldTransform.GetPointer());
  }

  int dimensions[3];
  int i,j,k;
  volumeImage->GetDimensions(dimensions);
  double doubleDimensions[4], *rasHDimensions;
  double minBounds[3], maxBounds[3];

  for ( i=0; i<3; i++)
  {
    minBounds[i] = 1.0e10;
    maxBounds[i] = -1.0e10;
  }
  for ( i=0; i<2; i++)
  {
    for ( j=0; j<2; j++)
    {
      for ( k=0; k<2; k++)
      {
        doubleDimensions[0] = i*(dimensions[0]) - 0.5;
        doubleDimensions[1] = j*(dimensions[1]) - 0.5 ;
        doubleDimensions[2] = k*(dimensions[2]) - 0.5;
        doubleDimensions[3] = 1;
        rasHDimensions = transform->TransformDoublePoint( doubleDimensions);
        for (int n=0; n<3; n++) {
          if (rasHDimensions[n] < minBounds[n])
          {
            minBounds[n] = rasHDimensions[n];
          }
          if (rasHDimensions[n] > maxBounds[n])
          {
            maxBounds[n] = rasHDimensions[n];
          }
        }
      }
    }
  }

  for ( i=0; i<3; i++)
  {
    bounds[2*i]   = minBounds[i];
    bounds[2*i+1] = maxBounds[i];
  }
}
