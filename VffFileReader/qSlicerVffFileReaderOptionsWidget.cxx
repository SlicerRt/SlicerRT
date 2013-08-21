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

/// VffFileReader includes
#include "qSlicerIOOptions_p.h"
#include "qSlicerVffFileReaderOptionsWidget.h"
#include "ui_qSlicerVffFileReaderOptionsWidget.h"

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_VffFileReader
class qSlicerVffFileReaderOptionsWidgetPrivate
  : public qSlicerIOOptionsPrivate
  , public Ui_qSlicerVffFileReaderOptionsWidget
{
public:
};

//-----------------------------------------------------------------------------
qSlicerVffFileReaderOptionsWidget::qSlicerVffFileReaderOptionsWidget(QWidget* parentWidget)
  : qSlicerIOOptionsWidget(new qSlicerVffFileReaderOptionsWidgetPrivate, parentWidget)
{
  Q_D(qSlicerVffFileReaderOptionsWidget);
  d->setupUi(this);

  ctkFlowLayout::replaceLayout(this);
  /*
  // Replace the horizontal layout with a flow layout
  ctkFlowLayout* flowLayout = new ctkFlowLayout;
  flowLayout->setPreferredExpandingDirections(Qt::Horizontal);
  flowLayout->setAlignItems(false);
  QLayout* oldLayout = this->layout();
  int margins[4];
  oldLayout->getContentsMargins(&margins[0],&margins[1],&margins[2],&margins[3]);
  QLayoutItem* item = 0;
  while((item = oldLayout->takeAt(0)))
    {
    if (item->widget())
      {
      flowLayout->addWidget(item->widget());
      }
    }
  // setLayout() will take care or reparenting layouts and widgets
  delete oldLayout;
  flowLayout->setContentsMargins(0,0,0,0);
  this->setLayout(flowLayout);
  */

  connect(d->UseImageIntensityScaleAndOffsetCheckBox, SIGNAL(toggled(bool)),
          this, SLOT(updateProperties()));

  // Image intensity scale and offset turned off by default
  d->UseImageIntensityScaleAndOffsetCheckBox->setChecked(false);
}

//-----------------------------------------------------------------------------
qSlicerVffFileReaderOptionsWidget::~qSlicerVffFileReaderOptionsWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerVffFileReaderOptionsWidget::updateProperties()
{
  Q_D(qSlicerVffFileReaderOptionsWidget);

  d->Properties["imageIntensityScaleAndOffset"] = d->UseImageIntensityScaleAndOffsetCheckBox->isChecked();
}
