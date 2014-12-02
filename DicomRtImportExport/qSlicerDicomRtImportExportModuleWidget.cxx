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

// SlicerQt includes
#include "qSlicerDicomRtImportExportModuleWidget.h"
#include "ui_qSlicerDicomRtImportExportModule.h"

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_DicomRtImport
class qSlicerDicomRtImportExportModuleWidgetPrivate: public Ui_qSlicerDicomRtImportExportModule
{
public:
  qSlicerDicomRtImportExportModuleWidgetPrivate();
};

//-----------------------------------------------------------------------------
// qSlicerDicomRtImportExportModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerDicomRtImportExportModuleWidgetPrivate::qSlicerDicomRtImportExportModuleWidgetPrivate()
{
}

//-----------------------------------------------------------------------------
// qSlicerDicomRtImportExportModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerDicomRtImportExportModuleWidget::qSlicerDicomRtImportExportModuleWidget(QWidget* _parent)
  : Superclass( _parent )
  , d_ptr( new qSlicerDicomRtImportExportModuleWidgetPrivate )
{
}

//-----------------------------------------------------------------------------
qSlicerDicomRtImportExportModuleWidget::~qSlicerDicomRtImportExportModuleWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerDicomRtImportExportModuleWidget::setup()
{
  Q_D(qSlicerDicomRtImportExportModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();
}
