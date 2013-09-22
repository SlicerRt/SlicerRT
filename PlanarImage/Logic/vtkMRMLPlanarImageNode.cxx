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

// MRMLPlanarImage includes
#include "vtkMRMLPlanarImageNode.h"

// SlicerRt includes
#include "SlicerRtCommon.h"

// VTK includes
#include <vtkObjectFactory.h>

//------------------------------------------------------------------------------
const char* vtkMRMLPlanarImageNode::PlanarImageVolumeNodeReferenceRole = "planarImage";
const char* vtkMRMLPlanarImageNode::DisplayedModelNodeReferenceRole = SlicerRtCommon::PLANARIMAGE_DISPLAYED_MODEL_REFERENCE_ROLE;
const char* vtkMRMLPlanarImageNode::TextureVolumeNodeReferenceRole = SlicerRtCommon::PLANARIMAGE_TEXTURE_REFERENCE_ROLE;

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLPlanarImageNode);

//----------------------------------------------------------------------------
vtkMRMLPlanarImageNode::vtkMRMLPlanarImageNode()
{
  this->HideFromEditors = false;

  std::string planarImageVolumeNodeReferenceRoleAttributeName = std::string(PlanarImageVolumeNodeReferenceRole) + SlicerRtCommon::SLICERRT_REFERENCE_ROLE_ATTRIBUTE_NAME_POSTFIX;
  this->AddNodeReferenceRole(PlanarImageVolumeNodeReferenceRole, planarImageVolumeNodeReferenceRoleAttributeName.c_str());
  std::string displayedModelNodeReferenceRoleAttributeName = std::string(DisplayedModelNodeReferenceRole) + SlicerRtCommon::SLICERRT_REFERENCE_ROLE_ATTRIBUTE_NAME_POSTFIX;
  this->AddNodeReferenceRole(DisplayedModelNodeReferenceRole, displayedModelNodeReferenceRoleAttributeName.c_str());
  std::string textureVolumeNodeReferenceRoleAttributeName = std::string(TextureVolumeNodeReferenceRole) + SlicerRtCommon::SLICERRT_REFERENCE_ROLE_ATTRIBUTE_NAME_POSTFIX;
  this->AddNodeReferenceRole(TextureVolumeNodeReferenceRole, textureVolumeNodeReferenceRoleAttributeName.c_str());
}

//----------------------------------------------------------------------------
vtkMRMLPlanarImageNode::~vtkMRMLPlanarImageNode()
{
}
