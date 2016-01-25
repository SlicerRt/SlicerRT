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

// Segmentations includes
#include "qSlicerSegmentEditorLabelEffect.h"

#include "vtkOrientedImageData.h"
#include "vtkMRMLSegmentationNode.h"

// Qt includes
#include <QDebug>

// VTK includes
#include <vtkMatrix4x4.h>

// MRML includes
#include <vtkMRMLVolumeNode.h>
#include <vtkMRMLTransformNode.h>

//----------------------------------------------------------------------------
qSlicerSegmentEditorLabelEffect::qSlicerSegmentEditorLabelEffect(QObject* parent)
 : Superclass(parent)
{
}

//----------------------------------------------------------------------------
qSlicerSegmentEditorLabelEffect::~qSlicerSegmentEditorLabelEffect()
{
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorLabelEffect::updateGUIFromMRML(vtkObject* caller, void* callData)
{
  // Get parameter set node
  vtkMRMLSegmentEditorEffectNode* parameterNode = reinterpret_cast<vtkMRMLSegmentEditorEffectNode*>(caller);
  if (!parameterNode)
  {
    return;
  }

  //TODO: Common label effect parameters (paintOver, paintThreshold, paintThresholdMin, paintThresholdMax)
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorLabelEffect::updateMRMLFromGUI()
{
  //TODO: Common label effect parameters (paintOver, paintThreshold, paintThresholdMin, paintThresholdMax)
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorLabelEffect::ijkToRasMatrix(vtkMRMLVolumeNode* node, vtkMatrix4x4* ijkToRas)
{
  if (!node || !ijkToRas)
  {
    return;
  }

  node->GetIJKToRASMatrix(ijkToRas);

  vtkMRMLTransformNode* transformNode = node->GetParentTransformNode();
  if (transformNode)
  {
    if (transformNode->IsTransformToWorldLinear())
    {
      vtkSmartPointer<vtkMatrix4x4> volumeRasToWorldRas = vtkSmartPointer<vtkMatrix4x4>::New();
      transformNode->GetMatrixTransformToWorld(volumeRasToWorldRas);
      vtkMatrix4x4::Multiply4x4(volumeRasToWorldRas, ijkToRas, ijkToRas);
    }
    else
    {
      qCritical() << "qSlicerSegmentEditorLabelEffect::ijkToRasMatrix: Parent transform is non-linear, which cannot be handled! Skipping.";
    }
  }
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorLabelEffect::ijkToRasMatrix(vtkOrientedImageData* image, vtkMRMLSegmentationNode* node, vtkMatrix4x4* ijkToRas)
{
  if (!image || !node || !ijkToRas)
  {
    return;
  }

  image->GetImageToWorldMatrix(ijkToRas);

  vtkMRMLTransformNode* transformNode = node->GetParentTransformNode();
  if (transformNode)
  {
    if (transformNode->IsTransformToWorldLinear())
    {
      vtkSmartPointer<vtkMatrix4x4> segmentationRasToWorldRas = vtkSmartPointer<vtkMatrix4x4>::New();
      transformNode->GetMatrixTransformToWorld(segmentationRasToWorldRas);
      vtkMatrix4x4::Multiply4x4(segmentationRasToWorldRas, ijkToRas, ijkToRas);
    }
    else
    {
      qCritical() << "qSlicerSegmentEditorLabelEffect::ijkToRasMatrix: Parent transform is non-linear, which cannot be handled! Skipping.";
    }
  }
}
