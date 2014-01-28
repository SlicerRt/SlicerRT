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

  This file was originally developed by Kevin Wang, Radiation Medicine Program, 
  University Health Network and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/

// Qt includes
#include <QDebug>

// SlicerQt includes
#include "qSlicerDicomSroImportModuleWidget.h"
#include "ui_qSlicerDicomSroImportModule.h"
#include "vtkSlicerDicomSroImportModuleLogic.h"

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_DicomSroImport
class qSlicerDicomSroImportModuleWidgetPrivate: public Ui_qSlicerDicomSroImportModule
{
public:
  qSlicerDicomSroImportModuleWidgetPrivate();
};

//-----------------------------------------------------------------------------
// qSlicerDicomSroImportModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerDicomSroImportModuleWidgetPrivate::qSlicerDicomSroImportModuleWidgetPrivate()
{
}

//-----------------------------------------------------------------------------
// qSlicerDicomSroImportModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerDicomSroImportModuleWidget::qSlicerDicomSroImportModuleWidget(QWidget* _parent)
  : Superclass( _parent )
  , d_ptr( new qSlicerDicomSroImportModuleWidgetPrivate )
{
}

//-----------------------------------------------------------------------------
qSlicerDicomSroImportModuleWidget::~qSlicerDicomSroImportModuleWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerDicomSroImportModuleWidget::setup()
{
  Q_D(qSlicerDicomSroImportModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();

  QObject::connect( this->d_ptr->pushButton_LoadDicomSro, SIGNAL(pressed()), this, SLOT(LoadDicomSro()) );
}

//-----------------------------------------------------------------------------
void qSlicerDicomSroImportModuleWidget::LoadDicomSro()
{
}
