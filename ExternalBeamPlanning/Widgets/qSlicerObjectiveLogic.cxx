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

   This file was originally developed by Niklas Wahl, German Cancer Research Center (DKFZ)

==============================================================================*/

// Dose engines includes
#include "qSlicerObjectiveLogic.h"

#include "qSlicerObjectivePluginHandler.h"
#include "qSlicerAbstractObjective.h"


// MRML includes
#include <vtkMRMLScene.h>


// Slicer includes
#include "qSlicerCoreApplication.h"
#include "vtkSlicerApplicationLogic.h"

// VTK includes
#include <vtkSmartPointer.h>

// Qt includes
#include <QDebug>

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_SubjectHierarchy
class qSlicerObjectiveLogicPrivate
{
  Q_DECLARE_PUBLIC(qSlicerObjectiveLogic);
protected:
  qSlicerObjectiveLogic* const q_ptr;
public:
  qSlicerObjectiveLogicPrivate(qSlicerObjectiveLogic& object);
  ~qSlicerObjectiveLogicPrivate();
};

//-----------------------------------------------------------------------------
// qSlicerObjectiveLogicPrivate methods

//-----------------------------------------------------------------------------
qSlicerObjectiveLogicPrivate::qSlicerObjectiveLogicPrivate(qSlicerObjectiveLogic& object)
  : q_ptr(&object)
{
}

//-----------------------------------------------------------------------------
qSlicerObjectiveLogicPrivate::~qSlicerObjectiveLogicPrivate() = default;

//-----------------------------------------------------------------------------
// qSlicerObjectiveLogic methods

//----------------------------------------------------------------------------
qSlicerObjectiveLogic::qSlicerObjectiveLogic(QObject* parent)
  : QObject(parent)
{
}

//----------------------------------------------------------------------------
qSlicerObjectiveLogic::~qSlicerObjectiveLogic() = default;

