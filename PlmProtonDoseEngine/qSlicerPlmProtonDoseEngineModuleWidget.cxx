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
  and was supported through CANARIE.

==============================================================================*/

// Qt includes
#include <QDebug>

// SlicerQt includes
#include "qSlicerPlmProtonDoseEngineModuleWidget.h"
#include "ui_qSlicerPlmProtonDoseEngineModuleWidget.h"

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_ExtensionTemplate
class qSlicerPlmProtonDoseEngineModuleWidgetPrivate: public Ui_qSlicerPlmProtonDoseEngineModuleWidget
{
public:
  qSlicerPlmProtonDoseEngineModuleWidgetPrivate();
};

//-----------------------------------------------------------------------------
// qSlicerPlmProtonDoseEngineModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerPlmProtonDoseEngineModuleWidgetPrivate::qSlicerPlmProtonDoseEngineModuleWidgetPrivate()
{
}

//-----------------------------------------------------------------------------
// qSlicerPlmProtonDoseEngineModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerPlmProtonDoseEngineModuleWidget::qSlicerPlmProtonDoseEngineModuleWidget(QWidget* _parent)
  : Superclass( _parent )
  , d_ptr( new qSlicerPlmProtonDoseEngineModuleWidgetPrivate )
{
}

//-----------------------------------------------------------------------------
qSlicerPlmProtonDoseEngineModuleWidget::~qSlicerPlmProtonDoseEngineModuleWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerPlmProtonDoseEngineModuleWidget::setup()
{
  Q_D(qSlicerPlmProtonDoseEngineModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();
}
