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

// Qt includes

// SlicerQt includes
#include "qSlicerGammaDoseComparisonModuleWidget.h"
#include "ui_qSlicerGammaDoseComparisonModule.h"

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_GammaDoseComparison
class qSlicerGammaDoseComparisonModuleWidgetPrivate: public Ui_qSlicerGammaDoseComparisonModule
{
public:
  qSlicerGammaDoseComparisonModuleWidgetPrivate();
};

//-----------------------------------------------------------------------------
// qSlicerGammaDoseComparisonModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerGammaDoseComparisonModuleWidgetPrivate::qSlicerGammaDoseComparisonModuleWidgetPrivate()
{
}

//-----------------------------------------------------------------------------
// qSlicerGammaDoseComparisonModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerGammaDoseComparisonModuleWidget::qSlicerGammaDoseComparisonModuleWidget(QWidget* _parent)
  : Superclass( _parent )
  , d_ptr( new qSlicerGammaDoseComparisonModuleWidgetPrivate )
{
}

//-----------------------------------------------------------------------------
qSlicerGammaDoseComparisonModuleWidget::~qSlicerGammaDoseComparisonModuleWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerGammaDoseComparisonModuleWidget::setup()
{
  Q_D(qSlicerGammaDoseComparisonModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();
}

