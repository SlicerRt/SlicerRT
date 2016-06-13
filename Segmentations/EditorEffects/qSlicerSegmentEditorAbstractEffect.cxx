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
#include "qSlicerSegmentEditorAbstractEffect.h"
#include "qSlicerSegmentEditorAbstractEffect_p.h"

#include "vtkMRMLSegmentationNode.h"
#include "vtkMRMLSegmentationDisplayNode.h"
#include "vtkMRMLSegmentEditorNode.h"
#include "vtkOrientedImageData.h"
#include "vtkSlicerSegmentationsModuleLogic.h"

// Qt includes
#include <QDebug>
#include <QFormLayout>
#include <QImage>
#include <QLabel>
#include <QPixmap>
#include <QPainter>
#include <QPaintDevice>
#include <QFrame>
#include <QColor>

// Slicer includes
#include "qMRMLSliceWidget.h"
#include "qMRMLSliceView.h"
#include "qMRMLThreeDWidget.h"
#include "qMRMLThreeDView.h"
#include "vtkMRMLSliceLogic.h"

// MRML includes
#include "vtkMRMLScene.h"
#include "vtkMRMLSliceNode.h"
#include "vtkMRMLViewNode.h"

// VTK includes
#include <vtkImageConstantPad.h>
#include <vtkImageMask.h>
#include <vtkImageShiftScale.h>
#include <vtkImageThreshold.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkProp.h>
#include <vtkRenderer.h>
#include <vtkRendererCollection.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkSmartPointer.h>
#include <vtkWeakPointer.h>

#include <vtkOrientedImageDataResample.h>

//-----------------------------------------------------------------------------
// qSlicerSegmentEditorAbstractEffectPrivate methods

//-----------------------------------------------------------------------------
qSlicerSegmentEditorAbstractEffectPrivate::qSlicerSegmentEditorAbstractEffectPrivate(qSlicerSegmentEditorAbstractEffect& object)
  : q_ptr(&object)
  , Scene(NULL)
  , ParameterSetNode(NULL)
  , SavedCursor(QCursor(Qt::ArrowCursor))
  , OptionsFrame(NULL)
  , EditedLabelmapApplyMode(qSlicerSegmentEditorAbstractEffect::APPLY_MODE_SET)
{
  this->OptionsFrame = new QFrame();
  this->OptionsFrame->setFrameShape(QFrame::NoFrame);
  this->OptionsFrame->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding));
  QFormLayout* layout = new QFormLayout(this->OptionsFrame);
  layout->setContentsMargins(4, 4, 4, 4);
  layout->setSpacing(4);
  this->EditedLabelmapApplyExtent[0]=0;
  this->EditedLabelmapApplyExtent[1]=-1;
  this->EditedLabelmapApplyExtent[2]=0;
  this->EditedLabelmapApplyExtent[3]=-1;
  this->EditedLabelmapApplyExtent[4]=0;
  this->EditedLabelmapApplyExtent[5]=-1;
}

