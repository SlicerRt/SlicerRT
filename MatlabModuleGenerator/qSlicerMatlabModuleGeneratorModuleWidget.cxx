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

==============================================================================*/

// Qt includes
#include <QDebug>
#include <QSettings> 

// SlicerQt includes
#include "qSlicerMatlabModuleGeneratorModuleWidget.h"
#include "ui_qSlicerMatlabModuleGeneratorModuleWidget.h"

// Module includes
#include "vtkSlicerMatlabModuleGeneratorLogic.h"


//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_ExtensionTemplate
class qSlicerMatlabModuleGeneratorModuleWidgetPrivate: public Ui_qSlicerMatlabModuleGeneratorModuleWidget
{
  Q_DECLARE_PUBLIC(qSlicerMatlabModuleGeneratorModuleWidget);
protected:
  qSlicerMatlabModuleGeneratorModuleWidget* const q_ptr;
public:
  qSlicerMatlabModuleGeneratorModuleWidgetPrivate(qSlicerMatlabModuleGeneratorModuleWidget& object);
  vtkSlicerMatlabModuleGeneratorLogic* logic() const;
};

//-----------------------------------------------------------------------------
// qSlicerMatlabModuleGeneratorModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerMatlabModuleGeneratorModuleWidgetPrivate::qSlicerMatlabModuleGeneratorModuleWidgetPrivate(qSlicerMatlabModuleGeneratorModuleWidget& object)
: q_ptr(&object)
{
}

//-----------------------------------------------------------------------------
vtkSlicerMatlabModuleGeneratorLogic*
qSlicerMatlabModuleGeneratorModuleWidgetPrivate::logic() const
{
  Q_Q(const qSlicerMatlabModuleGeneratorModuleWidget);
  return vtkSlicerMatlabModuleGeneratorLogic::SafeDownCast(q->logic());
} 

//-----------------------------------------------------------------------------
// qSlicerMatlabModuleGeneratorModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerMatlabModuleGeneratorModuleWidget::qSlicerMatlabModuleGeneratorModuleWidget(QWidget* _parent)
  : Superclass( _parent )
  , d_ptr( new qSlicerMatlabModuleGeneratorModuleWidgetPrivate(*this) )
{
}

//-----------------------------------------------------------------------------
qSlicerMatlabModuleGeneratorModuleWidget::~qSlicerMatlabModuleGeneratorModuleWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerMatlabModuleGeneratorModuleWidget::setup()
{
  Q_D(qSlicerMatlabModuleGeneratorModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();

  connect( d->lineEdit_MatlabExecutablePath, SIGNAL(currentPathChanged(QString)), this, SLOT(matlabExecutablePathChanged(QString)) ); 

  connect( d->pushButton_GenerateModule, SIGNAL(clicked()), this, SLOT(generateModuleClicked()) );

  d->lineEdit_MatlabExecutablePath->setCurrentPath(d->logic()->GetMatlabExecutablePath());
  d->lineEdit_MatlabScriptDirectory->setText(d->logic()->GetMatlabScriptDirectory());
}

//-----------------------------------------------------------------------------
void qSlicerMatlabModuleGeneratorModuleWidget::matlabExecutablePathChanged(QString path)
{
  Q_D(qSlicerMatlabModuleGeneratorModuleWidget);
  
  QSettings settings;
  settings.setValue("Matlab/MatlabExecutablePath",path); 

  d->logic()->SetMatlabExecutablePath(path.toLatin1());
}

//-----------------------------------------------------------------------------
void qSlicerMatlabModuleGeneratorModuleWidget::generateModuleClicked()
{
  Q_D(qSlicerMatlabModuleGeneratorModuleWidget);
  QString moduleName=d->lineEdit_GeneratorModuleName->text();
  vtkStdString result=d->logic()->GenerateModule(moduleName.toLatin1());
  d->textEdit_GeneratorResults->setText(QString(result));
}
