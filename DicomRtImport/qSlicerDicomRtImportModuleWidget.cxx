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

// SlicerQt includes
#include "qSlicerDicomRtImportModuleWidget.h"
#include "ui_qSlicerDicomRtImportModule.h"
#include "vtkSlicerDicomRtImportModuleLogic.h"

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_ExtensionTemplate
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
	vtkSlicerDicomRtImportModuleLogic *logic = vtkSlicerDicomRtImportModuleLogic::SafeDownCast(this->logic());
	logic->LoadDicomRT("\\wangk\\temp\\RTSTRUCT.2.16.840.1.113669.2.931128.198378313.20120104144541.513083.dcm","SeriesName");
}
