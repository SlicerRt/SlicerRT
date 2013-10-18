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

#ifndef __vtkMRMLPlanarImageNode_h
#define __vtkMRMLPlanarImageNode_h

// MRML includes
#include <vtkMRML.h>
#include <vtkMRMLNode.h>

#include "vtkSlicerPlanarImageModuleLogicExport.h"

class vtkMRMLScalarVolumeNode;
class vtkMRMLModelNode;

class VTK_SLICER_PLANARIMAGE_LOGIC_EXPORT vtkMRMLPlanarImageNode : public vtkMRMLNode
{
public:
  static vtkMRMLPlanarImageNode *New();
  vtkTypeMacro(vtkMRMLPlanarImageNode,vtkMRMLNode);

  /// Create instance of a node
  virtual vtkMRMLNode* CreateNodeInstance();

  /// Get unique node XML tag name (like Volume, Model) 
  virtual const char* GetNodeTagName() {return "PlanarImage";};

public:
  /// Get RT image volume node
  vtkMRMLScalarVolumeNode* GetRtImageVolumeNode();
  /// Set and observe RT image volume node
  void SetAndObserveRtImageVolumeNode(vtkMRMLScalarVolumeNode* node);

  /// Get displayed model node
  vtkMRMLModelNode* GetDisplayedModelNode();
  /// Set and observe displayed model node
  void SetAndObserveDisplayedModelNode(vtkMRMLModelNode* node);

  /// Get texture volume node
  vtkMRMLScalarVolumeNode* GetTextureVolumeNode();
  /// Set and observe texture volume node
  void SetAndObserveTextureVolumeNode(vtkMRMLScalarVolumeNode* node);

protected:
  vtkMRMLPlanarImageNode();
  ~vtkMRMLPlanarImageNode();
  vtkMRMLPlanarImageNode(const vtkMRMLPlanarImageNode&);
  void operator=(const vtkMRMLPlanarImageNode&);
};

#endif
