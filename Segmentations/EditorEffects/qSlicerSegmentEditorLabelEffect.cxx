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
#include "qSlicerSegmentEditorLabelEffect_p.h"

#include "vtkOrientedImageData.h"
#include "vtkOrientedImageDataResample.h"
#include "vtkMRMLSegmentationNode.h"
#include "vtkMRMLSegmentEditorNode.h"
#include "vtkSlicerSegmentationsModuleLogic.h"

// Qt includes
#include <QDebug>
#include <QCheckBox>
#include <QLabel>

// CTK includes
#include "ctkRangeWidget.h"

// VTK includes
#include <vtkMatrix4x4.h>
#include <vtkImageConstantPad.h>
#include <vtkImageMask.h>
#include <vtkImageThreshold.h>

// MRML includes
#include "vtkMRMLScalarVolumeNode.h"
#include "vtkMRMLTransformNode.h"

//-----------------------------------------------------------------------------
// qSlicerSegmentEditorLabelEffectPrivate methods

//-----------------------------------------------------------------------------
qSlicerSegmentEditorLabelEffectPrivate::qSlicerSegmentEditorLabelEffectPrivate(qSlicerSegmentEditorLabelEffect& object)
  : q_ptr(&object)
  , PaintOverCheckbox(NULL)
  , ThresholdPaintCheckbox(NULL)
  , ThresholdLabel(NULL)
  , ThresholdRangeWidget(NULL)
{
}

