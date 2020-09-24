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
#include "qSlicerPlmDrrParametersWidget.h"
#include "ui_qSlicerPlmDrrParametersWidget.h"

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_LoadableModuleTemplate
class qSlicerPlmDrrParametersWidgetPrivate : public Ui_qSlicerPlmDrrParametersWidget
{
  Q_DECLARE_PUBLIC(qSlicerPlmDrrParametersWidget);
protected:
  qSlicerPlmDrrParametersWidget* const q_ptr;

public:
  qSlicerPlmDrrParametersWidgetPrivate(
    qSlicerPlmDrrParametersWidget& object);
  virtual void setupUi(qSlicerPlmDrrParametersWidget*);
};

// --------------------------------------------------------------------------
qSlicerPlmDrrParametersWidgetPrivate::qSlicerPlmDrrParametersWidgetPrivate(qSlicerPlmDrrParametersWidget& object)
  : q_ptr(&object)
{
}

// --------------------------------------------------------------------------
void qSlicerPlmDrrParametersWidgetPrivate::setupUi(qSlicerPlmDrrParametersWidget* widget)
{
  this->Ui_qSlicerPlmDrrParametersWidget::setupUi(widget);
}

//-----------------------------------------------------------------------------
// qSlicerLoadableModuleTemplateFooBarWidget methods

//-----------------------------------------------------------------------------
qSlicerPlmDrrParametersWidget::qSlicerPlmDrrParametersWidget(QWidget* parentWidget)
  : Superclass( parentWidget )
  , d_ptr( new qSlicerPlmDrrParametersWidgetPrivate(*this) )
{
  Q_D(qSlicerPlmDrrParametersWidget);
  d->setupUi(this);
}

//-----------------------------------------------------------------------------
qSlicerPlmDrrParametersWidget::~qSlicerPlmDrrParametersWidget()
{
}
