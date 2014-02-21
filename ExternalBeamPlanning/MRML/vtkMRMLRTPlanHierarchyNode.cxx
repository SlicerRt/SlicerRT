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

// SlicerRtCommon includes
#include "SlicerRtCommon.h"
#include "vtkMRMLRTPlanHierarchyNode.h"

// MRML includes
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLModelNode.h>

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkIntArray.h>

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLRTPlanHierarchyNode);

//----------------------------------------------------------------------------
vtkMRMLRTPlanHierarchyNode::vtkMRMLRTPlanHierarchyNode()
{
  this->HideFromEditorsOff();

  // Register parent transform modified event so that the representations
  //   can be put under the same transform node
  vtkSmartPointer<vtkIntArray> events = vtkSmartPointer<vtkIntArray>::New();
  events->InsertNextValue(vtkMRMLTransformableNode::TransformModifiedEvent);
  vtkObserveMRMLObjectEventsMacro(this, events);
}

//----------------------------------------------------------------------------
vtkMRMLRTPlanHierarchyNode::~vtkMRMLRTPlanHierarchyNode()
{
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLRTPlanHierarchyNode::Copy(vtkMRMLNode *anode)
{
  Superclass::Copy(anode);
  //TODO:
}

//----------------------------------------------------------------------------
void vtkMRMLRTPlanHierarchyNode::ProcessMRMLEvents(vtkObject *caller, unsigned long eventID, void *callData)
{
  Superclass::ProcessMRMLEvents(caller, eventID, callData);

  if (this->Scene == NULL)
    {
    return;
    }
  if (this->Scene->IsBatchProcessing())
    {
    return;
    }

  // Representation internal data changed
  if (eventID == vtkMRMLModelNode::PolyDataModifiedEvent || eventID == vtkMRMLVolumeNode::ImageDataModifiedEvent)
    {
    vtkMRMLModelNode* callerModelNode = vtkMRMLModelNode::SafeDownCast(caller);
    vtkMRMLScalarVolumeNode* callerVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(caller);
    if (!callerModelNode && !callerVolumeNode)
      {
      return;
      }
    //TODO: Implement or delete
    }
}

