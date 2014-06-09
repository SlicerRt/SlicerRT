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

  This file was originally developed by Adam Rankin, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

#ifndef __vtkMRMLContourModelDisplayNode_h
#define __vtkMRMLContourModelDisplayNode_h

// Contour includes
#include "vtkSlicerContoursModuleMRMLExport.h"

// MRML includes
#include "vtkMRMLModelDisplayNode.h"

/// \brief MRML node to represent a display property of contour
///
/// vtkMRMLContourModelDisplayNode nodes stores display property of a contour
/// including reference to representation
class VTK_SLICER_CONTOURS_MODULE_MRML_EXPORT vtkMRMLContourModelDisplayNode : public vtkMRMLModelDisplayNode
{
public:
  static vtkMRMLContourModelDisplayNode *New();
  vtkTypeMacro(vtkMRMLContourModelDisplayNode, vtkMRMLModelDisplayNode);

  virtual vtkMRMLNode* CreateNodeInstance();

  /// Get node XML tag name (like Volume, Model)
  virtual const char* GetNodeTagName() {return "ContourModelDisplay";};

protected:
  vtkMRMLContourModelDisplayNode();
  ~vtkMRMLContourModelDisplayNode();
  vtkMRMLContourModelDisplayNode(const vtkMRMLContourModelDisplayNode&);
  void operator=(const vtkMRMLContourModelDisplayNode&);
};

#endif
