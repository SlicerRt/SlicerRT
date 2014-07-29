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

  This file was originally developed by Kevin Wang, Radiation Medicine Program, 
  University Health Network and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/

// Qt includes
#include <QDebug>

// SlicerQt includes
#include "qSlicerDicomRtImportModuleWidget.h"
#include "ui_qSlicerDicomRtImportModule.h"
#include "vtkSlicerDicomRtImportModuleLogic.h"

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_DicomRtImport
class qSlicerDicomRtImportModuleWidgetPrivate: public Ui_qSlicerDicomRtImportModule
{
public:
  qSlicerDicomRtImportModuleWidgetPrivate();
};

//-----------------------------------------------------------------------------
// qSlicerDicomRtImportModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerDicomRtImportModuleWidgetPrivate::qSlicerDicomRtImportModuleWidgetPrivate()
{
}

//-----------------------------------------------------------------------------
// qSlicerDicomRtImportModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerDicomRtImportModuleWidget::qSlicerDicomRtImportModuleWidget(QWidget* _parent)
  : Superclass( _parent )
  , d_ptr( new qSlicerDicomRtImportModuleWidgetPrivate )
{
}

//-----------------------------------------------------------------------------
qSlicerDicomRtImportModuleWidget::~qSlicerDicomRtImportModuleWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerDicomRtImportModuleWidget::setup()
{
  Q_D(qSlicerDicomRtImportModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();

  QObject::connect( this->d_ptr->pushButton_LoadDicomRT, SIGNAL(pressed()), this, SLOT(LoadDicomRt()) );
}

//-----------------------------------------------------------------------------
void qSlicerDicomRtImportModuleWidget::LoadDicomRt()
{
}
