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

  This file was originally developed by Julien Finet, Kitware Inc.
  and was partially funded by NIH grant 3P41RR013218-12S1

==============================================================================*/

/// Qt includes
#include <QFileInfo>

// CTK includes
#include <ctkFlowLayout.h>
#include <ctkUtils.h>

/// PinnacleDVFReader includes
#include "qSlicerIOOptions_p.h"
#include "qSlicerPinnacleDVFReaderOptionsWidget.h"
#include "ui_qSlicerPinnacleDVFReaderOptionsWidget.h"

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_PinnacleDVFReader
class qSlicerPinnacleDVFReaderOptionsWidgetPrivate
  : public qSlicerIOOptionsPrivate
  , public Ui_qSlicerPinnacleDVFReaderOptionsWidget
{
public:
};

//-----------------------------------------------------------------------------
qSlicerPinnacleDVFReaderOptionsWidget::qSlicerPinnacleDVFReaderOptionsWidget(QWidget* parentWidget)
  : qSlicerIOOptionsWidget(new qSlicerPinnacleDVFReaderOptionsWidgetPrivate, parentWidget)
{
  Q_D(qSlicerPinnacleDVFReaderOptionsWidget);
  d->setupUi(this);

  connect(d->gridOriginX, SIGNAL(textChanged(const QString &)), this, SLOT(updateProperties()));
  connect(d->gridOriginY, SIGNAL(textChanged(const QString &)), this, SLOT(updateProperties()));
  connect(d->gridOriginZ, SIGNAL(textChanged(const QString &)), this, SLOT(updateProperties()));
}

//-----------------------------------------------------------------------------
qSlicerPinnacleDVFReaderOptionsWidget::~qSlicerPinnacleDVFReaderOptionsWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerPinnacleDVFReaderOptionsWidget::updateProperties()
{
  Q_D(qSlicerPinnacleDVFReaderOptionsWidget);

  d->Properties["gridOriginX"] = d->gridOriginX->text();
  d->Properties["gridOriginY"] = d->gridOriginY->text();
  d->Properties["gridOriginZ"] = d->gridOriginZ->text();
}
