/*==========================================================================

  Copyright (c) Laboratory for Percutaneous Surgery (PerkLab)
  Queen's University, Kingston, ON, Canada. All Rights Reserved.
 
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

// VffFileReader includes
#include "vtkSlicerVffFileReaderLogic.h"

// VTK includes
#include <vtkImageData.h>
#include <vtkMatrix4x4.h>
#include <vtkImageShiftScale.h>
#include <vtkObjectFactory.h>

// MRML includes
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLScalarVolumeDisplayNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLSelectionNode.h>

// Slicer logic includes
#include <vtkSlicerApplicationLogic.h>

// STD includes
#include <vector>
#include <fstream>
#include <string>
#include <algorithm>
#include <cctype>
#include <functional>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerVffFileReaderLogic);

//----------------------------------------------------------------------------
vtkSlicerVffFileReaderLogic::vtkSlicerVffFileReaderLogic()
{
}

//----------------------------------------------------------------------------
vtkSlicerVffFileReaderLogic::~vtkSlicerVffFileReaderLogic()
{
}

//----------------------------------------------------------------------------
std::string vtkSlicerVffFileReaderLogic::TrimSpacesFromEndsOfString(std::string &stringToTrim)
{
  // Trim spaces from the beginning of the string
  stringToTrim.erase(stringToTrim.begin(), std::find_if(stringToTrim.begin(), stringToTrim.end(), std::not1(std::ptr_fun<int, int>(std::isspace)))); 

   // Trim spaces from the end of the string
  stringToTrim.erase(std::find_if(stringToTrim.rbegin(), stringToTrim.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), stringToTrim.end()); 
  
  return stringToTrim;
}

//----------------------------------------------------------------------------
bool vtkSlicerVffFileReaderLogic::ReadVffFileHeader(ifstream &readFileStream, std::map<std::string, std::string> &parameterList)
{
  std::string currentStringFromFile = "";
  while(readFileStream.good())
  {
    char nextCharacter;
    nextCharacter = readFileStream.get();

    // The form feed character and the following line feed indicate the beginning of the image data
    if (nextCharacter == '\f')
    {
      break;
    }

    // Parameters are separated from each other using semicolons and line feeds
    else if (nextCharacter == ';')
    {
      if (currentStringFromFile.compare("ncaa") != 0)
      {
        std::string parameterType;
        std::string parameterValue;
        size_t locationToSplitString = currentStringFromFile.find("=", 0);
        if (locationToSplitString != std::string::npos)
        {
          parameterType = currentStringFromFile.substr(0, locationToSplitString);
          parameterType = this->TrimSpacesFromEndsOfString(parameterType);
          parameterValue = currentStringFromFile.substr(locationToSplitString+1);
          parameterValue = this->TrimSpacesFromEndsOfString(parameterValue);
          if (parameterValue.size() <= 0)
          {
            vtkWarningMacro("ReadVffFileHeader: Nothing follows the equal sign in header item: '" << parameterType << "'");
          }
          else
          {
            parameterList[parameterType] = parameterValue;
          }
        }
        else
        {
          vtkWarningMacro("ReadVffFileHeader: No equal sign in header item '" << parameterType << "'");
        }
      }
      currentStringFromFile = "";
    }
    else if (nextCharacter != '\n')
    {
      nextCharacter = ::tolower(nextCharacter);
      currentStringFromFile += nextCharacter;
    }
  }

  return true;
}

//----------------------------------------------------------------------------
template <class Num> 
std::vector<Num> vtkSlicerVffFileReaderLogic::ParseNumberOfNumbersFromString(std::string stringToParse, unsigned int numberOfNumbers)
{
  std::vector<Num> vectorOfNumberOfNumbers (numberOfNumbers, 0);
  if (numberOfNumbers == 0)
  {
    return vectorOfNumberOfNumbers;
  }

  stringToParse = this->TrimSpacesFromEndsOfString(stringToParse);
  unsigned int currentNumber = 0;
  // Parses out the specified number of numbers from the string, 
  // assuming that numbers are separated by commas, spaces, or both
  while (currentNumber < numberOfNumbers)
  {
    size_t locationToSplitString = stringToParse.find_first_of(",", 0);
    std::string stringContainingNumber;
    if (locationToSplitString != std::string::npos)
    {
      stringContainingNumber = stringToParse.substr(0,locationToSplitString);
      stringToParse = stringToParse.substr(locationToSplitString+1);
    }
    else
    {
      size_t locationToSplitString = stringToParse.find_first_of(" ", 0);
      if (locationToSplitString != std::string::npos)
      {
        stringContainingNumber = stringToParse.substr(0,locationToSplitString);
        stringToParse = stringToParse.substr(locationToSplitString+1);
      }
      else
      {
        stringContainingNumber = stringToParse;
      }
    }
    stringContainingNumber = this->TrimSpacesFromEndsOfString(stringContainingNumber);
    if (stringContainingNumber.empty())
    {
      return vectorOfNumberOfNumbers;
    }
    std::stringstream convertStringToNumber(stringContainingNumber);
    Num parsedNumber;
    // Converts the parsed string into the number
    convertStringToNumber >> parsedNumber;
    std::string checkForConversionSuccess;

    // Checks that the string was correctly converted into the number,
    // which would result in nothing left in the stream
    if (convertStringToNumber >> checkForConversionSuccess)
    {
      return vectorOfNumberOfNumbers;
    }
    else
    {
      vectorOfNumberOfNumbers[currentNumber] = parsedNumber;
    }
    currentNumber++;
    stringToParse = this->TrimSpacesFromEndsOfString(stringToParse);
  }
  return vectorOfNumberOfNumbers;

}

//----------------------------------------------------------------------------
void vtkSlicerVffFileReaderLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSlicerVffFileReaderLogic::LoadVffFile(char *filename, bool useImageIntensityScaleAndOffsetFromFile)
{
  ifstream readFileStream;
  readFileStream.open(filename, std::ios::binary);
  if (!(readFileStream.fail()))
  {
    int rank = 0;
    std::string type = "";
    std::string format = "";
    int bits = 0;
    int bands = 0;
    int size[3] = {0, 0, 0};
    double spacing[3] = {0, 0 ,0};
    double origin[3] = {0, 0, 0};
    int rawsize = 0;
    double data_scale = 0;
    double data_offset = 0;
    std::string handleScatter;
    double referenceScatterFactor = 0;
    double dataScatterFactor = 0;
    std::string filter = "";
    std::string title = "";
    std::string name = "";
    std::string date = "";
    bool isDateCurrentlySet = false;
    std::map<std::string, std::string> parameterList;
    bool parameterMissing = false;
    bool parameterInvalidValue = false;

    bool headerParseSuccess = this->ReadVffFileHeader(readFileStream, parameterList);
    if (headerParseSuccess == false)
    {
      vtkErrorMacro("LoadVffFile: The header did not parse correctly.");
    }

    // For each of the known parameters, interprets the string associated with the parameter into the correct format and sets the corresponding variable, as well as checking the correctness of the value
    std::vector<int> numberFromParsedStringRank = this->ParseNumberOfNumbersFromString<int>(parameterList["rank"], 1);
    if (numberFromParsedStringRank.empty()) 
    {
      vtkErrorMacro("LoadVffFile: An integer was not entered for the rank. The value entered for the rank must be 3.");
      parameterMissing = true;
    }
    else 
    {
      rank = numberFromParsedStringRank[0];
      if (rank != 3)
      {
        vtkErrorMacro("LoadVffFile: The value entered for the rank must be 3.");
        parameterInvalidValue = true;
      }
    }

    if (parameterList["type"].empty())
    {
      vtkErrorMacro("LoadVffFile: A string was not entered for the type. The value must be separated from the parameter with an '='. The value entered for the type must be raster.");
      parameterMissing = true;
    }
    else
    {
      type = parameterList["type"];
      if (type.compare("raster") != 0)
      {
        vtkErrorMacro("LoadVffFile: The value entered for the type must be raster.");
        parameterInvalidValue = true;
      }
    }

    if (parameterList["format"].empty())
    {
      vtkErrorMacro("LoadVffFile: A string was not entered for the format. The value must be separated from the parameter with an '='. The value entered for the format must be slice.");
      parameterMissing = true;
    }
    else
    {
      format = parameterList["format"];
      if (format.compare("slice") != 0)
      {
        vtkErrorMacro("LoadVffFile: The value entered for the format must be slice.");
        parameterInvalidValue = true;
      }
    }

    std::vector<int> numberFromParsedStringBits = this->ParseNumberOfNumbersFromString<int>(parameterList["bits"], 1);
    if (numberFromParsedStringBits.empty()) 
    {
      vtkErrorMacro("LoadVffFile: An integer was not entered for the bits. The value must be divisible by 8.");
      parameterMissing = true;
    }
    else
    {
      bits = numberFromParsedStringBits[0];
      if (bits % 8 != 0)
      {
        vtkErrorMacro("LoadVffFile: The value entered for the bits must be divisible by 8.");
        parameterInvalidValue = true;
      }
    }

    std::vector<int> numberFromParsedStringBands = this->ParseNumberOfNumbersFromString<int>(parameterList["bands"], 1);
    if (numberFromParsedStringBands.empty()) 
    {
      vtkErrorMacro("LoadVffFile: An integer was not entered for the bands. The value entered for the bands must be 1.");
      parameterMissing = true;
    }
    else
    {
      bands = numberFromParsedStringBands[0];
      if (bands != 1)
      {
        vtkErrorMacro("LoadVffFile: The value entered for the bands must be 1.");
        parameterInvalidValue  = true;
      }
    }

    std::vector<int> numbersFromParsedStringSize = this->ParseNumberOfNumbersFromString<int>(parameterList["size"], 3);
    if (numbersFromParsedStringSize.empty())
    {
      vtkErrorMacro("LoadVffFile: 3 integers were not entered for the size.");
      parameterMissing = true;
    }
    else
    {
      size[0] = numbersFromParsedStringSize[0];
      size[1] = numbersFromParsedStringSize[1];
      size[2] = numbersFromParsedStringSize[2];
      if (size[0] <= 0 || size[1] <=0 || size[2] <= 0)
      {
        vtkErrorMacro("LoadVffFile: The values for the size must each be greater than 0.");
        parameterInvalidValue = true;
      }
    }

    std::vector<double> numbersFromParsedStringSpacing = this->ParseNumberOfNumbersFromString<double>(parameterList["spacing"], 3);
    if (numbersFromParsedStringSpacing.empty())
    {
      vtkErrorMacro("LoadVffFile: 3 doubles were not entered for the spacing.");
      parameterMissing = true;
    }
    else
    {
      spacing[0] = numbersFromParsedStringSpacing[0];
      spacing[1] = numbersFromParsedStringSpacing[1];
      spacing[2] = numbersFromParsedStringSpacing[2];
      if (spacing[0] < 0 || spacing[1] <0 || spacing[2] < 0)
      {
        vtkErrorMacro("LoadVffFile: The values for the spacing must each be greater than or equal to 0.");
        parameterInvalidValue = true;
      }
    }

    std::vector<double> numbersFromParsedStringOrigin = this->ParseNumberOfNumbersFromString<double>(parameterList["origin"], 3);
    if (numbersFromParsedStringOrigin.empty())
    {
      vtkErrorMacro("LoadVffFile: 3 doubles were not entered for the origin.");
      parameterMissing = true;
    }
    else
    {
      origin[0] = numbersFromParsedStringOrigin[0];
      origin[1] = numbersFromParsedStringOrigin[1];
      origin[2] = numbersFromParsedStringOrigin[2];
      if (origin[0] < 0 || origin[1] <0 || origin[2] < 0)
      {
        vtkErrorMacro("LoadVffFile: The values for the origin must each be greater than or equal to 0.");
        parameterInvalidValue = true;
      }
    }          

    std::vector<int> numberFromParsedStringRawsize = this->ParseNumberOfNumbersFromString<int>(parameterList["rawsize"], 1);
    if (numberFromParsedStringRawsize.empty()) 
    {
      vtkErrorMacro("LoadVffFile: An integer was not entered for the rawsize.");
      parameterMissing = true;
    }
    else 
    {
      rawsize = numberFromParsedStringRawsize[0];
      if (rawsize <= 0)
      {
        vtkErrorMacro("LoadVffFile: The value entered for the rawsize must be greater than or equal to 0.");
        parameterInvalidValue = true;
      }
    }

    std::vector<double> numberFromParsedStringDataScale = this->ParseNumberOfNumbersFromString<double>(parameterList["data_scale"], 1);
    if (numberFromParsedStringDataScale.empty())
    {
      vtkErrorMacro("LoadVffFile: A double was not entered for the data_scale.");
      parameterMissing = true;
    }
    else
    {
      data_scale = numberFromParsedStringDataScale[0];
    }

    std::vector<double> numberFromParsedStringDataOffset = this->ParseNumberOfNumbersFromString<double>(parameterList["data_offset"], 1);
    if (numberFromParsedStringDataOffset.empty())
    {
      vtkErrorMacro("LoadVffFile: A double was not entered for the data_offset. The value entered must be 0.");
      parameterMissing = true;
    }
    else
    {
      data_offset = numberFromParsedStringDataOffset[0];
    }

    if (parameterList["handlescatter"].empty())
    {
      vtkErrorMacro("LoadVffFile: A string was not entered for Handle Scatter. The value must be separated from the parameter with an '='. The value entered for Handle Scatter must be factor.");
      parameterMissing = true;
    }
    else
    {
      handleScatter = parameterList["handlescatter"]; 
      if (handleScatter.compare("factor") != 0)
      {
        vtkErrorMacro("LoadVffFile: The value entered for Handle Scatter must be factor.");
        parameterInvalidValue = true;
      }
    }

    std::vector<double> numberFromParsedStringReferenceScatterFactor = this->ParseNumberOfNumbersFromString<double>(parameterList["referencescatterfactor"], 1);
    if (numberFromParsedStringReferenceScatterFactor.empty())
    {
      vtkErrorMacro("LoadVffFile: A double was not entered for the Reference Scatter Factor. The value entered must be 1.");
      parameterMissing = true;
    }
    else
    {
      referenceScatterFactor = numberFromParsedStringReferenceScatterFactor[0];
      if (referenceScatterFactor != 1)
      {
        vtkErrorMacro("LoadVffFile: The value entered for the Reference Scatter Factor must be 1.");
        parameterInvalidValue = true;
      }
    }

    std::vector<double> numberFromParsedStringDataScatterFactor = this->ParseNumberOfNumbersFromString<double>(parameterList["datascatterfactor"], 1);
    if (numberFromParsedStringDataScatterFactor.empty())
    {
      vtkErrorMacro("LoadVffFile: A double was not entered for the Data Scatter Factor. The value entered must be 1.");
      parameterMissing = true;
    }
    else 
    {
      dataScatterFactor = numberFromParsedStringDataScatterFactor[0];
      if (dataScatterFactor != 1)
      {
        vtkErrorMacro("LoadVffFile: The value entered for the Data Scatter Factor must be 1.");
        parameterInvalidValue = true;
      }
    }

    if (parameterList["filter"].empty())
    {
      vtkWarningMacro("LoadVffFile: Empty filter value! The value must be separated from the parameter with an '='");
      //parameterMissing = true;
    }
    else
    {
      filter = parameterList["filter"];
      vtkDebugMacro("LoadVffFile: Used filter for optical CT file is:\n" << filter);
    }

    if (parameterList["title"].empty() == false)
    {
      title = parameterList["title"];
      // Parse out the name of the file from the file path given as the parameter title
      std::string titleStringToParse = title;
      unsigned long locationToSplitString = titleStringToParse.find_last_of("/\\");
      if (locationToSplitString != std::string::npos)
      {
        titleStringToParse = titleStringToParse.substr(locationToSplitString+1);
        titleStringToParse = this->TrimSpacesFromEndsOfString(titleStringToParse);
        name = titleStringToParse;
      }
      else 
      {
        name = titleStringToParse;
      }
    }
    else
    {
      parameterMissing = true;
      vtkErrorMacro("LoadVffFile: A string was not entered for the title.");
    }

    if (parameterList["date"].size() <= 0)
    {
      vtkErrorMacro("LoadVffFile: A string was not entered for the date. The value must be separated from the parameter with an '='.");
    }
    else
    {
      date = parameterList["date"];
      isDateCurrentlySet = true; 
    }

    // Checks that the end of the header has been reached and that all of the required parameters have been set
    if (parameterMissing == false && parameterInvalidValue == false)
    {
      // Calculates the number of bytes to read based on some of the specified parameters
      long bytesToRead = 1;
      bytesToRead = bands*bits/8;
      int sizeOfImageData = (int)size[0]*size[1]*size[2]*(bits/8);

      if (rawsize != sizeOfImageData)
      {
        vtkWarningMacro("LoadVffFile: The specified size from the parameters does not match the specified raw size.");
      }

      vtkSmartPointer<vtkMRMLScalarVolumeNode> vffVolumeNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
      vffVolumeNode->SetScene(this->GetMRMLScene());
      vffVolumeNode->SetName(name.c_str());
      vffVolumeNode->SetSpacing(spacing[0], spacing[1], spacing[2]);
      vffVolumeNode->SetOrigin(origin[0], origin[1], origin[2]);
      vtkSmartPointer<vtkMatrix4x4> lpsToRasMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
      lpsToRasMatrix->SetElement(0,0,-1);
      lpsToRasMatrix->SetElement(1,1,-1);
      vtkSmartPointer<vtkMatrix4x4> vffIjkToLpsMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
      vffVolumeNode->GetIJKToRASMatrix(vffIjkToLpsMatrix);
      vtkSmartPointer<vtkMatrix4x4> vffIjkToRasMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
      vtkMatrix4x4::Multiply4x4(vffIjkToLpsMatrix, lpsToRasMatrix, vffIjkToRasMatrix);
      vffVolumeNode->SetIJKToRASMatrix(vffIjkToRasMatrix);
      vffVolumeNode->SetSlicerDataType(type.c_str());
      this->GetMRMLScene()->AddNode(vffVolumeNode);

      // Checks if the date parameter (not required) was set
      if (isDateCurrentlySet == true)
      {
        vffVolumeNode->SetAttribute("Date", date.c_str());
      }

      vtkSmartPointer<vtkImageData> floatVffVolumeData = vtkSmartPointer<vtkImageData>::New();
      floatVffVolumeData->SetExtent(0, size[0]-1, 0, size[1]-1, 0, size[2]-1);
      floatVffVolumeData->SetSpacing(1, 1, 1);
      floatVffVolumeData->SetOrigin(0, 0, 0);

#if (VTK_MAJOR_VERSION <= 5)
      floatVffVolumeData->SetScalarTypeToFloat();
      floatVffVolumeData->SetNumberOfScalarComponents(bands);
	  floatVffVolumeData->AllocateScalars();
#else
      floatVffVolumeData->AllocateScalars(VTK_FLOAT, bands);
#endif

      // Reads the line feed that comes directly before the image data from the file
      readFileStream.get();

      float* floatPtr = (float*)floatVffVolumeData->GetScalarPointer();
      std::stringstream ss;

      // The size of the image data read is specified in the header by the parameter size
      for (long x = 0; x < size[0]; x++)
      {
        for (long y = 0; y < size[1]; y++)
        {
          for (long z = 0; z < size[2]; z++)
          {
            if (!readFileStream.eof())
            {
              // Reads in the specified number of bytes at a time from the file
              char *buffer = new char[bytesToRead];
              char *newBuffer = new char[bytesToRead];
              readFileStream.read(buffer, bytesToRead);

              // Checks that the correct number of bytes were read from the file
              if (readFileStream.gcount() == bytesToRead) 
              { 
                float *valueFromFile = (float *) newBuffer;

                // Reverses the byte order
                for (int byte=0; byte<bytesToRead; ++byte)
                {
                  newBuffer[byte] = buffer[bytesToRead-byte-1];
                }
                (*floatPtr) = (*valueFromFile);
                ++floatPtr;
              }
              else
              {
                vtkErrorMacro("LoadVffFile: The end of the file was reached earlier than specified.");
              }
            }
            else
            {
              vtkErrorMacro("LoadVffFile: The end of the file was reached earlier than specified.");
            }
          }
        }
      }
      
      if (readFileStream.get() && !readFileStream.eof())
      {
        vtkWarningMacro("LoadVffFile: The end of the file was not reached.");
      }
      
      
      if (useImageIntensityScaleAndOffsetFromFile == true)
      {
        vtkSmartPointer<vtkImageShiftScale> imageIntensityShiftScale = vtkSmartPointer<vtkImageShiftScale>::New();
        imageIntensityShiftScale->SetScale(data_scale);
        imageIntensityShiftScale->SetShift(data_offset);
#if (VTK_MAJOR_VERSION <= 5)
	    imageIntensityShiftScale->SetInput(floatVffVolumeData);
#else
	    imageIntensityShiftScale->SetInputData(floatVffVolumeData);
#endif
        imageIntensityShiftScale->Update();
        floatVffVolumeData = imageIntensityShiftScale->GetOutput();
      }

      vffVolumeNode->SetAndObserveImageData(floatVffVolumeData);

      vtkSmartPointer<vtkMRMLScalarVolumeDisplayNode> vffVolumeDisplayNode = vtkSmartPointer<vtkMRMLScalarVolumeDisplayNode>::New();
      this->GetMRMLScene()->AddNode(vffVolumeDisplayNode);
      vffVolumeNode->SetAndObserveDisplayNodeID(vffVolumeDisplayNode->GetID());

      if (this->GetApplicationLogic()!=NULL)
      {
        if (this->GetApplicationLogic()->GetSelectionNode()!=NULL)
        {
          this->GetApplicationLogic()->GetSelectionNode()->SetReferenceActiveVolumeID(vffVolumeNode->GetID());
          this->GetApplicationLogic()->PropagateVolumeSelection();
          this->GetApplicationLogic()->FitSliceToAll();
        }
      }

      if (bands == 1)
      {
        vffVolumeDisplayNode->SetAndObserveColorNodeID("vtkMRMLColorTableNodeGrey");
      }
      vffVolumeNode->SetAndObserveDisplayNodeID(vffVolumeDisplayNode->GetID());
    }
    else
    {
      vtkErrorMacro("LoadVffFile: Incorrect parameters or required parameters that were not set, vff file failed to load. The required parameters are: rank, type, format, bits, bands, size, spacing, origin, rawsize, data scale, data offset, and title.");
    }   
  }
  else
  {
    vtkErrorMacro("LoadVffFile: The specified file could not be opened.");
  }
    
  readFileStream.close();
}