//-----------------------------------------------------------------------------
qSlicerSegmentEditorLabelEffectPrivate::~qSlicerSegmentEditorLabelEffectPrivate()
{
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorLabelEffectPrivate::masterVolumeScalarRange(double& low, double& high)
{
  Q_Q(qSlicerSegmentEditorLabelEffect);

  low = 0.0;
  high = 0.0;

  if (!q->parameterSetNode())
  {
    qCritical() << "qSlicerSegmentEditorLabelEffectPrivate::masterVolumeScalarRange: Invalid segment editor parameter set node!";
    return;
  }

  vtkMRMLScalarVolumeNode* masterVolumeNode = q->parameterSetNode()->GetMasterVolumeNode();
  if (!masterVolumeNode)
  {
    qCritical() << "qSlicerSegmentEditorLabelEffectPrivate::masterVolumeScalarRange: Failed to get master volume!";
    return;
  }
  if (masterVolumeNode->GetImageData())
  {
    double range[2] = {0.0, 0.0};
    masterVolumeNode->GetImageData()->GetScalarRange(range);
    low = range[0];
    high = range[1];
  }
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorLabelEffectPrivate::applyMaskImage(vtkOrientedImageData* input, vtkOrientedImageData* mask, int notMask)
{
  if (!input || !mask)
  {
    qCritical() << "qSlicerSegmentEditorLabelEffectPrivate::applyMaskImage: Invalid inputs!";
    return;
  }

  // Make sure mask has the same lattice as the edited editedLabelmap
  vtkSmartPointer<vtkOrientedImageData> resampledMask = vtkSmartPointer<vtkOrientedImageData>::New();
  vtkOrientedImageDataResample::ResampleOrientedImageToReferenceOrientedImage(
    mask, input, resampledMask);

  // Make sure mask has the same extent as the edited editedLabelmap
  vtkSmartPointer<vtkImageConstantPad> padder = vtkSmartPointer<vtkImageConstantPad>::New();
  padder->SetInputData(mask);
  padder->SetOutputWholeExtent(input->GetExtent());
  padder->Update();
  mask->DeepCopy(padder->GetOutput());

  // Apply mask
  vtkSmartPointer<vtkImageMask> masker = vtkSmartPointer<vtkImageMask>::New();
  masker->SetImageInputData(input);
  masker->SetMaskInputData(resampledMask);
  masker->SetNotMask(notMask);
  masker->SetMaskedOutputValue(0);
  masker->Update();
  input->DeepCopy(masker->GetOutput());
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorLabelEffectPrivate::onThresholdChecked(bool checked)
{
  Q_Q(qSlicerSegmentEditorLabelEffect);

  this->ThresholdLabel->setVisible(checked);

  this->ThresholdRangeWidget->blockSignals(true);
  this->ThresholdRangeWidget->setVisible(checked);
  this->ThresholdRangeWidget->blockSignals(false);

  q->updateMRMLFromGUI();
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorLabelEffectPrivate::onThresholdValuesChanged(double min, double max)
{
  Q_Q(qSlicerSegmentEditorLabelEffect);

  q->updateMRMLFromGUI();
}

//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// qSlicerSegmentEditorLabelEffect methods

//----------------------------------------------------------------------------
qSlicerSegmentEditorLabelEffect::qSlicerSegmentEditorLabelEffect(QObject* parent)
 : Superclass(parent)
 , d_ptr( new qSlicerSegmentEditorLabelEffectPrivate(*this) )
{
}

//----------------------------------------------------------------------------
qSlicerSegmentEditorLabelEffect::~qSlicerSegmentEditorLabelEffect()
{
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorLabelEffect::editedLabelmapChanged()
{
  if (!this->parameterSetNode())
  {
    qCritical() << "qSlicerSegmentEditorPaintEffect::editedLabelmapChanged: Invalid segment editor parameter set node!";
    return;
  }

  vtkOrientedImageData* editedLabelmap = this->parameterSetNode()->GetEditedLabelmap();
  vtkMRMLSegmentationNode* segmentationNode = this->parameterSetNode()->GetSegmentationNode();
  if (!editedLabelmap || !segmentationNode)
  {
    qCritical() << "qSlicerSegmentEditorPaintEffect::editedLabelmapChanged: Invalid segment selection!";
    return;
  }

  // Update mask
  vtkOrientedImageData* maskLabelmap = this->parameterSetNode()->GetMaskLabelmap();
  vtkSmartPointer<vtkMatrix4x4> mergedImageToWorldMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  segmentationNode->GenerateMergedLabelmap(maskLabelmap, mergedImageToWorldMatrix, editedLabelmap);
  maskLabelmap->SetGeometryFromImageToWorldMatrix(mergedImageToWorldMatrix);

  // Set displayed image data to segmentation node
  //TODO:
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorLabelEffect::masterVolumeNodeChanged()
{
  Q_D(qSlicerSegmentEditorLabelEffect);

  if (!this->parameterSetNode())
  {
    qCritical() << "qSlicerSegmentEditorLabelEffect::masterVolumeNodeChanged: Invalid segment editor parameter set node!";
    return;
  }

  double low = 0.0;
  double high = 1000.0;

  vtkMRMLScalarVolumeNode* masterVolumeNode = this->parameterSetNode()->GetMasterVolumeNode();
  if (masterVolumeNode)
  {
    d->masterVolumeScalarRange(low, high);
  }

  d->ThresholdRangeWidget->setMinimum(low);
  d->ThresholdRangeWidget->setMaximum(high);
  this->setParameter(this->paintThresholdMinParameterName(), low);
  this->setParameter(this->paintThresholdMaxParameterName(), high);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorLabelEffect::setupOptionsFrame()
{
  Q_D(qSlicerSegmentEditorLabelEffect);

  d->PaintOverCheckbox = new QCheckBox("Paint over");
  d->PaintOverCheckbox->setToolTip("Allow effect to overwrite non-zero labels.");
  this->addOptionsWidget(d->PaintOverCheckbox);

  d->ThresholdPaintCheckbox = new QCheckBox("Threshold paint");
  d->ThresholdPaintCheckbox->setToolTip("Enable/Disable threshold mode for labeling");
  this->addOptionsWidget(d->ThresholdPaintCheckbox);

  d->ThresholdLabel = new QLabel("Threshold");
  d->ThresholdLabel->setToolTip("In threshold mode, the label will only be set if the background value is within this range.");
  this->addOptionsWidget(d->ThresholdLabel);

  d->ThresholdRangeWidget = new ctkRangeWidget();
  d->ThresholdRangeWidget->setSpinBoxAlignment(Qt::AlignTop);
  d->ThresholdRangeWidget->setSingleStep(0.01);
  this->addOptionsWidget(d->ThresholdRangeWidget);

  QObject::connect(d->PaintOverCheckbox, SIGNAL(clicked()), this, SLOT(updateMRMLFromGUI()));
  QObject::connect(d->ThresholdPaintCheckbox, SIGNAL(toggled(bool)), d, SLOT(onThresholdChecked(bool)));
  QObject::connect(d->ThresholdRangeWidget, SIGNAL(valuesChanged(double,double)), d, SLOT(onThresholdValuesChanged(double,double)));
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorLabelEffect::setMRMLDefaults()
{
  this->setParameter(this->paintOverParameterName(), 1);
  this->setParameter(this->paintThresholdParameterName(), 0);
  this->setParameter(this->paintThresholdMinParameterName(), 0);
  this->setParameter(this->paintThresholdMaxParameterName(), 1000);
  this->setParameter(this->thresholdAvailableParameterName(), 1);
  this->setParameter(this->paintOverAvailableParameterName(), 1);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorLabelEffect::updateGUIFromMRML()
{
  Q_D(qSlicerSegmentEditorLabelEffect);

  d->PaintOverCheckbox->blockSignals(true);
  d->PaintOverCheckbox->setChecked(this->integerParameter(this->paintOverParameterName()));
  d->PaintOverCheckbox->setVisible(this->integerParameter(this->paintOverAvailableParameterName()));
  d->PaintOverCheckbox->blockSignals(false);

  d->ThresholdPaintCheckbox->blockSignals(true);
  d->ThresholdPaintCheckbox->setChecked(this->integerParameter(this->paintThresholdParameterName()));
  d->ThresholdPaintCheckbox->setVisible(this->integerParameter(this->thresholdAvailableParameterName()));
  d->ThresholdPaintCheckbox->blockSignals(false);

  bool thresholdEnabled = d->ThresholdPaintCheckbox->isChecked() && (bool)this->integerParameter(this->thresholdAvailableParameterName());
  d->ThresholdLabel->setVisible(thresholdEnabled);

  d->ThresholdRangeWidget->blockSignals(true);
  d->ThresholdRangeWidget->setMinimumValue(this->doubleParameter(this->paintThresholdMinParameterName()));
  d->ThresholdRangeWidget->setMaximumValue(this->doubleParameter(this->paintThresholdMaxParameterName()));
  d->ThresholdRangeWidget->setVisible(thresholdEnabled);
  d->ThresholdRangeWidget->blockSignals(false);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorLabelEffect::updateMRMLFromGUI()
{
  Q_D(qSlicerSegmentEditorLabelEffect);

  this->setParameter(this->paintOverParameterName(), (int)d->PaintOverCheckbox->isChecked());
  this->setParameter(this->paintThresholdParameterName(), (int)d->ThresholdPaintCheckbox->isChecked());
  this->setParameter(this->paintThresholdMinParameterName(), (double)d->ThresholdRangeWidget->minimumValue());
  this->setParameter(this->paintThresholdMaxParameterName(), (double)d->ThresholdRangeWidget->maximumValue());
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorLabelEffect::apply()
{
  Q_D(qSlicerSegmentEditorLabelEffect);

  if (!this->parameterSetNode())
  {
    qCritical() << "qSlicerSegmentEditorLabelEffect::apply: Invalid segment editor parameter set node!";
    return;
  }

  vtkOrientedImageData* editedLabelmap = this->parameterSetNode()->GetEditedLabelmap();
  vtkOrientedImageData* maskLabelmap = this->parameterSetNode()->GetMaskLabelmap();

  // Apply mask to edited editedLabelmap if paint over is turned off
  if (!this->integerParameter(this->paintOverParameterName()))
  {
    d->applyMaskImage(editedLabelmap, maskLabelmap, 1);
  }

  // Apply threshold mask if paint threshold is turned on
  if (this->integerParameter(this->paintThresholdParameterName()))
  {
    vtkMRMLScalarVolumeNode* masterVolumeNode = this->parameterSetNode()->GetMasterVolumeNode();
    if (!masterVolumeNode)
    {
      qCritical() << "qSlicerSegmentEditorPaintEffect::apply: Invalid master volume!";
      return;
    }
    vtkSmartPointer<vtkOrientedImageData> masterVolumeOrientedImageData = vtkSmartPointer<vtkOrientedImageData>::Take(
      vtkSlicerSegmentationsModuleLogic::CreateOrientedImageDataFromVolumeNode(masterVolumeNode) );
    if (!masterVolumeOrientedImageData)
    {
      qCritical() << "qSlicerSegmentEditorPaintEffect::apply: Unable to get master volume image!";
      return;
    }
    // Make sure the edited labelmap has the same geometry as the master volume
    if (!vtkOrientedImageDataResample::DoGeometriesMatch(editedLabelmap, masterVolumeOrientedImageData))
    {
      qCritical() << "qSlicerSegmentEditorPaintEffect::apply: Edited labelmap should have the same geometry as the master volume!";
      return;
    }

    // Get color table index for the edited segment
    int segmentColorIndex = 3; //TODO
    
    // Create threshold image
    vtkSmartPointer<vtkImageThreshold> threshold = vtkSmartPointer<vtkImageThreshold>::New();
    threshold->SetInputData(masterVolumeOrientedImageData);
    threshold->ThresholdBetween(
      this->doubleParameter(this->paintThresholdMinParameterName()),
      this->doubleParameter(this->paintThresholdMaxParameterName()) );
    threshold->SetInValue(segmentColorIndex);
    threshold->SetOutValue(0);
    threshold->SetOutputScalarType(editedLabelmap->GetScalarType());
    threshold->Update();

    vtkSmartPointer<vtkOrientedImageData> thresholdMask = vtkSmartPointer<vtkOrientedImageData>::New();
    thresholdMask->DeepCopy(threshold->GetOutput());
    vtkSmartPointer<vtkMatrix4x4> editedLabelmapToWorldMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
    editedLabelmap->GetImageToWorldMatrix(editedLabelmapToWorldMatrix);
    thresholdMask->SetGeometryFromImageToWorldMatrix(editedLabelmapToWorldMatrix);

    d->applyMaskImage(editedLabelmap, thresholdMask, 0);
  }

  // Notify editor about changes
  Superclass::apply();
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
