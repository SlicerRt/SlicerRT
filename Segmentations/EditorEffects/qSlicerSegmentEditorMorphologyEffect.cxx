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
#include "qSlicerSegmentEditorMorphologyEffect.h"

#include "vtkMRMLSegmentationNode.h"
#include "vtkMRMLSegmentEditorNode.h"

// Qt includes
#include <QDebug>
#include <QRadioButton>

//-----------------------------------------------------------------------------
// qSlicerSegmentEditorMorphologyEffectPrivate methods

//-----------------------------------------------------------------------------
class qSlicerSegmentEditorMorphologyEffectPrivate: public QObject
{
  Q_DECLARE_PUBLIC(qSlicerSegmentEditorMorphologyEffect);
protected:
  qSlicerSegmentEditorMorphologyEffect* const q_ptr;
public:
  typedef QObject Superclass;
  qSlicerSegmentEditorMorphologyEffectPrivate(qSlicerSegmentEditorMorphologyEffect& object);
  ~qSlicerSegmentEditorMorphologyEffectPrivate();

public:
  QRadioButton* EightNeighborsRadioButton;
  QRadioButton* FourNeighborsRadioButton;
};

//-----------------------------------------------------------------------------
qSlicerSegmentEditorMorphologyEffectPrivate::qSlicerSegmentEditorMorphologyEffectPrivate(qSlicerSegmentEditorMorphologyEffect& object)
  : q_ptr(&object)
  , EightNeighborsRadioButton(NULL)
  , FourNeighborsRadioButton(NULL)
{
}

//-----------------------------------------------------------------------------
qSlicerSegmentEditorMorphologyEffectPrivate::~qSlicerSegmentEditorMorphologyEffectPrivate()
{
}

//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// qSlicerSegmentEditorMorphologyEffect methods

//----------------------------------------------------------------------------
qSlicerSegmentEditorMorphologyEffect::qSlicerSegmentEditorMorphologyEffect(QObject* parent)
 : Superclass(parent)
 , d_ptr( new qSlicerSegmentEditorMorphologyEffectPrivate(*this) )
{
}

//----------------------------------------------------------------------------
qSlicerSegmentEditorMorphologyEffect::~qSlicerSegmentEditorMorphologyEffect()
{
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorMorphologyEffect::setupOptionsFrame()
{
  Q_D(qSlicerSegmentEditorMorphologyEffect);

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
void qSlicerSegmentEditorMorphologyEffect::setMRMLDefaults()
{
  this->setCommonParameter(this->neighborModeParameterName(), 4);
  this->setCommonParameter(this->iterationsParameterName(), 1);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorMorphologyEffect::updateGUIFromMRML()
{
  Q_D(qSlicerSegmentEditorMorphologyEffect);

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
void qSlicerSegmentEditorMorphologyEffect::updateMRMLFromGUI()
{
  Q_D(qSlicerSegmentEditorMorphologyEffect);

  if (d->EightNeighborsRadioButton->isChecked())
  {
    this->setCommonParameter(this->neighborModeParameterName(), 8);
  }
  else
  {
    this->setCommonParameter(this->neighborModeParameterName(), 4);
  }
}
