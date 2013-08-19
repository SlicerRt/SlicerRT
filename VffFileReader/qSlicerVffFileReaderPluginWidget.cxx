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

  This file was originally developed by Jennifer Andrea, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Natural Sciences and Engineering Research Council
  of Canada.

==============================================================================*/

/// VffFileReader includes
#include "qSlicerVffFileReaderPluginWidget.h"
#include "ui_qSlicerVffFileReaderPluginWidget.h"
#include "vtkSlicerVffFileReaderLogic.h"

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_VffFileReader
class qSlicerVffFileReaderPluginWidgetPrivate: public Ui_qSlicerVffFileReaderPluginWidget
{
public:
  qSlicerVffFileReaderPluginWidgetPrivate();
};

//-----------------------------------------------------------------------------
qSlicerVffFileReaderPluginWidgetPrivate::qSlicerVffFileReaderPluginWidgetPrivate()
{
}

//-----------------------------------------------------------------------------
qSlicerVffFileReaderPluginWidget::qSlicerVffFileReaderPluginWidget(QWidget* parentWidget)
  : Superclass( parentWidget )
  , d_ptr( new qSlicerVffFileReaderPluginWidgetPrivate )
{
}

//-----------------------------------------------------------------------------
qSlicerVffFileReaderPluginWidget::~qSlicerVffFileReaderPluginWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerVffFileReaderPluginWidget::setup()
{
  Q_D(qSlicerVffFileReaderPluginWidget);
  d->setupUi(this);
  this->Superclass::setup();
}
