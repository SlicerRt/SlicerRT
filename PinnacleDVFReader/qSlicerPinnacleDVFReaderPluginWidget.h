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

  This file was originally developed by Kevin Wang, Princess Margaret Cancer Centre 
  and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/

#ifndef __qSlicerPinnacleDVFReaderPluginWidget_h
#define __qSlicerPinnacleDVFReaderPluginWidget_h

// SlicerQt includes
#include "qSlicerAbstractModuleWidget.h"

// PinnacleDVFReader includes
#include "qSlicerPinnacleDVFReaderModuleExport.h"

class qSlicerPinnacleDVFReaderPluginWidgetPrivate;

/// \ingroup SlicerRt_QtModules_PinnacleDVFReader
class Q_SLICER_PINNACLEDVFREADER_EXPORT qSlicerPinnacleDVFReaderPluginWidget :
  public qSlicerAbstractModuleWidget
{
  Q_OBJECT
public:
  typedef qSlicerAbstractModuleWidget Superclass;
  qSlicerPinnacleDVFReaderPluginWidget(QWidget *parent=0);
  virtual ~qSlicerPinnacleDVFReaderPluginWidget();

protected:
  QScopedPointer<qSlicerPinnacleDVFReaderPluginWidgetPrivate> d_ptr;
  virtual void setup();

private:
  Q_DECLARE_PRIVATE(qSlicerPinnacleDVFReaderPluginWidget);
  Q_DISABLE_COPY(qSlicerPinnacleDVFReaderPluginWidget);
};

#endif
