/*==============================================================================

  Copyright (c) Radiation Medicine Program, University Health Network,
  Princess Margaret Hospital, Toronto, ON, Canada. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Kevin Wang, Princess Margaret Cancer Centre 
  and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/

// Qt includes
#include <QDebug>

// SlicerQt includes
#include "qSlicerDicomSroImportExportModuleWidget.h"
#include "ui_qSlicerDicomSroImportExportModule.h"
#include "vtkSlicerDicomSroImportExportModuleLogic.h"

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_DicomSroImportExport
class qSlicerDicomSroImportExportModuleWidgetPrivate: public Ui_qSlicerDicomSroImportExportModule
{
public:
  qSlicerDicomSroImportExportModuleWidgetPrivate();
};

//-----------------------------------------------------------------------------
// qSlicerDicomSroImportExportModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerDicomSroImportExportModuleWidgetPrivate::qSlicerDicomSroImportExportModuleWidgetPrivate() = default;

//-----------------------------------------------------------------------------
// qSlicerDicomSroImportExportModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerDicomSroImportExportModuleWidget::qSlicerDicomSroImportExportModuleWidget(QWidget* _parent)
  : Superclass( _parent )
  , d_ptr( new qSlicerDicomSroImportExportModuleWidgetPrivate )
{
}

//-----------------------------------------------------------------------------
qSlicerDicomSroImportExportModuleWidget::~qSlicerDicomSroImportExportModuleWidget() = default;

//-----------------------------------------------------------------------------
void qSlicerDicomSroImportExportModuleWidget::setup()
{
  Q_D(qSlicerDicomSroImportExportModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();

  QObject::connect( this->d_ptr->pushButton_LoadDicomSro, SIGNAL(pressed()), this, SLOT(LoadDicomSro()) );
}

//-----------------------------------------------------------------------------
void qSlicerDicomSroImportExportModuleWidget::LoadDicomSro()
{
}
