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
  /// \param useImageIntensityScaleAndOffsetFromFile Boolean flag which is set to false by default, but is set to true to use the intensity scale and offset provided in the file to load the image
  void LoadVffFile(char* filename, bool useImageIntensityScaleAndOffsetFromFile = false); //, bool useDataOffset);

protected:
  /// A helper function which removes all spaces from the beginning and end of a string, and returns the modified string.
  /// \param stringToTrim String which is to be modified
  /// \return String that has had leading and trailing spaces removed
  std::string TrimSpacesFromEndsOfString(std::string &stringToTrim);

  /// A helper function which extracts a user-specified number of numbers from a string, which can be of any numerical type, and returns them as a vector.
  /// \param stringToParse String from which the numbers are to be extracted
  /// \param numberOfNumbers Integer specifying the number of numbers to be extracted from the string and returned in the vector
  /// \return Vector containing the extracted numbers
  template <class Num> std::vector<Num> ParseNumberOfNumbersFromString(std::string stringToParse, unsigned int numberOfNumbers);

  bool ReadVffFileHeader(ifstream &readFileStream, std::map<std::string, std::string> &parameterList);

protected:
  vtkSlicerVffFileReaderLogic();
  virtual ~vtkSlicerVffFileReaderLogic();

private:
  vtkSlicerVffFileReaderLogic(const vtkSlicerVffFileReaderLogic&); // Not implemented
  void operator=(const vtkSlicerVffFileReaderLogic&);               // Not implemented
};

#endif