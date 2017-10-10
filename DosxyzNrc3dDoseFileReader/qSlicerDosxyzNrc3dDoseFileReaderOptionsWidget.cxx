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

/// Qt includes
#include <QFileInfo>

// CTK includes
#include <ctkFlowLayout.h>
#include <ctkUtils.h>

/// DosxyzNrc3dDoseFileReader includes
#include "qSlicerIOOptions_p.h"
#include "qSlicerDosxyzNrc3dDoseFileReaderOptionsWidget.h"
#include "ui_qSlicerDosxyzNrc3dDoseFileReaderOptionsWidget.h"

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_DosxyzNrc3dDoseFileReader
class qSlicerDosxyzNrc3dDoseFileReaderOptionsWidgetPrivate
  : public qSlicerIOOptionsPrivate
  , public Ui_qSlicerDosxyzNrc3dDoseFileReaderOptionsWidget
{
public:
};

//-----------------------------------------------------------------------------
qSlicerDosxyzNrc3dDoseFileReaderOptionsWidget::qSlicerDosxyzNrc3dDoseFileReaderOptionsWidget(QWidget* parentWidget)
  : qSlicerIOOptionsWidget(new qSlicerDosxyzNrc3dDoseFileReaderOptionsWidgetPrivate, parentWidget)
{
  Q_D(qSlicerDosxyzNrc3dDoseFileReaderOptionsWidget);
  d->setupUi(this);

  ctkFlowLayout::replaceLayout(this);

  connect(d->UseImageIntensityScaleAndOffsetCheckBox, SIGNAL(toggled(bool)),
          this, SLOT(updateProperties()));

  // Image intensity scale and offset turned off by default
  d->UseImageIntensityScaleAndOffsetCheckBox->setChecked(false);
}

//-----------------------------------------------------------------------------
qSlicerDosxyzNrc3dDoseFileReaderOptionsWidget::~qSlicerDosxyzNrc3dDoseFileReaderOptionsWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerDosxyzNrc3dDoseFileReaderOptionsWidget::updateProperties()
{
  Q_D(qSlicerDosxyzNrc3dDoseFileReaderOptionsWidget);

  d->Properties["imageIntensityScaleAndOffset"] = d->UseImageIntensityScaleAndOffsetCheckBox->isChecked();
}
