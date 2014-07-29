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

  This file was originally developed by Jennifer Andrea, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Natural Sciences and Engineering Research Council
  of Canada.

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
/// \ingroup SlicerRt_QtModules_VffFileReader
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
