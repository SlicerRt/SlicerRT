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
#include "qSlicerSegmentEditorAbstractMorphologyEffect.h"

#include "vtkMRMLSegmentationNode.h"
#include "vtkMRMLSegmentEditorNode.h"

// Qt includes
#include <QDebug>
#include <QRadioButton>

//-----------------------------------------------------------------------------
// qSlicerSegmentEditorAbstractMorphologyEffectPrivate methods

//-----------------------------------------------------------------------------
class qSlicerSegmentEditorAbstractMorphologyEffectPrivate: public QObject
{
  Q_DECLARE_PUBLIC(qSlicerSegmentEditorAbstractMorphologyEffect);
protected:
  qSlicerSegmentEditorAbstractMorphologyEffect* const q_ptr;
public:
  typedef QObject Superclass;
  qSlicerSegmentEditorAbstractMorphologyEffectPrivate(qSlicerSegmentEditorAbstractMorphologyEffect& object);
  ~qSlicerSegmentEditorAbstractMorphologyEffectPrivate();

public:
  QRadioButton* EightNeighborsRadioButton;
  QRadioButton* FourNeighborsRadioButton;
};

//-----------------------------------------------------------------------------
qSlicerSegmentEditorAbstractMorphologyEffectPrivate::qSlicerSegmentEditorAbstractMorphologyEffectPrivate(qSlicerSegmentEditorAbstractMorphologyEffect& object)
  : q_ptr(&object)
  , EightNeighborsRadioButton(NULL)
  , FourNeighborsRadioButton(NULL)
{
}

//-----------------------------------------------------------------------------
qSlicerSegmentEditorAbstractMorphologyEffectPrivate::~qSlicerSegmentEditorAbstractMorphologyEffectPrivate()
{
}

//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// qSlicerSegmentEditorAbstractMorphologyEffect methods

//----------------------------------------------------------------------------
qSlicerSegmentEditorAbstractMorphologyEffect::qSlicerSegmentEditorAbstractMorphologyEffect(QObject* parent)
 : Superclass(parent)
 , d_ptr( new qSlicerSegmentEditorAbstractMorphologyEffectPrivate(*this) )
{
}

//----------------------------------------------------------------------------
qSlicerSegmentEditorAbstractMorphologyEffect::~qSlicerSegmentEditorAbstractMorphologyEffect()
{
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractMorphologyEffect::setupOptionsFrame()
{
  Q_D(qSlicerSegmentEditorAbstractMorphologyEffect);

  d->EightNeighborsRadioButton = new QRadioButton("Eight neighbors");
  d->EightNeighborsRadioButton->setToolTip("Treat diagonally adjacent voxels as neighbors.");
  this->addOptionsWidget(d->EightNeighborsRadioButton);

  d->FourNeighborsRadioButton = new QRadioButton("Four neighbors");
  d->FourNeighborsRadioButton->setToolTip("Do not treat diagonally adjacent voxels as neighbors.");
  this->addOptionsWidget(d->FourNeighborsRadioButton);

  //TODO: Iterations option

  QObject::connect(d->EightNeighborsRadioButton, SIGNAL(clicked()), this, SLOT(updateMRMLFromGUI()));
  QObject::connect(d->FourNeighborsRadioButton, SIGNAL(clicked()), this, SLOT(updateMRMLFromGUI()));
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractMorphologyEffect::setMRMLDefaults()
{
  this->setCommonParameter(this->neighborModeParameterName(), 4);
  this->setCommonParameter(this->iterationsParameterName(), 1);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractMorphologyEffect::updateGUIFromMRML()
{
  Q_D(qSlicerSegmentEditorAbstractMorphologyEffect);
  if (!this->active())
  {
    // updateGUIFromMRML is called when the effect is activated
    return;
  }
  
  d->EightNeighborsRadioButton->blockSignals(true);
  d->FourNeighborsRadioButton->blockSignals(true);

  int neighborMode = this->integerParameter(this->neighborModeParameterName());
  if (neighborMode == 8)
  {
    d->EightNeighborsRadioButton->setChecked(true);
    d->FourNeighborsRadioButton->setChecked(false);
  }
  else if (neighborMode == 4)
  {
    d->EightNeighborsRadioButton->setChecked(false);
    d->FourNeighborsRadioButton->setChecked(true);
  }

  d->EightNeighborsRadioButton->blockSignals(false);
  d->FourNeighborsRadioButton->blockSignals(false);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractMorphologyEffect::updateMRMLFromGUI()
{
  Q_D(qSlicerSegmentEditorAbstractMorphologyEffect);

  if (d->EightNeighborsRadioButton->isChecked())
  {
    this->setCommonParameter(this->neighborModeParameterName(), 8);
  }
  else
  {
    this->setCommonParameter(this->neighborModeParameterName(), 4);
  }
}
