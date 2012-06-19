/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

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
#include "qSlicerDoseComparisonModuleWidget.h"
#include "ui_qSlicerDoseComparisonModule.h"

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_DoseComparison
class qSlicerDoseComparisonModuleWidgetPrivate: public Ui_qSlicerDoseComparisonModule
{
public:
  qSlicerDoseComparisonModuleWidgetPrivate();
};

//-----------------------------------------------------------------------------
// qSlicerDoseComparisonModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerDoseComparisonModuleWidgetPrivate::qSlicerDoseComparisonModuleWidgetPrivate()
{
}

//-----------------------------------------------------------------------------
// qSlicerDoseComparisonModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerDoseComparisonModuleWidget::qSlicerDoseComparisonModuleWidget(QWidget* _parent)
  : Superclass( _parent )
  , d_ptr( new qSlicerDoseComparisonModuleWidgetPrivate )
{
}

//-----------------------------------------------------------------------------
qSlicerDoseComparisonModuleWidget::~qSlicerDoseComparisonModuleWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerDoseComparisonModuleWidget::setup()
{
  Q_D(qSlicerDoseComparisonModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();
}

