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
#include "qSlicerSegmentEditorAbstractLabelEffect.h"
#include "qSlicerSegmentEditorAbstractLabelEffect_p.h"

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
#include <vtkTransform.h>
#include <vtkImageConstantPad.h>
#include <vtkImageMask.h>
#include <vtkImageThreshold.h>
#include <vtkPolyData.h>
#include <vtkImageMathematics.h>

// Slicer includes
#include "qMRMLSliceWidget.h"
#include "vtkImageFillROI.h"

// MRML includes
#include "vtkMRMLScalarVolumeNode.h"
#include "vtkMRMLTransformNode.h"
#include "vtkMRMLSliceNode.h"

//-----------------------------------------------------------------------------
// qSlicerSegmentEditorAbstractLabelEffectPrivate methods

//-----------------------------------------------------------------------------
qSlicerSegmentEditorAbstractLabelEffectPrivate::qSlicerSegmentEditorAbstractLabelEffectPrivate(qSlicerSegmentEditorAbstractLabelEffect& object)
  : q_ptr(&object)
  , PaintOverCheckbox(NULL)
  , ThresholdPaintCheckbox(NULL)
  , ThresholdLabel(NULL)
  , ThresholdRangeWidget(NULL)
{
}

//-----------------------------------------------------------------------------
qSlicerSegmentEditorAbstractLabelEffectPrivate::~qSlicerSegmentEditorAbstractLabelEffectPrivate()
{
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractLabelEffectPrivate::onThresholdChecked(bool checked)
{
  Q_Q(qSlicerSegmentEditorAbstractLabelEffect);

  this->ThresholdLabel->setVisible(checked);

  this->ThresholdRangeWidget->blockSignals(true);
  this->ThresholdRangeWidget->setVisible(checked);
  this->ThresholdRangeWidget->blockSignals(false);

  q->updateMRMLFromGUI();
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractLabelEffectPrivate::onThresholdValuesChanged(double min, double max)
{
  Q_Q(qSlicerSegmentEditorAbstractLabelEffect);

  q->updateMRMLFromGUI();
}

//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// qSlicerSegmentEditorAbstractLabelEffect methods

//----------------------------------------------------------------------------
qSlicerSegmentEditorAbstractLabelEffect::qSlicerSegmentEditorAbstractLabelEffect(QObject* parent)
 : Superclass(parent)
 , d_ptr( new qSlicerSegmentEditorAbstractLabelEffectPrivate(*this) )
{
}

//----------------------------------------------------------------------------
qSlicerSegmentEditorAbstractLabelEffect::~qSlicerSegmentEditorAbstractLabelEffect()
{
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractLabelEffect::editedLabelmapChanged()
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
  if (!this->parameterSetNode()->GetSelectedSegmentID())
  {
    return;
  }

  // Collect all segment IDs except for the current one for the mask
  std::vector<std::string> maskSegmentIDs;
  segmentationNode->GetSegmentation()->GetSegmentIDs(maskSegmentIDs);
  std::string editedSegmentID(this->parameterSetNode()->GetSelectedSegmentID());
  maskSegmentIDs.erase(std::remove(maskSegmentIDs.begin(), maskSegmentIDs.end(), editedSegmentID), maskSegmentIDs.end());

  // Update mask
  vtkOrientedImageData* maskLabelmap = this->parameterSetNode()->GetMaskLabelmap();
  vtkSmartPointer<vtkMatrix4x4> mergedImageToWorldMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  segmentationNode->GenerateMergedLabelmap(maskLabelmap, mergedImageToWorldMatrix, editedLabelmap, maskSegmentIDs);
  maskLabelmap->SetGeometryFromImageToWorldMatrix(mergedImageToWorldMatrix);

  vtkSmartPointer<vtkImageThreshold> threshold = vtkSmartPointer<vtkImageThreshold>::New();
  threshold->SetInputData(maskLabelmap);
  threshold->SetInValue(255);
  threshold->SetOutValue(0);
  threshold->ReplaceInOn();
  threshold->ThresholdBetween(1, 254);
  threshold->SetOutputScalarType(VTK_SHORT);
  threshold->Update();
  maskLabelmap->DeepCopy(threshold->GetOutput());

  //TODO: Show mask?
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractLabelEffect::masterVolumeNodeChanged()
{
  Q_D(qSlicerSegmentEditorAbstractLabelEffect);

  if (!this->parameterSetNode())
  {
    qCritical() << "qSlicerSegmentEditorAbstractLabelEffect::masterVolumeNodeChanged: Invalid segment editor parameter set node!";
    return;
  }

  double low = 0.0;
  double high = 1000.0;

  vtkMRMLScalarVolumeNode* masterVolumeNode = this->parameterSetNode()->GetMasterVolumeNode();
  if (masterVolumeNode)
  {
    this->masterVolumeScalarRange(low, high);
  }

  d->ThresholdRangeWidget->setMinimum(low);
  d->ThresholdRangeWidget->setMaximum(high);
  this->setCommonParameter(this->paintThresholdMinParameterName(), low);
  this->setCommonParameter(this->paintThresholdMaxParameterName(), high);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractLabelEffect::setupOptionsFrame()
{
  Q_D(qSlicerSegmentEditorAbstractLabelEffect);

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
void qSlicerSegmentEditorAbstractLabelEffect::setMRMLDefaults()
{
  this->setCommonParameter(this->paintOverParameterName(), 1);
  this->setCommonParameter(this->paintThresholdParameterName(), 0);
  this->setCommonParameter(this->paintThresholdMinParameterName(), 0);
  this->setCommonParameter(this->paintThresholdMaxParameterName(), 1000);
  this->setCommonParameter(this->thresholdAvailableParameterName(), 1);
  this->setCommonParameter(this->paintOverAvailableParameterName(), 1);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractLabelEffect::updateGUIFromMRML()
{
  Q_D(qSlicerSegmentEditorAbstractLabelEffect);

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
void qSlicerSegmentEditorAbstractLabelEffect::updateMRMLFromGUI()
{
  Q_D(qSlicerSegmentEditorAbstractLabelEffect);

  this->setCommonParameter(this->paintOverParameterName(), (int)d->PaintOverCheckbox->isChecked());
  this->setCommonParameter(this->paintThresholdParameterName(), (int)d->ThresholdPaintCheckbox->isChecked());
  this->setCommonParameter(this->paintThresholdMinParameterName(), (double)d->ThresholdRangeWidget->minimumValue());
  this->setCommonParameter(this->paintThresholdMaxParameterName(), (double)d->ThresholdRangeWidget->maximumValue());
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractLabelEffect::apply()
{
  Q_D(qSlicerSegmentEditorAbstractLabelEffect);

  if (!this->parameterSetNode())
  {
    qCritical() << "qSlicerSegmentEditorAbstractLabelEffect::apply: Invalid segment editor parameter set node!";
    return;
  }

  vtkOrientedImageData* editedLabelmap = this->parameterSetNode()->GetEditedLabelmap();
  vtkOrientedImageData* maskLabelmap = this->parameterSetNode()->GetMaskLabelmap();

  // Apply mask to edited labelmap if paint over is turned off
  if (!this->integerParameter(this->paintOverParameterName()))
  {
    this->applyImageMask(editedLabelmap, maskLabelmap, true);
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
    
    // Create threshold image
    vtkSmartPointer<vtkImageThreshold> threshold = vtkSmartPointer<vtkImageThreshold>::New();
    threshold->SetInputData(masterVolumeOrientedImageData);
    threshold->ThresholdBetween(
      this->doubleParameter(this->paintThresholdMinParameterName()),
      this->doubleParameter(this->paintThresholdMaxParameterName()) );
    threshold->SetInValue(1);
    threshold->SetOutValue(0);
    threshold->SetOutputScalarType(editedLabelmap->GetScalarType());
    threshold->Update();

    vtkSmartPointer<vtkOrientedImageData> thresholdMask = vtkSmartPointer<vtkOrientedImageData>::New();
    thresholdMask->DeepCopy(threshold->GetOutput());
    vtkSmartPointer<vtkMatrix4x4> editedLabelmapToWorldMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
    editedLabelmap->GetImageToWorldMatrix(editedLabelmapToWorldMatrix);
    thresholdMask->SetGeometryFromImageToWorldMatrix(editedLabelmapToWorldMatrix);

    this->applyImageMask(editedLabelmap, thresholdMask);
  }

  // Notify editor about changes
  Superclass::apply();
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractLabelEffect::applyImageMask(
  vtkOrientedImageData* input, vtkOrientedImageData* mask,
  bool notMask/*=false*/ )
{
  if (!input || !mask)
  {
    qCritical() << "qSlicerSegmentEditorAbstractLabelEffect::applyImageMask: Invalid inputs!";
    return;
  }

  // Make sure mask has the same lattice as the edited labelmap
  vtkSmartPointer<vtkOrientedImageData> resampledMask = vtkSmartPointer<vtkOrientedImageData>::New();
  vtkOrientedImageDataResample::ResampleOrientedImageToReferenceOrientedImage(
    mask, input, resampledMask);

  // Make sure mask has the same extent as the edited labelmap
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
void qSlicerSegmentEditorAbstractLabelEffect::appendPolyMask(vtkOrientedImageData* input, vtkPolyData* polyData, qMRMLSliceWidget* sliceWidget)
{
  // Rasterize a poly data onto the input image into the slice view
  // - Points are specified in current XY space
  vtkSmartPointer<vtkOrientedImageData> polyMaskImage = vtkSmartPointer<vtkOrientedImageData>::New();
  qSlicerSegmentEditorAbstractLabelEffect::createMaskImageFromPolyData(polyData, polyMaskImage, sliceWidget);

  // Append poly mask onto input image
  qSlicerSegmentEditorAbstractLabelEffect::appendImage(input, polyMaskImage);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractLabelEffect::appendImage(vtkOrientedImageData* inputImage, vtkOrientedImageData* appendedImage)
{
  if (!inputImage || !appendedImage)
  {
    qCritical() << "qSlicerSegmentEditorAbstractLabelEffect::appendImages: Invalid inputs!";
    return;
  }

  // Make sure appended image has the same lattice as the input image
  vtkSmartPointer<vtkOrientedImageData> resampledAppendedImage = vtkSmartPointer<vtkOrientedImageData>::New();
  vtkOrientedImageDataResample::ResampleOrientedImageToReferenceOrientedImage(
    appendedImage, inputImage, resampledAppendedImage);

  // Add image created from poly data to input image
  vtkSmartPointer<vtkImageMathematics> imageMath = vtkSmartPointer<vtkImageMathematics>::New();
  imageMath->SetInput1Data(inputImage);
  imageMath->SetInput2Data(resampledAppendedImage);
  imageMath->SetOperationToMax();
  imageMath->Update();
  inputImage->DeepCopy(imageMath->GetOutput());
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractLabelEffect::createMaskImageFromPolyData(vtkPolyData* polyData, vtkOrientedImageData* outputMask, qMRMLSliceWidget* sliceWidget)
{
  if (!polyData || !outputMask)
  {
    qCritical() << "qSlicerSegmentEditorAbstractLabelEffect::createMaskImageFromPolyData: Invalid inputs!";
    return;
  }
  vtkMRMLSliceNode* sliceNode = vtkMRMLSliceNode::SafeDownCast(
    qSlicerSegmentEditorAbstractEffect::viewNode(sliceWidget) );
  if (!sliceNode)
  {
    qCritical() << "qSlicerSegmentEditorAbstractLabelEffect::createMaskImageFromPolyData: Failed to get slice node!";
    return;
  }

  // Need to know the mapping from RAS into polygon space
  // so the painter can use this as a mask
  // - Need the bounds in RAS space
  // - Need to get an IJKToRAS for just the mask area
  // - Directions are the XYToRAS for this slice
  // - Origin is the lower left of the polygon bounds
  // - TODO: need to account for the boundary pixels
  //
  // Note: uses the slicer2-based vtkImageFillROI filter
  vtkSmartPointer<vtkMatrix4x4> maskIjkToRasMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  maskIjkToRasMatrix->DeepCopy(sliceNode->GetXYToRAS());

  polyData->GetPoints()->Modified();
  double bounds[6] = {0,0,0,0,0,0};
  polyData->GetBounds(bounds);

  double xlo = bounds[0] - 1.0;
  double xhi = bounds[1];
  double ylo = bounds[2] - 1.0;
  double yhi = bounds[3];

  double originXYZ[3] = {xlo, ylo, 0.0};
  double originRAS[3] = {0.0,0.0,0.0};
  qSlicerSegmentEditorAbstractEffect::xyzToRas(originXYZ, originRAS, sliceWidget);

  maskIjkToRasMatrix->SetElement(0, 3, originRAS[0]);
  maskIjkToRasMatrix->SetElement(1, 3, originRAS[1]);
  maskIjkToRasMatrix->SetElement(2, 3, originRAS[2]);

  // Get a good size for the draw buffer
  // - Needs to include the full region of the polygon
  // - Plus a little extra
  //
  // Round to int and add extra pixel for both sides
  // TODO: figure out why we need to add buffer pixels on each
  //   side for the width in order to end up with a single extra
  //   pixel in the rasterized image map.  Probably has to
  //   do with how boundary conditions are handled in the filler
  int w = (int)(xhi - xlo) + 32;
  int h = (int)(yhi - ylo) + 32;

  vtkSmartPointer<vtkOrientedImageData> imageData = vtkSmartPointer<vtkOrientedImageData>::New();
  imageData->SetDimensions(w, h, 1);
  imageData->AllocateScalars(VTK_UNSIGNED_CHAR, 1);

  // Move the points so the lower left corner of the bounding box is at 1, 1 (to avoid clipping)
  vtkSmartPointer<vtkTransform> translate = vtkSmartPointer<vtkTransform>::New();
  translate->Translate(-xlo, -ylo, 0.0);

  vtkSmartPointer<vtkPoints> drawPoints = vtkSmartPointer<vtkPoints>::New();
  drawPoints->Reset();
  translate->TransformPoints(polyData->GetPoints(), drawPoints);
  drawPoints->Modified();

  vtkSmartPointer<vtkImageFillROI> fill = vtkSmartPointer<vtkImageFillROI>::New();
  fill->SetInputData(imageData);
  fill->SetValue(1);
  fill->SetPoints(drawPoints);
  fill->Update();

  outputMask->DeepCopy(fill->GetOutput());
  outputMask->SetGeometryFromImageToWorldMatrix(maskIjkToRasMatrix);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractLabelEffect::imageToWorldMatrix(vtkMRMLVolumeNode* node, vtkMatrix4x4* ijkToRas)
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
      qCritical() << "qSlicerSegmentEditorAbstractLabelEffect::ijkToRasMatrix: Parent transform is non-linear, which cannot be handled! Skipping.";
    }
  }
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractLabelEffect::imageToWorldMatrix(vtkOrientedImageData* image, vtkMRMLSegmentationNode* node, vtkMatrix4x4* ijkToRas)
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
      qCritical() << "qSlicerSegmentEditorAbstractLabelEffect::ijkToRasMatrix: Parent transform is non-linear, which cannot be handled! Skipping.";
    }
  }
}
