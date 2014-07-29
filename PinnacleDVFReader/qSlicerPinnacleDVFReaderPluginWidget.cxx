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

  This file was originally developed by Kevin Wang, Princess Margaret Cancer Centre 
  and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/

/// PinnacleDVFReader includes
#include "qSlicerPinnacleDVFReaderPluginWidget.h"
#include "ui_qSlicerPinnacleDVFReaderPluginWidget.h"
#include "vtkSlicerPinnacleDVFReaderLogic.h"

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_PinnacleDVFReader
class qSlicerPinnacleDVFReaderPluginWidgetPrivate: public Ui_qSlicerPinnacleDVFReaderPluginWidget
{
public:
  qSlicerPinnacleDVFReaderPluginWidgetPrivate();
};

//-----------------------------------------------------------------------------
qSlicerPinnacleDVFReaderPluginWidgetPrivate::qSlicerPinnacleDVFReaderPluginWidgetPrivate()
{
}

//-----------------------------------------------------------------------------
qSlicerPinnacleDVFReaderPluginWidget::qSlicerPinnacleDVFReaderPluginWidget(QWidget* parentWidget)
  : Superclass( parentWidget )
  , d_ptr( new qSlicerPinnacleDVFReaderPluginWidgetPrivate )
{
}

//-----------------------------------------------------------------------------
qSlicerPinnacleDVFReaderPluginWidget::~qSlicerPinnacleDVFReaderPluginWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerPinnacleDVFReaderPluginWidget::setup()
{
  Q_D(qSlicerPinnacleDVFReaderPluginWidget);
  d->setupUi(this);
  this->Superclass::setup();
}
