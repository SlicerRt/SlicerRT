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
#include "qSlicerPlastimatchFooBarWidget.h"
#include "ui_qSlicerPlastimatchFooBarWidget.h"

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_Plastimatch
class qSlicerPlastimatchFooBarWidgetPrivate
  : public Ui_qSlicerPlastimatchFooBarWidget
{
  Q_DECLARE_PUBLIC(qSlicerPlastimatchFooBarWidget);
protected:
  qSlicerPlastimatchFooBarWidget* const q_ptr;

public:
  qSlicerPlastimatchFooBarWidgetPrivate(
    qSlicerPlastimatchFooBarWidget& object);
  virtual void setupUi(qSlicerPlastimatchFooBarWidget*);
};

// --------------------------------------------------------------------------
qSlicerPlastimatchFooBarWidgetPrivate
::qSlicerPlastimatchFooBarWidgetPrivate(
  qSlicerPlastimatchFooBarWidget& object)
  : q_ptr(&object)
{
}

// --------------------------------------------------------------------------
void qSlicerPlastimatchFooBarWidgetPrivate
::setupUi(qSlicerPlastimatchFooBarWidget* widget)
{
  this->Ui_qSlicerPlastimatchFooBarWidget::setupUi(widget);
}

//-----------------------------------------------------------------------------
// qSlicerPlastimatchFooBarWidget methods

//-----------------------------------------------------------------------------
qSlicerPlastimatchFooBarWidget
::qSlicerPlastimatchFooBarWidget(QWidget* parentWidget)
  : Superclass( parentWidget )
  , d_ptr( new qSlicerPlastimatchFooBarWidgetPrivate(*this) )
{
  Q_D(qSlicerPlastimatchFooBarWidget);
  d->setupUi(this);
}

//-----------------------------------------------------------------------------
qSlicerPlastimatchFooBarWidget
::~qSlicerPlastimatchFooBarWidget()
{
}
