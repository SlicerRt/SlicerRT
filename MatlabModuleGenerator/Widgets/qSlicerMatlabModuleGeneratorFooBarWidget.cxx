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

  This file was originally developed by Jean-Christophe Fillion-Robin, Kitware Inc.
  and was partially funded by NIH grant 3P41RR013218-12S1

==============================================================================*/

// FooBar Widgets includes
#include "qSlicerMatlabModuleGeneratorFooBarWidget.h"
#include "ui_qSlicerMatlabModuleGeneratorFooBarWidget.h"

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_MatlabModuleGenerator
class qSlicerMatlabModuleGeneratorFooBarWidgetPrivate
  : public Ui_qSlicerMatlabModuleGeneratorFooBarWidget
{
  Q_DECLARE_PUBLIC(qSlicerMatlabModuleGeneratorFooBarWidget);
protected:
  qSlicerMatlabModuleGeneratorFooBarWidget* const q_ptr;

public:
  qSlicerMatlabModuleGeneratorFooBarWidgetPrivate(
    qSlicerMatlabModuleGeneratorFooBarWidget& object);
  virtual void setupUi(qSlicerMatlabModuleGeneratorFooBarWidget*);
};

// --------------------------------------------------------------------------
qSlicerMatlabModuleGeneratorFooBarWidgetPrivate
::qSlicerMatlabModuleGeneratorFooBarWidgetPrivate(
  qSlicerMatlabModuleGeneratorFooBarWidget& object)
  : q_ptr(&object)
{
}

// --------------------------------------------------------------------------
void qSlicerMatlabModuleGeneratorFooBarWidgetPrivate
::setupUi(qSlicerMatlabModuleGeneratorFooBarWidget* widget)
{
  this->Ui_qSlicerMatlabModuleGeneratorFooBarWidget::setupUi(widget);
}

//-----------------------------------------------------------------------------
// qSlicerMatlabModuleGeneratorFooBarWidget methods

//-----------------------------------------------------------------------------
qSlicerMatlabModuleGeneratorFooBarWidget
::qSlicerMatlabModuleGeneratorFooBarWidget(QWidget* parentWidget)
  : Superclass( parentWidget )
  , d_ptr( new qSlicerMatlabModuleGeneratorFooBarWidgetPrivate(*this) )
{
  Q_D(qSlicerMatlabModuleGeneratorFooBarWidget);
  d->setupUi(this);
}

//-----------------------------------------------------------------------------
qSlicerMatlabModuleGeneratorFooBarWidget
::~qSlicerMatlabModuleGeneratorFooBarWidget()
{
}
