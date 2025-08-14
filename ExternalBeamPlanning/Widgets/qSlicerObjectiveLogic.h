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

This file was originally developed by Lina Bucher, Institut fuer Biomedizinische
Technik at the Karlsruher Institut fuer Technologie (IBT-KIT) and German Cancer
Research Center (DKFZ)

==============================================================================*/

#ifndef __qSlicerObjectiveLogic_h
#define __qSlicerObjectiveLogic_h

// ExternalBeamPlanning includes
#include "qSlicerExternalBeamPlanningModuleWidgetsExport.h"

// SlicerQt includes
#include "qSlicerObject.h"

// CTK includes
#include <ctkVTKObject.h>

// Qt includes
#include <QObject>

class qSlicerObjectiveLogicPrivate;

/// \ingroup SlicerRt_QtModules_ExternalBeamPlanning
/// \brief Abstract objectives
class Q_SLICER_MODULE_EXTERNALBEAMPLANNING_WIDGETS_EXPORT qSlicerObjectiveLogic :
  public QObject, public virtual qSlicerObject
{
  Q_OBJECT
    QVTK_OBJECT

public:
  typedef QObject Superclass;
  /// Constructor
  explicit qSlicerObjectiveLogic(QObject* parent = nullptr);
  /// Destructor
  ~qSlicerObjectiveLogic() override;

protected:
  QScopedPointer<qSlicerObjectiveLogic> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerObjectiveLogic);
  Q_DISABLE_COPY(qSlicerObjectiveLogic);
};

#endif
