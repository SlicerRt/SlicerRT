/*==============================================================================

  Copyright (c) Laboratory for Percutaneous Surgery (PerkLab)
  Queen's University, Kingston, ON, Canada. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Anna Ilina, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario.

==============================================================================*/

/// DosxyzNrc3dDoseFileReader includes
#include "qSlicerDosxyzNrc3dDoseFileReaderPluginWidget.h"
#include "ui_qSlicerDosxyzNrc3dDoseFileReaderPluginWidget.h"
#include "vtkSlicerDosxyzNrc3dDoseFileReaderLogic.h"

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_DosxyzNrc3dDoseFileReader
class qSlicerDosxyzNrc3dDoseFileReaderPluginWidgetPrivate: public Ui_qSlicerDosxyzNrc3dDoseFileReaderPluginWidget
{
public:
  qSlicerDosxyzNrc3dDoseFileReaderPluginWidgetPrivate();
};

//-----------------------------------------------------------------------------
qSlicerDosxyzNrc3dDoseFileReaderPluginWidgetPrivate::qSlicerDosxyzNrc3dDoseFileReaderPluginWidgetPrivate()
{
}

//-----------------------------------------------------------------------------
qSlicerDosxyzNrc3dDoseFileReaderPluginWidget::qSlicerDosxyzNrc3dDoseFileReaderPluginWidget(QWidget* parentWidget)
  : Superclass( parentWidget )
  , d_ptr( new qSlicerDosxyzNrc3dDoseFileReaderPluginWidgetPrivate )
{
}

//-----------------------------------------------------------------------------
qSlicerDosxyzNrc3dDoseFileReaderPluginWidget::~qSlicerDosxyzNrc3dDoseFileReaderPluginWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerDosxyzNrc3dDoseFileReaderPluginWidget::setup()
{
  Q_D(qSlicerDosxyzNrc3dDoseFileReaderPluginWidget);
  d->setupUi(this);
  this->Superclass::setup();
}
