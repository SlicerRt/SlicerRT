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

  This file was originally developed by Csaba Pinter, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

#include "vtkTopologicalHierarchy.h"

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkNew.h>
#include <vtkPolyDataCollection.h>
#include <vtkIntArray.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkTopologicalHierarchy);

//----------------------------------------------------------------------------
vtkTopologicalHierarchy::vtkTopologicalHierarchy()
{
  this->InputPolyDatas = NULL;
  vtkSmartPointer<vtkPolyDataCollection> inputPolyDatas = vtkSmartPointer<vtkPolyDataCollection>::New();
  this->SetInputPolyDatas(inputPolyDatas);

  this->OutputLevels = NULL;
  vtkSmartPointer<vtkIntArray> outputLevels = vtkSmartPointer<vtkIntArray>::New();
  this->SetOutputLevels(outputLevels);

  this->ContainConstraintFactor = 0.0;

  this->MaximumLevel = 7;
}

//----------------------------------------------------------------------------
vtkTopologicalHierarchy::~vtkTopologicalHierarchy()
{
  this->SetInputPolyDatas(NULL);
  this->SetOutputLevels(NULL);
}

//----------------------------------------------------------------------------
void vtkTopologicalHierarchy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkIntArray* vtkTopologicalHierarchy::GetOutput()
{
  return this->OutputLevels;
}

//----------------------------------------------------------------------------
bool vtkTopologicalHierarchy::Contains(vtkPolyData* polyOut, vtkPolyData* polyIn)
{
  if (!polyOut || !polyOut)
  {
    vtkErrorMacro("Empty input parameters!");
    return false;
  }

  double extentOut[6];
  polyOut->GetBounds(extentOut);
  
  double extentIn[6];
  polyIn->GetBounds(extentIn);

  if ( extentOut[0] < extentIn[0] - this->ContainConstraintFactor * (extentOut[1]-extentOut[0])
    && extentOut[1] > extentIn[1] + this->ContainConstraintFactor * (extentOut[1]-extentOut[0])
    && extentOut[2] < extentIn[2] - this->ContainConstraintFactor * (extentOut[3]-extentOut[2])
    && extentOut[3] > extentIn[3] + this->ContainConstraintFactor * (extentOut[3]-extentOut[2])
    && extentOut[4] < extentIn[4] - this->ContainConstraintFactor * (extentOut[5]-extentOut[4])
    && extentOut[5] > extentIn[5] + this->ContainConstraintFactor * (extentOut[5]-extentOut[4]) )
  {
    return true;
  }

  return false;
}

//----------------------------------------------------------------------------
void vtkTopologicalHierarchy::Update()
{
  if (!this->InputPolyDatas || !this->OutputLevels)
  {
    vtkErrorMacro("Input poly data collection and output int array have to be initialized!");
    return;
  }

  this->OutputLevels->Initialize();
  unsigned int numberOfPolyDatas = this->InputPolyDatas->GetNumberOfItems();

  // Check input polydata collection
  for (unsigned int i=0; i<numberOfPolyDatas; ++i)
  {
    vtkPolyData* polyOut = vtkPolyData::SafeDownCast(this->InputPolyDatas->GetItemAsObject(i));
    if (!polyOut)
    {
      vtkErrorMacro("Input collection contains invalid object at item " << i);
      return;
    }
  }

  std::vector<std::vector<int> > containedPolyDatas(numberOfPolyDatas);
  this->OutputLevels->SetNumberOfComponents(1);
  this->OutputLevels->SetNumberOfTuples(numberOfPolyDatas);
  this->OutputLevels->FillComponent(0, -1);

  // Step 1: Set level of polydata containing no other polydata to 0
  this->InputPolyDatas->InitTraversal();
  for (unsigned int i=0; i<numberOfPolyDatas; ++i)
  {
    vtkPolyData* polyOut = vtkPolyData::SafeDownCast(this->InputPolyDatas->GetItemAsObject(i));

    for (int j=0; j<numberOfPolyDatas; ++j)
    {
      if (i==j)
      {
        continue;
      }

      vtkPolyData* polyIn = vtkPolyData::SafeDownCast(this->InputPolyDatas->GetItemAsObject(j));

      if (this->Contains(polyOut, polyIn))
      {
        containedPolyDatas[i].push_back(j);
      }
    }

    if (containedPolyDatas[i].size() == 0)
    {
      this->OutputLevels->SetValue(i, 0);
    }
  }

  // Step 2: Set level of the polydata containing other polydata to one bigger than the highest contained level
  vtkSmartPointer<vtkIntArray> outputLevelsSnapshot = vtkSmartPointer<vtkIntArray>::New();
  unsigned int currentLevel = 1;
  while (this->OutputContainsEmptyLevels() && currentLevel < this->MaximumLevel)
  {
    // Creating snapshot of the level array state so that the newly set values don't interfere with the check
    // Without this, the check "does all contained polydata have level values assigned" is corrupted
    outputLevelsSnapshot->DeepCopy(this->OutputLevels);

    // Step 3: For all polydata without level value assigned
    for (unsigned int i=0; i<numberOfPolyDatas; ++i)
    {
      if (this->OutputLevels->GetValue(i) > -1)
      {
        continue;
      }

      // Step 4: If all contained polydata have level values assigned, then set it to the current level value
      //   The level that is to be set cannot be lower than the current level value, because then we would
      //   already have assigned it in the previous iterations.
      bool allContainedPolydataHasLevelValueAssigned = true;
      for (unsigned int j=0; j<numberOfPolyDatas; ++j)
      {
        if (i==j)
        {
          continue;
        }
        bool isContained = false;
        for (std::vector<int>::iterator it=containedPolyDatas[i].begin(); it!=containedPolyDatas[i].end(); ++it)
        {
          if ((*it) == j)
          {
            isContained = true;
          }
        }
        if (!isContained)
        {
          continue;
        }

        if (outputLevelsSnapshot->GetValue(j) == -1)
        {
          allContainedPolydataHasLevelValueAssigned = false;
          break;
        }
      }
      if (allContainedPolydataHasLevelValueAssigned)
      {
        this->OutputLevels->SetValue(i, currentLevel);
      }
    }

    // Increase current level for the next iteration
    currentLevel++;
  }

  // Step 5: Set maximum level to all polydata that has no level value assigned
  for (unsigned int i=0; i<numberOfPolyDatas; ++i)
  {
    if (this->OutputLevels->GetValue(i) == -1)
    {
      this->OutputLevels->SetValue(i, this->MaximumLevel);
    }
  }
}

//----------------------------------------------------------------------------
bool vtkTopologicalHierarchy::OutputContainsEmptyLevels()
{
  if (!this->OutputLevels)
  {
    return false;
  }

  for (int i=0; i<this->OutputLevels->GetNumberOfTuples(); ++i)
  {
    if (this->OutputLevels->GetValue(i) == -1)
    {
      return true;
    }
  }

  return false;
}
