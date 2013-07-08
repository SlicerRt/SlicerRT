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

  This file was originally developed by Jean-Christophe Fillion-Robin, Kitware Inc.
  and was partially funded by NIH grant 3P41RR013218-12S1

==============================================================================*/

#ifndef __qSlicerPlastimatchFooBarWidget_h
#define __qSlicerPlastimatchFooBarWidget_h

// Qt includes
#include <QWidget>

// FooBar Widgets includes
#include "qSlicerPlastimatchModuleWidgetsExport.h"

class qSlicerPlastimatchFooBarWidgetPrivate;

/// \ingroup Slicer_QtModules_Plastimatch
class Q_SLICER_MODULE_PLASTIMATCH_WIDGETS_EXPORT qSlicerPlastimatchFooBarWidget
  : public QWidget
{
  Q_OBJECT
public:
  typedef QWidget Superclass;
  qSlicerPlastimatchFooBarWidget(QWidget *parent=0);
  virtual ~qSlicerPlastimatchFooBarWidget();

protected slots:

protected:
  QScopedPointer<qSlicerPlastimatchFooBarWidgetPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerPlastimatchFooBarWidget);
  Q_DISABLE_COPY(qSlicerPlastimatchFooBarWidget);
};

#endif
