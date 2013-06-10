/*==========================================================================

  Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

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

==========================================================================*/

#ifndef __vtkSlicerVffFileReaderLogic_h
#define __vtkSlicerVffFileReaderLogic_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"

// STD includes
#include <vector>

// VffFileReader includes
#include "vtkSlicerVffFileReaderLogicExport.h"

class vtkMRMLScalarVolumeNode;
class vtkMRMLScalarVolumeDisplayNode;
class vtkMRMLVolumeHeaderlessStorageNode;
class vtkStringArray;


class VTK_SLICER_VFFFILEREADER_LOGIC_EXPORT vtkSlicerVffFileReaderLogic :
  public vtkSlicerModuleLogic
{
public:
  static vtkSlicerVffFileReaderLogic *New();
  vtkTypeMacro(vtkSlicerVffFileReaderLogic, vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// Load VFF volume from file
  /// \param filename Path and filename of the VFF file
  void LoadVffFile(char* filename);

protected:
  /// A helper function which removes all spaces from the beginning and end of a string, and returns the modified string.
  /// \param stringToTrim String which is to be modified
  /// \return String that has had leading and trailing spaces removed
  std::string TrimSpacesFromEndsOfString(std::string &stringToTrim);

  /// A helper function which extracts a user-specified number of numbers from a string, which can be of any numerical type, and returns them as a vector.
  /// \param stringToParse String from which the numbers are to be extracted
  /// \param successFlag Boolean flag which is set to true if the extraction of numbers is successful, and is set to false otherwise
  /// \param numberOfNumbers Integer specifying the number of numbers to be extracted from the string and returned in the vector
  /// \return Vector containing the extracted numbers
  template <class Num> std::vector<Num> ParseNumberOfNumbersFromString(std::string stringToParse, bool &successFlag, int numberOfNumbers);

  /// A helper function which extracts the string following the first "=" in the original string, removes spaces from the beginning and end of the extracted string, and returns the modified string.
  /// \param stringToParse String from the returned string is extracted
  /// \param successFlag Boolean flag which is set to true if the extraction of the string following the "=" is successful, and is set to false otherwise
  /// \return String which has been extracted from the provided string
  std::string GetValueForHeaderItem(std::string stringToParse, bool &successFlag);

protected:
  vtkSlicerVffFileReaderLogic();
  virtual ~vtkSlicerVffFileReaderLogic();

private:
  vtkSlicerVffFileReaderLogic(const vtkSlicerVffFileReaderLogic&); // Not implemented
  void operator=(const vtkSlicerVffFileReaderLogic&);               // Not implemented
};

#endif