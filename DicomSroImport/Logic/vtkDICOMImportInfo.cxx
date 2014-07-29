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

  This file was originally developed by Andras Lasso, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

// ModuleTemplate includes
#include "vtkDICOMImportInfo.h"

// MRML includes

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkStringArray.h>

// STD includes
#include <cassert>
#include <deque>

//----------------------------------------------------------------------------
struct Loadable
{
  vtkStringArray *files;
  std::string name;
  std::string tooltip;    
  std::string warning;  
  bool selected;
  double confidence;
};

//----------------------------------------------------------------------------
struct DicomImportInfoPrivate
{
  std::deque<vtkStringArray*> FileLists;
  std::deque<Loadable> Loadables;
};


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkDICOMImportInfo);

//---------------------------------------------------------------------------
vtkDICOMImportInfo::vtkDICOMImportInfo()
{
  this->PrivateData=new DicomImportInfoPrivate;
}

//---------------------------------------------------------------------------
vtkDICOMImportInfo::~vtkDICOMImportInfo()
{
  RemoveAllFileLists();
  RemoveAllLoadables();  
  delete this->PrivateData;
  this->PrivateData=NULL;
}

//----------------------------------------------------------------------------
void vtkDICOMImportInfo::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkStringArray* vtkDICOMImportInfo::GetLoadableFiles(unsigned int loadableIndex)
{
  if (loadableIndex>=this->PrivateData->Loadables.size())
  {
    vtkErrorMacro("Invalid loadable index" << loadableIndex);
    return NULL;
  }
  return this->PrivateData->Loadables[loadableIndex].files;
}

//----------------------------------------------------------------------------
const char* vtkDICOMImportInfo::GetLoadableName(unsigned int loadableIndex)
{
  if (loadableIndex>=this->PrivateData->Loadables.size())
  {
    vtkErrorMacro("Invalid loadable index" << loadableIndex);
    return NULL;
  }
  return this->PrivateData->Loadables[loadableIndex].name.c_str();
}

//----------------------------------------------------------------------------
const char* vtkDICOMImportInfo::GetLoadableTooltip(unsigned int loadableIndex)
{
  if (loadableIndex>=this->PrivateData->Loadables.size())
  {
    vtkErrorMacro("Invalid loadable index" << loadableIndex);
    return NULL;
  }
  return this->PrivateData->Loadables[loadableIndex].tooltip.c_str();
}

//----------------------------------------------------------------------------
const char* vtkDICOMImportInfo::GetLoadableWarning(unsigned int loadableIndex)
{
  if (loadableIndex>=this->PrivateData->Loadables.size())
  {
    vtkErrorMacro("Invalid loadable index" << loadableIndex);
    return NULL;
  }
  return this->PrivateData->Loadables[loadableIndex].warning.c_str();
}

//----------------------------------------------------------------------------
bool vtkDICOMImportInfo::GetLoadableSelected(unsigned int loadableIndex)
{
  if (loadableIndex>=this->PrivateData->Loadables.size())
  {
    vtkErrorMacro("Invalid loadable index" << loadableIndex);
    return false;
  }
  return this->PrivateData->Loadables[loadableIndex].selected;
}

//----------------------------------------------------------------------------
bool vtkDICOMImportInfo::GetLoadableConfidence(unsigned int loadableIndex)
{
  if (loadableIndex>=this->PrivateData->Loadables.size())
  {
    vtkErrorMacro("Invalid loadable index" << loadableIndex);
    return false;
  }
  return this->PrivateData->Loadables[loadableIndex].confidence;
}

//----------------------------------------------------------------------------
int vtkDICOMImportInfo::InsertNextFileList()
{
  vtkStringArray* fileList=vtkStringArray::New();
  this->PrivateData->FileLists.push_back(fileList);
  return this->PrivateData->FileLists.size()-1;
}

//----------------------------------------------------------------------------
vtkStringArray* vtkDICOMImportInfo::GetFileList(unsigned int fileListIndex)
{
  if (fileListIndex>=this->PrivateData->FileLists.size())
  {
    vtkErrorMacro("Invalid file lists index" << fileListIndex);
    return NULL;
  }
  return this->PrivateData->FileLists[fileListIndex];
}

//----------------------------------------------------------------------------
int vtkDICOMImportInfo::GetNumberOfFileLists()
{
  return this->PrivateData->FileLists.size();
}

//----------------------------------------------------------------------------
void vtkDICOMImportInfo::RemoveAllFileLists()
{
  for (unsigned int i=0; i<this->PrivateData->FileLists.size(); i++)
  {
    if (this->PrivateData->FileLists[i]!=NULL)
    {
      this->PrivateData->FileLists[i]->Delete();
      this->PrivateData->FileLists[i]=NULL;
    }
  }
  this->PrivateData->FileLists.clear();
}

//----------------------------------------------------------------------------
void vtkDICOMImportInfo::RemoveAllLoadables()
{
  for (unsigned int i=0; i<this->PrivateData->Loadables.size(); i++)
  {
    if (this->PrivateData->Loadables[i].files!=NULL)
    {
      this->PrivateData->Loadables[i].files->Delete();
      this->PrivateData->Loadables[i].files=NULL;
    }
  }  
  this->PrivateData->Loadables.clear();
}

//----------------------------------------------------------------------------
int vtkDICOMImportInfo::InsertNextLoadable(vtkStringArray* files, const char* name, const char* tooltip, const char* warning, bool selected, double confidence)
{
  Loadable loadable;
  loadable.files=files;
  loadable.files->Register(NULL);
  loadable.name=name;
  loadable.tooltip=tooltip;
  loadable.warning=warning;
  loadable.selected=selected;
  loadable.confidence=confidence;
  this->PrivateData->Loadables.push_back(loadable);
  return this->PrivateData->Loadables.size()-1;
}

//----------------------------------------------------------------------------
int vtkDICOMImportInfo::GetNumberOfLoadables()
{
  return this->PrivateData->Loadables.size();
}
