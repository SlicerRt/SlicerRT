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
#include "qSlicerDicomRtExportModuleWidget.h"
#include "ui_qSlicerDicomRtExportModule.h"
#include "vtkSlicerDicomRtExportModuleLogic.h"

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_DicomRtExport
class qSlicerDicomRtExportModuleWidgetPrivate: public Ui_qSlicerDicomRtExportModule
{
public:
  qSlicerDicomRtExportModuleWidgetPrivate();
};

//-----------------------------------------------------------------------------
// qSlicerDicomRtExportModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerDicomRtExportModuleWidgetPrivate::qSlicerDicomRtExportModuleWidgetPrivate()
{
}

//-----------------------------------------------------------------------------
// qSlicerDicomRtExportModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerDicomRtExportModuleWidget::qSlicerDicomRtExportModuleWidget(QWidget* _parent)
  : Superclass( _parent )
  , d_ptr( new qSlicerDicomRtExportModuleWidgetPrivate )
{
}

//-----------------------------------------------------------------------------
qSlicerDicomRtExportModuleWidget::~qSlicerDicomRtExportModuleWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerDicomRtExportModuleWidget::setup()
{
  Q_D(qSlicerDicomRtExportModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();

  QObject::connect( this->d_ptr->pushButton_SaveDicomRT, SIGNAL(pressed()), this, SLOT(SaveDicomRt()) );
}

//-----------------------------------------------------------------------------
void qSlicerDicomRtExportModuleWidget::SaveDicomRt()
{
}