//-----------------------------------------------------------------------------
qSlicerSegmentEditorAbstractEffectPrivate::~qSlicerSegmentEditorAbstractEffectPrivate()
{
  if (this->OptionsFrame)
  {
    delete this->OptionsFrame;
    this->OptionsFrame = NULL;
  }
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffectPrivate::scheduleRender(qMRMLWidget* viewWidget)
{
  qMRMLSliceWidget* sliceWidget = qobject_cast<qMRMLSliceWidget*>(viewWidget);
  qMRMLThreeDWidget* threeDWidget = qobject_cast<qMRMLThreeDWidget*>(viewWidget);
  if (sliceWidget)
  {
    sliceWidget->sliceView()->scheduleRender();
  }
  else if (threeDWidget)
  {
    threeDWidget->threeDView()->scheduleRender();
  }
  else
  {
    qCritical() << Q_FUNC_INFO << ": Unsupported view widget";
  }
}

//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// qSlicerSegmentEditorAbstractEffect methods

//----------------------------------------------------------------------------
qSlicerSegmentEditorAbstractEffect::qSlicerSegmentEditorAbstractEffect(QObject* parent)
 : Superclass(parent)
 , m_Name(QString())
 , m_Active(false)
 , m_PerSegment(true)
 , m_FillValue(1.0)
 , m_EraseValue(0.0)
 , d_ptr(new qSlicerSegmentEditorAbstractEffectPrivate(*this))
{
}

//----------------------------------------------------------------------------
qSlicerSegmentEditorAbstractEffect::~qSlicerSegmentEditorAbstractEffect()
{
}

//-----------------------------------------------------------------------------
QString qSlicerSegmentEditorAbstractEffect::name()const
{
  if (m_Name.isEmpty())
    {
    qCritical() << Q_FUNC_INFO << ": Empty effect name!";
    }
  return this->m_Name;
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::setName(QString name)
{
  Q_UNUSED(name);
  qCritical() << Q_FUNC_INFO << ": Cannot set effect name by method, only in constructor!";
}

//-----------------------------------------------------------------------------
bool qSlicerSegmentEditorAbstractEffect::perSegment()const
{
  return this->m_PerSegment;
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::setPerSegment(bool perSegment)
{
  Q_UNUSED(perSegment);
  qCritical() << Q_FUNC_INFO << ": Cannot set per-segment flag by method, only in constructor!";
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::activate()
{
  Q_D(qSlicerSegmentEditorAbstractEffect);

  // Reset edited labelmap
  this->setEditedLabelmapApplyExtentToWholeExtent();
  this->setEditedLabelmapApplyModeToSet();

  // Show options frame
  d->OptionsFrame->setVisible(true);

  this->m_Active = true;
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::deactivate()
{
  Q_D(qSlicerSegmentEditorAbstractEffect);

  // Hide options frame
  d->OptionsFrame->setVisible(false);

  this->m_Active = false;
}

//-----------------------------------------------------------------------------
bool qSlicerSegmentEditorAbstractEffect::active()
{
  return m_Active;
}


//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::setCallbackSlots(QObject* receiver, const char* applySlot, const char* selectEffectSlot, const char* updateVolumeSlot)
{
  Q_D(qSlicerSegmentEditorAbstractEffect);
  QObject::connect(d, SIGNAL(applySignal()), receiver, applySlot);
  QObject::connect(d, SIGNAL(selectEffectSignal(QString)), receiver, selectEffectSlot);
  QObject::connect(d, SIGNAL(updateVolumeSignal(void*)), receiver, updateVolumeSlot);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::applyImageMask(vtkOrientedImageData* input, vtkOrientedImageData* mask, double fillValue,
  bool notMask/*=false*/)
{
  if (!input || !mask)
  {
    qCritical() << Q_FUNC_INFO << " failed: Invalid inputs";
    return;
  }

  // Make sure mask has the same lattice as the edited labelmap
  if (!vtkOrientedImageDataResample::DoGeometriesMatch(input, mask))
  {
    qCritical() << Q_FUNC_INFO << " failed: input and mask image geometry mismatch";
    return;
  }

  // Make sure mask has the same extent as the edited labelmap
  vtkSmartPointer<vtkImageConstantPad> padder = vtkSmartPointer<vtkImageConstantPad>::New();
  padder->SetInputData(mask);
  padder->SetOutputWholeExtent(input->GetExtent());
  padder->Update();
  //mask->DeepCopy(padder->GetOutput());

  // Apply mask
  vtkSmartPointer<vtkImageMask> masker = vtkSmartPointer<vtkImageMask>::New();
  masker->SetImageInputData(input);
  //masker->SetMaskInputData(resampledMask);
  masker->SetMaskInputData(padder->GetOutput());
  //masker->SetMaskInputData(mask);
  masker->SetNotMask(notMask);
  masker->SetMaskedOutputValue(fillValue);
  masker->Update();

  // Copy masked input to input
  vtkSmartPointer<vtkMatrix4x4> inputImageToWorldMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  input->GetImageToWorldMatrix(inputImageToWorldMatrix);
  input->DeepCopy(masker->GetOutput());
  input->SetGeometryFromImageToWorldMatrix(inputImageToWorldMatrix);
}


//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::apply()
{
  Q_D(qSlicerSegmentEditorAbstractEffect);

  vtkMRMLSegmentEditorNode* parameterSetNode = this->parameterSetNode();
  if (!parameterSetNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid segment editor parameter set node!";
    this->clearEditedLabelmap(); 
    return;
  }

  vtkOrientedImageData* labelmapImage = this->editedLabelmap();

  // Apply mask to edited labelmap if paint over is turned off
  if (parameterSetNode->GetMaskMode() != vtkMRMLSegmentEditorNode::PaintAllowedEverywhere)
  {
    vtkOrientedImageData* maskImage = this->maskLabelmap();
    this->applyImageMask(labelmapImage, maskImage, this->m_EraseValue, true);
  }

  // Apply threshold mask if paint threshold is turned on
  if (parameterSetNode->GetMasterVolumeIntensityMask())
  {
    vtkOrientedImageData* masterVolumeOrientedImageData = this->masterVolumeImageData();
    if (!masterVolumeOrientedImageData)
    {
      qCritical() << Q_FUNC_INFO << ": Unable to get master volume image";
      this->clearEditedLabelmap();
      return;
    }
    // Make sure the edited labelmap has the same geometry as the master volume
    if (!vtkOrientedImageDataResample::DoGeometriesMatch(labelmapImage, masterVolumeOrientedImageData))
    {
      qCritical() << Q_FUNC_INFO << ": Edited labelmap should have the same geometry as the master volume";
      this->clearEditedLabelmap();
      return;
    }

    // Create threshold image
    vtkSmartPointer<vtkImageThreshold> threshold = vtkSmartPointer<vtkImageThreshold>::New();
    threshold->SetInputData(masterVolumeOrientedImageData);
    threshold->ThresholdBetween(parameterSetNode->GetMasterVolumeIntensityMaskRange()[0], parameterSetNode->GetMasterVolumeIntensityMaskRange()[1]);
    threshold->SetInValue(1);
    threshold->SetOutValue(0);
    threshold->SetOutputScalarType(labelmapImage->GetScalarType());
    threshold->Update();

    vtkSmartPointer<vtkOrientedImageData> thresholdMask = vtkSmartPointer<vtkOrientedImageData>::New();
    thresholdMask->DeepCopy(threshold->GetOutput());
    vtkSmartPointer<vtkMatrix4x4> labelmapImageToWorldMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
    labelmapImage->GetImageToWorldMatrix(labelmapImageToWorldMatrix);
    thresholdMask->SetGeometryFromImageToWorldMatrix(labelmapImageToWorldMatrix);

    this->applyImageMask(labelmapImage, thresholdMask, this->m_EraseValue);
  }

  if (!d->ParameterSetNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid segment editor parameter set node";
    this->clearEditedLabelmap();
    return;
  }

  vtkMRMLSegmentationNode* segmentationNode = d->ParameterSetNode->GetSegmentationNode();
  const char* selectedSegmentID = d->ParameterSetNode->GetSelectedSegmentID();
  if (!segmentationNode || !selectedSegmentID)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid segment selection";
    this->clearEditedLabelmap();
    return;
  }

  if (!d->EditedLabelmap)
  {
    // If per-segment flag is off, then it is not an error (the effect itself has written it back to segmentation)
    if (this->perSegment())
    {
      qCritical() << Q_FUNC_INFO << ": Cannot apply edit operation because edited labelmap cannot be accessed";
    }
    this->clearEditedLabelmap();
    return;
  }

  // Copy the temporary padded edited labelmap to the segment.
  // Mask and threshold was already applied on edited labelmap at this point if requested.  
  int applyMode = this->editedLabelmapApplyMode();
  int* extent = this->editedLabelmapApplyExtent();
  if (extent[0]>extent[1] || extent[2]>extent[3] || extent[4]>extent[5])
  {
    // invalid extent, it means we have to work with the entire edited labelmap
    extent = NULL;
  }

  // TODO: composite editedLabelmap with threshold mask
  
  // inverter output = (input+shift)*scale
  vtkNew<vtkImageShiftScale> inverter;
  inverter->SetInputData(d->EditedLabelmap);
  inverter->SetScale(-1);
  inverter->SetShift(-m_FillValue);
  if (applyMode == qSlicerSegmentEditorAbstractEffect::APPLY_MODE_SET)
  {
    if (!vtkSlicerSegmentationsModuleLogic::SetBinaryLabelmapToSegment(
      d->EditedLabelmap, segmentationNode, selectedSegmentID, vtkSlicerSegmentationsModuleLogic::MODE_REPLACE, extent))
    {
      qCritical() << Q_FUNC_INFO << ": Failed to set edited labelmap to selected segment!";
    }
  }
  else if (applyMode == qSlicerSegmentEditorAbstractEffect::APPLY_MODE_ADD)
  {
    if (!vtkSlicerSegmentationsModuleLogic::SetBinaryLabelmapToSegment(
      d->EditedLabelmap, segmentationNode, selectedSegmentID, vtkSlicerSegmentationsModuleLogic::MODE_MERGE_MAX, extent))
    {
      qCritical() << Q_FUNC_INFO << ": Failed to set edited labelmap to selected segment!";
    }
  }
  else if (applyMode == qSlicerSegmentEditorAbstractEffect::APPLY_MODE_REMOVE)
  {
    inverter->Update();
    vtkNew<vtkOrientedImageData> invertedEditedLabelmap;
    invertedEditedLabelmap->ShallowCopy(inverter->GetOutput());
    vtkNew<vtkMatrix4x4> imageToWorldMatrix;
    d->EditedLabelmap->GetImageToWorldMatrix(imageToWorldMatrix.GetPointer());
    invertedEditedLabelmap->SetGeometryFromImageToWorldMatrix(imageToWorldMatrix.GetPointer());
    if (!vtkSlicerSegmentationsModuleLogic::SetBinaryLabelmapToSegment(
      invertedEditedLabelmap.GetPointer(), segmentationNode, selectedSegmentID, vtkSlicerSegmentationsModuleLogic::MODE_MERGE_MIN, extent))
    {
      qCritical() << Q_FUNC_INFO << ": Failed to set edited labelmap to selected segment!";
    }
  }

  std::vector<std::string> allSegmentIDs;
  segmentationNode->GetSegmentation()->GetSegmentIDs(allSegmentIDs);
  // remove selected segment, that is already handled
  allSegmentIDs.erase(std::remove(allSegmentIDs.begin(), allSegmentIDs.end(), selectedSegmentID), allSegmentIDs.end());

  std::vector<std::string> visibleSegmentIDs;
  vtkMRMLSegmentationDisplayNode* displayNode = vtkMRMLSegmentationDisplayNode::SafeDownCast(segmentationNode->GetDisplayNode());
  if (displayNode)
  {
    for (std::vector<std::string>::iterator segmentIDIt = allSegmentIDs.begin(); segmentIDIt != allSegmentIDs.end(); ++segmentIDIt)
    {
      if (displayNode->GetSegmentVisibility(*segmentIDIt))
      {
        visibleSegmentIDs.push_back(*segmentIDIt);
      }
    }
  }

  std::vector<std::string> segmentIDsToOverwrite;
  switch (this->parameterSetNode()->GetOverwriteMode())
  {
  case vtkMRMLSegmentEditorNode::OverwriteNone:
    // nothing to overwrite
    break;
  case vtkMRMLSegmentEditorNode::OverwriteVisibleSegments:
    segmentIDsToOverwrite = visibleSegmentIDs;
    break;
  case vtkMRMLSegmentEditorNode::OverwriteAllSegments:
    segmentIDsToOverwrite = allSegmentIDs;
    break;
  }

  if (!segmentIDsToOverwrite.empty())
  {
    if (applyMode == qSlicerSegmentEditorAbstractEffect::APPLY_MODE_SET
      || applyMode == qSlicerSegmentEditorAbstractEffect::APPLY_MODE_ADD)
    {
      inverter->Update();
      vtkNew<vtkOrientedImageData> invertedEditedLabelmap;
      invertedEditedLabelmap->ShallowCopy(inverter->GetOutput());
      vtkNew<vtkMatrix4x4> imageToWorldMatrix;
      d->EditedLabelmap->GetImageToWorldMatrix(imageToWorldMatrix.GetPointer());
      invertedEditedLabelmap->SetGeometryFromImageToWorldMatrix(imageToWorldMatrix.GetPointer());
      for (std::vector<std::string>::iterator segmentIDIt = segmentIDsToOverwrite.begin(); segmentIDIt != segmentIDsToOverwrite.end(); ++segmentIDIt)
      {
        if (!vtkSlicerSegmentationsModuleLogic::SetBinaryLabelmapToSegment(
          invertedEditedLabelmap.GetPointer(), segmentationNode, *segmentIDIt, vtkSlicerSegmentationsModuleLogic::MODE_MERGE_MIN, extent))
        {
          qCritical() << Q_FUNC_INFO << ": Failed to set edited labelmap to segment " << (segmentIDIt->c_str());
        }
      }
    }
    else if (applyMode == qSlicerSegmentEditorAbstractEffect::APPLY_MODE_REMOVE)
    {
      // In general, we don't try to "add back" areas to other segments when an area is removed from the selected segment.
      // The only exception is when we draw inside one specific segment. In that case erasing adds to the mask segment. It is useful
      // for splitting a segment into two by painting.
      if (this->parameterSetNode()->GetMaskMode() == vtkMRMLSegmentEditorNode::PaintAllowedInsideSingleSegment
        && this->parameterSetNode()->GetMaskSegmentID())
      {
        if (!vtkSlicerSegmentationsModuleLogic::SetBinaryLabelmapToSegment(
          d->EditedLabelmap, segmentationNode, this->parameterSetNode()->GetMaskSegmentID(), vtkSlicerSegmentationsModuleLogic::MODE_MERGE_MAX, extent))
        {
          qCritical() << Q_FUNC_INFO << ": Failed to set edited labelmap to segment " << this->parameterSetNode()->GetMaskSegmentID();
        }
      }
    }
  }

  // Update is completed, clear the edited labelmap
  this->clearEditedLabelmap();

  emit d->applySignal();
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::clearEditedLabelmap()
{
  Q_D(qSlicerSegmentEditorAbstractEffect);
  if (d->EditedLabelmap)
  {
    int* extent = this->editedLabelmapApplyExtent();
    if (extent[0]>extent[1] || extent[2]>extent[3] || extent[4]>extent[5])
    {
      // invalid extent, it means we have to work with the entire edited labelmap
      extent = NULL;
    }
    vtkOrientedImageDataResample::FillImage(d->EditedLabelmap, this->m_EraseValue, extent);
  }
  this->setEditedLabelmapApplyExtent(0, -1, 0, -1, 0, -1);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::selectEffect(QString effectName)
{
  Q_D(qSlicerSegmentEditorAbstractEffect);
  emit d->selectEffectSignal(effectName);
}

//-----------------------------------------------------------------------------
QCursor qSlicerSegmentEditorAbstractEffect::createCursor(qMRMLWidget* viewWidget)
{
  Q_UNUSED(viewWidget); // The default cursor is not view-specific, but this method can be overridden

  QImage baseImage(":Icons/CursorBaseArrow.png");
  QIcon effectIcon(this->icon());
  if (effectIcon.isNull())
  {
    QPixmap cursorPixmap = QPixmap::fromImage(baseImage);
    return QCursor(cursorPixmap, baseImage.width()/2, 0);
  }

  QImage effectImage(effectIcon.pixmap(effectIcon.availableSizes()[0]).toImage());
  int width = qMax(baseImage.width(), effectImage.width());
  int pad = -9;
  int height = pad + baseImage.height() + effectImage.height();
  width = height = qMax(width,height);
  int center = width/2;
  QImage cursorImage(width, height, QImage::Format_ARGB32);
  QPainter painter;
  cursorImage.fill(0);
  painter.begin(&cursorImage);
  QPoint point(center - (baseImage.width()/2), 0);
  painter.drawImage(point, baseImage);
  point.setX(center - (effectImage.width()/2));
  point.setY(cursorImage.height() - effectImage.height());
  painter.drawImage(point, effectImage);
  painter.end();

  QPixmap cursorPixmap = QPixmap::fromImage(cursorImage);
  return QCursor(cursorPixmap, center, 0);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::cursorOff(qMRMLWidget* viewWidget)
{
  Q_D(qSlicerSegmentEditorAbstractEffect);

  d->SavedCursor = QCursor(viewWidget->cursor());
  viewWidget->setCursor(QCursor(Qt::BlankCursor));
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::cursorOn(qMRMLWidget* viewWidget)
{
  Q_D(qSlicerSegmentEditorAbstractEffect);

  viewWidget->setCursor(d->SavedCursor);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::addActor3D(qMRMLWidget* viewWidget, vtkProp3D* actor)
{
  Q_D(qSlicerSegmentEditorAbstractEffect);

  vtkRenderer* renderer = qSlicerSegmentEditorAbstractEffect::renderer(viewWidget);
  if (renderer)
  {
    renderer->AddViewProp(actor);
    d->scheduleRender(viewWidget);
  }
  else
  {
    qCritical() << Q_FUNC_INFO << ": Failed to get renderer for view widget";
  }
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::addActor2D(qMRMLWidget* viewWidget, vtkActor2D* actor)
{
  Q_D(qSlicerSegmentEditorAbstractEffect);

  vtkRenderer* renderer = qSlicerSegmentEditorAbstractEffect::renderer(viewWidget);
  if (renderer)
  {
    renderer->AddActor2D(actor);
    d->scheduleRender(viewWidget);
  }
  else
  {
    qCritical() << Q_FUNC_INFO << ": Failed to get renderer for view widget";
  }
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::removeActor3D(qMRMLWidget* viewWidget, vtkProp3D* actor)
{
  Q_D(qSlicerSegmentEditorAbstractEffect);

  vtkRenderer* renderer = qSlicerSegmentEditorAbstractEffect::renderer(viewWidget);
  if (renderer)
  {
    renderer->RemoveActor(actor);
    d->scheduleRender(viewWidget);
  }
  else
  {
    qCritical() << Q_FUNC_INFO << ": Failed to get renderer for view widget";
  }
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::removeActor2D(qMRMLWidget* viewWidget, vtkActor2D* actor)
{
  Q_D(qSlicerSegmentEditorAbstractEffect);

  vtkRenderer* renderer = qSlicerSegmentEditorAbstractEffect::renderer(viewWidget);
  if (renderer)
  {
    renderer->RemoveActor2D(actor);
    d->scheduleRender(viewWidget);
  }
  else
  {
    qCritical() << Q_FUNC_INFO << ": Failed to get renderer for view widget";
  }

}

//-----------------------------------------------------------------------------
QFrame* qSlicerSegmentEditorAbstractEffect::optionsFrame()
{
  Q_D(qSlicerSegmentEditorAbstractEffect);

  return d->OptionsFrame;
}

//-----------------------------------------------------------------------------
QFormLayout* qSlicerSegmentEditorAbstractEffect::optionsLayout()
{
  Q_D(qSlicerSegmentEditorAbstractEffect);
  QFormLayout* formLayout = qobject_cast<QFormLayout*>(d->OptionsFrame->layout());
  return formLayout;
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::addOptionsWidget(QWidget* newOptionsWidget)
{
  Q_D(qSlicerSegmentEditorAbstractEffect);

  newOptionsWidget->setParent(d->OptionsFrame);
  newOptionsWidget->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding));
  this->optionsLayout()->addRow(newOptionsWidget);
}

//-----------------------------------------------------------------------------
QWidget* qSlicerSegmentEditorAbstractEffect::addLabeledOptionsWidget(QString label, QWidget* newOptionsWidget)  
{
  Q_D(qSlicerSegmentEditorAbstractEffect);
  QLabel* labelWidget = new QLabel(label);
  newOptionsWidget->setParent(d->OptionsFrame);
  newOptionsWidget->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding));
  this->optionsLayout()->addRow(labelWidget, newOptionsWidget);
  return labelWidget;
}

//-----------------------------------------------------------------------------
QWidget* qSlicerSegmentEditorAbstractEffect::addLabeledOptionsWidget(QString label, QLayout* newOptionsWidget)
{
  Q_D(qSlicerSegmentEditorAbstractEffect);
  QLabel* labelWidget = new QLabel(label);
  this->optionsLayout()->addRow(labelWidget, newOptionsWidget);
  return labelWidget;
}

//-----------------------------------------------------------------------------
vtkMRMLScene* qSlicerSegmentEditorAbstractEffect::scene()
{
  Q_D(qSlicerSegmentEditorAbstractEffect);

  if (!d->ParameterSetNode)
  {
    return NULL;
  }

  return d->ParameterSetNode->GetScene();
}

//-----------------------------------------------------------------------------
vtkMRMLSegmentEditorNode* qSlicerSegmentEditorAbstractEffect::parameterSetNode()
{
  Q_D(qSlicerSegmentEditorAbstractEffect);

  return d->ParameterSetNode.GetPointer();
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::setParameterSetNode(vtkMRMLSegmentEditorNode* node)
{
  Q_D(qSlicerSegmentEditorAbstractEffect);

  d->ParameterSetNode = node;
}

//-----------------------------------------------------------------------------
QString qSlicerSegmentEditorAbstractEffect::parameter(QString name)
{
  Q_D(qSlicerSegmentEditorAbstractEffect);
  if (!d->ParameterSetNode)
  {
    return QString();
  }

  // Get effect-specific prefixed parameter first
  QString attributeName = QString("%1.%2").arg(this->name()).arg(name);
  const char* value = d->ParameterSetNode->GetAttribute(attributeName.toLatin1().constData());
  // Look for common parameter if effect-specific one is not found
  if (!value)
  {
    value = d->ParameterSetNode->GetAttribute(name.toLatin1().constData());
  }
  if (!value)
  {
    qCritical() << Q_FUNC_INFO << ": Parameter named " << name << " cannot be found for effect " << this->name();
    return QString();
  }

  return QString(value);
}

//-----------------------------------------------------------------------------
int qSlicerSegmentEditorAbstractEffect::integerParameter(QString name)
{
  Q_D(qSlicerSegmentEditorAbstractEffect);
  if (!d->ParameterSetNode)
  {
    return 0;
  }

  QString parameterStr = this->parameter(name);
  bool ok = false;
  int parameterInt = parameterStr.toInt(&ok);
  if (!ok)
  {
    qCritical() << Q_FUNC_INFO << ": Parameter named " << name << " cannot be converted to integer!";
    return 0;
  }

  return parameterInt;
}

//-----------------------------------------------------------------------------
double qSlicerSegmentEditorAbstractEffect::doubleParameter(QString name)
{
  Q_D(qSlicerSegmentEditorAbstractEffect);
  if (!d->ParameterSetNode)
  {
    return 0.0;
  }

  QString parameterStr = this->parameter(name);
  bool ok = false;
  double parameterDouble = parameterStr.toDouble(&ok);
  if (!ok)
  {
    qCritical() << Q_FUNC_INFO << ": Parameter named " << name << " cannot be converted to floating point number!";
    return 0.0;
  }

  return parameterDouble;
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::setParameter(QString name, QString value)
{
  Q_D(qSlicerSegmentEditorAbstractEffect);
  if (!d->ParameterSetNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid segment editor parameter set node set to effect " << this->name();
    return;
  }

  // Set parameter as attribute
  QString attributeName = QString("%1.%2").arg(this->name()).arg(name);
  this->setCommonParameter(attributeName, value);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::setCommonParameter(QString name, QString value)
{
  Q_D(qSlicerSegmentEditorAbstractEffect);
  if (!d->ParameterSetNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid segment editor parameter set node set to effect " << this->name();
    return;
  }

  const char* oldValue = d->ParameterSetNode->GetAttribute(name.toLatin1().constData());
  if (oldValue == NULL && value.isEmpty())
  {
    // no change
    return;
  }
  if (value == QString(oldValue))
  {
    // no change
    return;
  }

  // Disable full modified events in all cases (observe EffectParameterModified instead if necessary)
  int disableState = d->ParameterSetNode->GetDisableModifiedEvent();
  d->ParameterSetNode->SetDisableModifiedEvent(1);

  // Set parameter as attribute
  d->ParameterSetNode->SetAttribute(name.toLatin1().constData(), value.toLatin1().constData());

  // Re-enable full modified events for parameter node
  d->ParameterSetNode->SetDisableModifiedEvent(disableState);

  // Emit parameter modified event if requested
  // Don't pass parameter name as char pointer, as custom modified events may be compressed and invoked after EndModify()
  // and by that time the pointer may not be valid anymore.
  d->ParameterSetNode->InvokeCustomModifiedEvent(vtkMRMLSegmentEditorNode::EffectParameterModified);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::setParameter(QString name, int value)
{
  this->setParameter(name, QString::number(value));
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::setCommonParameter(QString name, int value)
{
  this->setCommonParameter(name, QString::number(value));
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::setParameter(QString name, double value)
{
  this->setParameter(name, QString::number(value));
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::setCommonParameter(QString name, double value)
{
  this->setCommonParameter(name, QString::number(value));
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::setVolumes(vtkOrientedImageData* alignedMasterVolume,
  vtkOrientedImageData* editedLabelmap, vtkOrientedImageData* maskLabelmap,
  vtkOrientedImageData* selectedSegmentLabelmap)
{
  Q_D(qSlicerSegmentEditorAbstractEffect);
  d->EditedLabelmap = editedLabelmap;
  d->MaskLabelmap = maskLabelmap;
  d->AlignedMasterVolume = alignedMasterVolume;
  d->SelectedSegmentLabelmap = selectedSegmentLabelmap;
}

//-----------------------------------------------------------------------------
vtkOrientedImageData* qSlicerSegmentEditorAbstractEffect::editedLabelmap()
{
  Q_D(qSlicerSegmentEditorAbstractEffect);
  // don't request edited labelmap update using updateVolumeSignal here, as it would clear the volume
  return d->EditedLabelmap;
}

//-----------------------------------------------------------------------------
vtkOrientedImageData* qSlicerSegmentEditorAbstractEffect::maskLabelmap()
{
  Q_D(qSlicerSegmentEditorAbstractEffect);
  emit d->updateVolumeSignal(d->MaskLabelmap.GetPointer());
  return d->MaskLabelmap;
}

//-----------------------------------------------------------------------------
vtkOrientedImageData* qSlicerSegmentEditorAbstractEffect::masterVolumeImageData()
{
  Q_D(qSlicerSegmentEditorAbstractEffect);
  emit d->updateVolumeSignal(d->AlignedMasterVolume.GetPointer());
  return d->AlignedMasterVolume;
}

//-----------------------------------------------------------------------------
vtkOrientedImageData* qSlicerSegmentEditorAbstractEffect::selectedSegmentLabelmap()
{
  Q_D(qSlicerSegmentEditorAbstractEffect);
  emit d->updateVolumeSignal(d->SelectedSegmentLabelmap.GetPointer());
  return d->SelectedSegmentLabelmap;
}

//-----------------------------------------------------------------------------
vtkRenderWindow* qSlicerSegmentEditorAbstractEffect::renderWindow(qMRMLWidget* viewWidget)
{
  if (!viewWidget)
  {
    return NULL;
  }

  qMRMLSliceWidget* sliceWidget = qobject_cast<qMRMLSliceWidget*>(viewWidget);
  qMRMLThreeDWidget* threeDWidget = qobject_cast<qMRMLThreeDWidget*>(viewWidget);
  if (sliceWidget)
  {
    return sliceWidget->sliceView()->renderWindow();
  }
  else if (threeDWidget)
  {
    return threeDWidget->threeDView()->renderWindow();
  }

  qCritical() << Q_FUNC_INFO << ": Unsupported view widget type!";
  return NULL;
}

//-----------------------------------------------------------------------------
vtkRenderer* qSlicerSegmentEditorAbstractEffect::renderer(qMRMLWidget* viewWidget)
{
  vtkRenderWindow* renderWindow = qSlicerSegmentEditorAbstractEffect::renderWindow(viewWidget);
  if (!renderWindow)
  {
    return NULL;
  }

  return vtkRenderer::SafeDownCast(renderWindow->GetRenderers()->GetItemAsObject(0));
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractViewNode* qSlicerSegmentEditorAbstractEffect::viewNode(qMRMLWidget* viewWidget)
{
  if (!viewWidget)
  {
    return NULL;
  }

  qMRMLSliceWidget* sliceWidget = qobject_cast<qMRMLSliceWidget*>(viewWidget);
  qMRMLThreeDWidget* threeDWidget = qobject_cast<qMRMLThreeDWidget*>(viewWidget);
  if (sliceWidget)
  {
    return sliceWidget->sliceLogic()->GetSliceNode();
  }
  else if (threeDWidget)
  {
    return threeDWidget->mrmlViewNode();
  }

  qCritical() << Q_FUNC_INFO << ": Unsupported view widget type!";
  return NULL;
}

//-----------------------------------------------------------------------------
QPoint qSlicerSegmentEditorAbstractEffect::rasToXy(double ras[3], qMRMLSliceWidget* sliceWidget)
{
  QPoint xy(0,0);

  vtkMRMLSliceNode* sliceNode = vtkMRMLSliceNode::SafeDownCast(
    qSlicerSegmentEditorAbstractEffect::viewNode(sliceWidget) );
  if (!sliceNode)
  {
    qCritical() << Q_FUNC_INFO << ": Failed to get slice node!";
    return xy;
  }

  double rast[4] = {ras[0], ras[1], ras[2], 1.0};
  double xyzw[4] = {0.0, 0.0, 0.0, 1.0};
  vtkSmartPointer<vtkMatrix4x4> rasToXyMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  rasToXyMatrix->DeepCopy(sliceNode->GetXYToRAS());
  rasToXyMatrix->Invert();
  rasToXyMatrix->MultiplyPoint(rast, xyzw);

  xy.setX(xyzw[0]);
  xy.setY(xyzw[1]);
  return xy;
}

//-----------------------------------------------------------------------------
QPoint qSlicerSegmentEditorAbstractEffect::rasToXy(QVector3D rasVector, qMRMLSliceWidget* sliceWidget)
{
  double ras[3] = {rasVector.x(), rasVector.y(), rasVector.z()};
  return qSlicerSegmentEditorAbstractEffect::rasToXy(ras, sliceWidget);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::xyzToRas(double inputXyz[3], double outputRas[3], qMRMLSliceWidget* sliceWidget)
{
  outputRas[0] = outputRas[1] = outputRas[2] = 0.0;

  vtkMRMLSliceNode* sliceNode = vtkMRMLSliceNode::SafeDownCast(
    qSlicerSegmentEditorAbstractEffect::viewNode(sliceWidget) );
  if (!sliceNode)
  {
    qCritical() << Q_FUNC_INFO << ": Failed to get slice node!";
    return;
  }
  
  // x,y uses slice (canvas) coordinate system and actually has a 3rd z component (index into the
  // slice you're looking at), hence xyToRAS is really performing xyzToRAS. RAS is patient world
  // coordinate system. Note the 1 is because the transform uses homogeneous coordinates.
  double xyzw[4] = {inputXyz[0], inputXyz[1], inputXyz[2], 1.0};
  double rast[4] = {0.0, 0.0, 0.0, 1.0};
  sliceNode->GetXYToRAS()->MultiplyPoint(xyzw, rast);
  outputRas[0] = rast[0];
  outputRas[1] = rast[1];
  outputRas[2] = rast[2];
}

//-----------------------------------------------------------------------------
QVector3D qSlicerSegmentEditorAbstractEffect::xyzToRas(QVector3D inputXyzVector, qMRMLSliceWidget* sliceWidget)
{
  double inputXyz[3] = {inputXyzVector.x(), inputXyzVector.y(), inputXyzVector.z()};
  double outputRas[3] = {0.0, 0.0, 0.0};
  qSlicerSegmentEditorAbstractEffect::xyzToRas(inputXyz, outputRas, sliceWidget);
  QVector3D outputVector(outputRas[0], outputRas[1], outputRas[2]);
  return outputVector;
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::xyToRas(QPoint xy, double outputRas[3], qMRMLSliceWidget* sliceWidget)
{
  double xyz[3] = {xy.x(), xy.y(), 0.0};
  qSlicerSegmentEditorAbstractEffect::xyzToRas(xyz, outputRas, sliceWidget);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::xyToRas(double xy[2], double outputRas[3], qMRMLSliceWidget* sliceWidget)
{
  double xyz[3] = {xy[0], xy[1], 0.0};
  qSlicerSegmentEditorAbstractEffect::xyzToRas(xyz, outputRas, sliceWidget);
}

//-----------------------------------------------------------------------------
QVector3D qSlicerSegmentEditorAbstractEffect::xyToRas(QPoint xy, qMRMLSliceWidget* sliceWidget)
{
  double outputRas[3] = {0.0, 0.0, 0.0};
  qSlicerSegmentEditorAbstractEffect::xyToRas(xy, outputRas, sliceWidget);
  QVector3D outputVector(outputRas[0], outputRas[1], outputRas[2]);
  return outputVector;
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::xyzToIjk(double inputXyz[3], int outputIjk[3], qMRMLSliceWidget* sliceWidget, vtkOrientedImageData* image)
{
  outputIjk[0] = outputIjk[1] = outputIjk[2] = 0;

  if (!sliceWidget || !image)
  {
    return;
  }

  // Convert from XY to RAS first
  double ras[3] = {0.0, 0.0, 0.0};
  qSlicerSegmentEditorAbstractEffect::xyzToRas(inputXyz, ras, sliceWidget);

  // Convert RAS to image IJK
  double rast[4] = {ras[0], ras[1], ras[2], 1.0};
  double ijkl[4] = {0.0, 0.0, 0.0, 1.0};
  vtkSmartPointer<vtkMatrix4x4> rasToIjkMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  image->GetImageToWorldMatrix(rasToIjkMatrix);
  rasToIjkMatrix->Invert();
  rasToIjkMatrix->MultiplyPoint(rast, ijkl);

  outputIjk[0] = (int)(ijkl[0] + 0.5);
  outputIjk[1] = (int)(ijkl[1] + 0.5);
  outputIjk[2] = (int)(ijkl[2] + 0.5);
}

//-----------------------------------------------------------------------------
QVector3D qSlicerSegmentEditorAbstractEffect::xyzToIjk(QVector3D inputXyzVector, qMRMLSliceWidget* sliceWidget, vtkOrientedImageData* image)
{
  double inputXyz[3] = {inputXyzVector.x(), inputXyzVector.y(), inputXyzVector.z()};
  int outputIjk[3] = {0, 0, 0};
  qSlicerSegmentEditorAbstractEffect::xyzToIjk(inputXyz, outputIjk, sliceWidget, image);
  QVector3D outputVector(outputIjk[0], outputIjk[1], outputIjk[2]);
  return outputVector;
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::xyToIjk(QPoint xy, int outputIjk[3], qMRMLSliceWidget* sliceWidget, vtkOrientedImageData* image)
{
  double xyz[3] = {xy.x(), xy.y(), 0.0};
  qSlicerSegmentEditorAbstractEffect::xyzToIjk(xyz, outputIjk, sliceWidget, image);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::xyToIjk(double xy[2], int outputIjk[3], qMRMLSliceWidget* sliceWidget, vtkOrientedImageData* image)
{
  double xyz[3] = {xy[0], xy[0], 0.0};
  qSlicerSegmentEditorAbstractEffect::xyzToIjk(xyz, outputIjk, sliceWidget, image);
}

//-----------------------------------------------------------------------------
QVector3D qSlicerSegmentEditorAbstractEffect::xyToIjk(QPoint xy, qMRMLSliceWidget* sliceWidget, vtkOrientedImageData* image)
{
  int outputIjk[3] = {0, 0, 0};
  qSlicerSegmentEditorAbstractEffect::xyToIjk(xy, outputIjk, sliceWidget, image);
  QVector3D outputVector(outputIjk[0], outputIjk[1], outputIjk[2]);
  return outputVector;
}

//-----------------------------------------------------------------------------
int qSlicerSegmentEditorAbstractEffect::editedLabelmapApplyMode()
{
  Q_D(qSlicerSegmentEditorAbstractEffect);
  return d->EditedLabelmapApplyMode;
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::setEditedLabelmapApplyMode(int applyMode)
{
  Q_D(qSlicerSegmentEditorAbstractEffect);
  d->EditedLabelmapApplyMode = applyMode;
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::setEditedLabelmapApplyModeToSet()
{
  this->setEditedLabelmapApplyMode(qSlicerSegmentEditorAbstractEffect::APPLY_MODE_SET);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::setEditedLabelmapApplyModeToAdd()
{
  this->setEditedLabelmapApplyMode(qSlicerSegmentEditorAbstractEffect::APPLY_MODE_ADD);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::setEditedLabelmapApplyModeToRemove()
{
  this->setEditedLabelmapApplyMode(qSlicerSegmentEditorAbstractEffect::APPLY_MODE_REMOVE);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::editedLabelmapApplyExtent(int extent[6])const
{
  Q_D(const qSlicerSegmentEditorAbstractEffect);
  for (int i=0; i<6; i++)
  {
    extent[i] = d->EditedLabelmapApplyExtent[i];
  }
}

//-----------------------------------------------------------------------------
int* qSlicerSegmentEditorAbstractEffect::editedLabelmapApplyExtent()
{
  Q_D(qSlicerSegmentEditorAbstractEffect);
  return d->EditedLabelmapApplyExtent;
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::setEditedLabelmapApplyExtent(int extent[6])
{
  Q_D(qSlicerSegmentEditorAbstractEffect);
  for (int i=0; i<6; i++)
  {
    d->EditedLabelmapApplyExtent[i] = extent[i];
  }
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::setEditedLabelmapApplyExtent(int xStart, int xEnd, int yStart, int yEnd, int zStart, int zEnd)
{
  Q_D(qSlicerSegmentEditorAbstractEffect);
  d->EditedLabelmapApplyExtent[0] = xStart;
  d->EditedLabelmapApplyExtent[1] = xEnd;
  d->EditedLabelmapApplyExtent[2] = yStart;
  d->EditedLabelmapApplyExtent[3] = yEnd;
  d->EditedLabelmapApplyExtent[4] = zStart;
  d->EditedLabelmapApplyExtent[5] = zEnd;
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::setEditedLabelmapApplyExtentToWholeExtent()
{
  this->setEditedLabelmapApplyExtent(0,-1,0,-1,0,-1);
}
