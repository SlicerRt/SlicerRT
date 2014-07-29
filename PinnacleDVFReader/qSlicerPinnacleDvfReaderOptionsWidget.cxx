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

/// Qt includes
#include <QFileInfo>

// CTK includes
#include <ctkFlowLayout.h>
#include <ctkUtils.h>

/// PinnacleDvfReader includes
#include "qSlicerIOOptions_p.h"
#include "qSlicerPinnacleDvfReaderOptionsWidget.h"
#include "ui_qSlicerPinnacleDvfReaderOptionsWidget.h"

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_PinnacleDvfReader
class qSlicerPinnacleDvfReaderOptionsWidgetPrivate
  : public qSlicerIOOptionsPrivate
  , public Ui_qSlicerPinnacleDvfReaderOptionsWidget
{
public:
};

//-----------------------------------------------------------------------------
qSlicerPinnacleDvfReaderOptionsWidget::qSlicerPinnacleDvfReaderOptionsWidget(QWidget* parentWidget)
  : qSlicerIOOptionsWidget(new qSlicerPinnacleDvfReaderOptionsWidgetPrivate, parentWidget)
{
  Q_D(qSlicerPinnacleDvfReaderOptionsWidget);
  d->setupUi(this);

  connect(d->gridOriginX, SIGNAL(textChanged(const QString &)), this, SLOT(updateProperties()));
  connect(d->gridOriginY, SIGNAL(textChanged(const QString &)), this, SLOT(updateProperties()));
  connect(d->gridOriginZ, SIGNAL(textChanged(const QString &)), this, SLOT(updateProperties()));
}

//-----------------------------------------------------------------------------
qSlicerPinnacleDvfReaderOptionsWidget::~qSlicerPinnacleDvfReaderOptionsWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerPinnacleDvfReaderOptionsWidget::updateProperties()
{
  Q_D(qSlicerPinnacleDvfReaderOptionsWidget);

  d->Properties["gridOriginX"] = d->gridOriginX->text();
  d->Properties["gridOriginY"] = d->gridOriginY->text();
  d->Properties["gridOriginZ"] = d->gridOriginZ->text();
}
