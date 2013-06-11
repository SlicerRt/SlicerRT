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

// VffFileReader includes
#include "vtkSlicerVffFileReaderLogic.h"

// VTK includes
#include <vtkImageData.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLScalarVolumeDisplayNode.h>
#include <vtkSlicerApplicationLogic.h>
#include <vtkMRMLSelectionNode.h>

// STD includes
#include <vector>
#include <fstream>
#include <string>
#include <algorithm>
#include <cctype>


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
template <class Num> 
std::vector<Num> vtkSlicerVffFileReaderLogic::ParseNumberOfNumbersFromString(std::string stringToParse, bool &successFlag, int numberOfNumbers)
{
  successFlag = false;
  if (numberOfNumbers > 0)
  {
    std::vector<Num> vectorOfNumberOfNumbers (numberOfNumbers, 0);
    bool parseStringSuccessFlag;

    // Parse out the part of the string that follows the "="
    stringToParse = this->GetValueForHeaderItem(stringToParse, parseStringSuccessFlag);
    if (parseStringSuccessFlag == true)
    {
      stringToParse = this->TrimSpacesFromEndsOfString(stringToParse);
      int currentNumber = 0;
     // bool continueParsingFlag = true;

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
        if (stringContainingNumber.size() > 0)
        {
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
        }
        else
        {
          return vectorOfNumberOfNumbers;
        }
        currentNumber++;
        stringToParse = this->TrimSpacesFromEndsOfString(stringToParse);
      }

      successFlag = true;
      return vectorOfNumberOfNumbers;
    }
    else
    {
      return vectorOfNumberOfNumbers;
    }
  }
  else
  {
    std::vector<Num> vectorOfNumberOfNumbers(1, 0);
    return vectorOfNumberOfNumbers;
  } 
}

//----------------------------------------------------------------------------
std::string vtkSlicerVffFileReaderLogic::GetValueForHeaderItem(std::string stringToParse, bool &successFlag)
{
  successFlag = false;
  size_t locationToSplitString = stringToParse.find("=", 0);
  if (locationToSplitString != std::string::npos)
  {
    stringToParse = stringToParse.substr(locationToSplitString+1);
    stringToParse = this->TrimSpacesFromEndsOfString(stringToParse);
    if (stringToParse.size() > 0)
    {
      successFlag = true;
      return stringToParse;
    }
    else
    {
      vtkErrorMacro("GetValueForHeaderItem: Nothing follows the equal sign in header item");
      return stringToParse;
    }
  }
  else
  {
    vtkErrorMacro("GetValueForHeaderItem: No equal sign in header item '" << stringToParse << "'");
    return stringToParse;
  }
}

//----------------------------------------------------------------------------
void vtkSlicerVffFileReaderLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSlicerVffFileReaderLogic::LoadVffFile(char *filename)
{
  ifstream readFileStream;
  readFileStream.open(filename, std::ios::binary);
  if (!(readFileStream.fail()))
  {
    int rank;
    std::string type;
    std::string format;
    int bits;
    int bands;
    int size[3];
    double spacing[3];
    double origin[3];
    int rawsize;
    double data_scale;
    double data_offset;
    std::string handleScatter;
    double referenceScatterFactor;
    double dataScatterFactor;
    std::string filter;
    std::string title;
    std::string name;
    std::string date;

    bool parameterParseSuccess = true;
    bool stopReadingFlag = false;
    std::string currentStringFromFile = "";
    std::vector<bool> parametersCurrentlySet(12, false);
    std::vector<bool> parametersToSet(12, true);
    bool isDateCurrentlySet = false;

    while(readFileStream.good() && !stopReadingFlag)
    {
      char nextCharacter;
      nextCharacter = readFileStream.get();

      // The form feed character and the following line feed indicate the beginning of the image data
      if (nextCharacter == '\f')
      {
        stopReadingFlag = true;
      }

      // Parameters are separated from each other using semicolons and line feeds
      else if (nextCharacter == ';')
      {

        // Checks for one of the known parameters, and if found, parses out the parameter value and sets the correct variable
        if (currentStringFromFile.find("rank", 0) != -1)
        {
          bool parseStringSuccessFlag = false;
          std::vector<int> numberFromParsedString(1,0);
          numberFromParsedString = this->ParseNumberOfNumbersFromString<int>(currentStringFromFile, parseStringSuccessFlag, 1);
          if (parseStringSuccessFlag == false) 
          {
            vtkErrorMacro("LoadVffFile: An integer was not entered for the rank. The value entered for the rank must be 3.");
            parameterParseSuccess = false;
          }
          else if (parseStringSuccessFlag == true && numberFromParsedString[0] != 3)
          {
            vtkErrorMacro("LoadVffFile: The value entered for the rank must be 3.");
            parameterParseSuccess  = false;
          }
          else 
          {
            rank = numberFromParsedString[0];
            parametersCurrentlySet[0] = true;
          }
        }

        else if (currentStringFromFile.find("type", 0) != std::string::npos)
        {
          bool parseStringSuccessFlag = false;
          std::string parsedString;
          parsedString = this->GetValueForHeaderItem(currentStringFromFile, parseStringSuccessFlag);
          if (parseStringSuccessFlag == false)
          {
            vtkErrorMacro("LoadVffFile: A string was not entered for the type. The value must be separated from the parameter with an '='. The value entered for the type must be raster.");
            parameterParseSuccess = false;
          }
          else if (parseStringSuccessFlag == true && parsedString.compare("raster") != 0)
          {
            vtkErrorMacro("LoadVffFile: The value entered for the type must be raster.");
            parameterParseSuccess = false;
          }
          else
          {
            type = parsedString;
            parametersCurrentlySet[1] = true;
          }
        }

        else if (currentStringFromFile.find("format", 0) != std::string::npos)
        {
          bool parseStringSuccessFlag = false;
          std::string parsedString;
          parsedString = this->GetValueForHeaderItem(currentStringFromFile, parseStringSuccessFlag);
          if (parseStringSuccessFlag == false)
          {
            vtkErrorMacro("LoadVffFile: A string was not entered for the format. The value must be separated from the parameter with an '='. The value entered for the format must be slice.");
            parameterParseSuccess = false;
          }
          else if (parseStringSuccessFlag == true && parsedString.compare("slice") != 0)
          {
            vtkErrorMacro("LoadVffFile: The value entered for the format must be slice.");
            parameterParseSuccess = false;
          }
          else
          {
            format = parsedString;
            parametersCurrentlySet[2] = true;
          }
        }

        else if (currentStringFromFile.find("bits", 0) != std::string::npos)
         {
          bool parseStringSuccessFlag = false;
          std::vector<int> numberFromParsedString(1,0);
          numberFromParsedString = this->ParseNumberOfNumbersFromString<int>(currentStringFromFile, parseStringSuccessFlag, 1);
          if (parseStringSuccessFlag == false) 
          {
            vtkErrorMacro("LoadVffFile: An integer was not entered for the bits. The value must be divisible by 8.");
            parameterParseSuccess = false;
          }
          else if (parseStringSuccessFlag == true && numberFromParsedString[0] % 8 != 0)
          {
            vtkErrorMacro("LoadVffFile: The value entered for the bits must be divisible by 8.");
            parameterParseSuccess = false;
          }
          else 
          {
            bits = numberFromParsedString[0];
            parametersCurrentlySet[3] = true;
          }
        }

        else if (currentStringFromFile.find("bands", 0) != std::string::npos)
        {
          bool parseStringSuccessFlag = false;
          std::vector<int> numberFromParsedString(1,0);
          numberFromParsedString = this->ParseNumberOfNumbersFromString<int>(currentStringFromFile, parseStringSuccessFlag, 1);
          if (parseStringSuccessFlag == false) 
          {
            vtkErrorMacro("LoadVffFile: An integer was not entered for the bands. The value entered for the bands must be 1.");
            parameterParseSuccess = false;
          }
          else if (parseStringSuccessFlag == true && numberFromParsedString[0] != 1)
          {
            vtkErrorMacro("LoadVffFile: The value entered for the bands must be 1.");
            parameterParseSuccess  = false;
          }
          else 
          {
            bands = numberFromParsedString[0];
            parametersCurrentlySet[4] = true;
          }

        }

        else if (currentStringFromFile.find("size", 0) != std::string::npos && currentStringFromFile.find("rawsize", 0) == std::string::npos)
        {
          bool parseStringSuccessFlag = false;
          std::vector<int> numbersFromParsedString (3, 0);
          numbersFromParsedString = this->ParseNumberOfNumbersFromString<int>(currentStringFromFile, parseStringSuccessFlag, 3);
          if (parseStringSuccessFlag == true)
          {
            size[0] = numbersFromParsedString[0];
            size[1] = numbersFromParsedString[1];
            size[2] = numbersFromParsedString[2];
            parametersCurrentlySet[5] = true;
          }
          else
          {
            vtkErrorMacro("LoadVffFile: 3 integers were not entered for the size.");
          }
        }

        else if (currentStringFromFile.find("spacing", 0) != std::string::npos)
        {
          bool parseStringSuccessFlag = false;
          std::vector<double> numbersFromParsedString (3, 0);
          numbersFromParsedString = this->ParseNumberOfNumbersFromString<double>(currentStringFromFile, parseStringSuccessFlag, 3);
          if (parseStringSuccessFlag == true)
          {
            spacing[0] = numbersFromParsedString[0];
            spacing[1] = numbersFromParsedString[1];
            spacing[2] = numbersFromParsedString[2];
            parametersCurrentlySet[6] = true;
          }
          else
          {
            vtkErrorMacro("LoadVffFile: 3 doubles were not entered for the spacing.");
          }
        }

        else if (currentStringFromFile.find("origin", 0) != std::string::npos)
        {
          bool parseStringSuccessFlag = false;
          std::vector<double> numbersFromParsedString (3, 0);
          numbersFromParsedString = this->ParseNumberOfNumbersFromString<double>(currentStringFromFile, parseStringSuccessFlag, 3);
          if (parseStringSuccessFlag == true)
          {
            origin[0] = numbersFromParsedString[0];
            origin[1] = numbersFromParsedString[1];
            origin[2] = numbersFromParsedString[2];
            parametersCurrentlySet[7] = true;
          }
          else
          {
            vtkErrorMacro("LoadVffFile: 3 doubles were not entered for the origin.");
          }          
        }

        else if (currentStringFromFile.find("rawsize", 0) != std::string::npos)
        {
          bool parseStringSuccessFlag = false;
          std::vector<int> numberFromParsedString(1,0);
          numberFromParsedString = this->ParseNumberOfNumbersFromString<int>(currentStringFromFile, parseStringSuccessFlag, 1);
          if (parseStringSuccessFlag == false) 
          {
            vtkErrorMacro("LoadVffFile: An integer was not entered for the rawsize.");
            parameterParseSuccess = false;
          }
          else 
          {
            rawsize = numberFromParsedString[0];
            parametersCurrentlySet[8] = true;
          }
        }

        else if (currentStringFromFile.find("data_scale", 0) != std::string::npos)
        {
          bool parseStringSuccessFlag = false;
          std::vector<double> numberFromParsedString(1,0);
          numberFromParsedString = this->ParseNumberOfNumbersFromString<double>(currentStringFromFile, parseStringSuccessFlag, 1);
          if (parseStringSuccessFlag == false)
          {
            vtkErrorMacro("LoadVffFile: A double was not entered for the data_scale.");
            parameterParseSuccess = false;
          }
          else
          {
            data_scale = numberFromParsedString[0];
            parametersCurrentlySet[9] = true;
          }
        }

        else if (currentStringFromFile.find("data_offset", 0) != std::string::npos)
        {
          bool parseStringSuccessFlag = false;
          std::vector<double> numberFromParsedString(1,0);
          numberFromParsedString = this->ParseNumberOfNumbersFromString<double>(currentStringFromFile, parseStringSuccessFlag, 1);
          if (parseStringSuccessFlag == false)
          {
            vtkErrorMacro("LoadVffFile: A double was not entered for the data_offset. The value entered must be 0.");
            parameterParseSuccess = false;
          }
          else if (parseStringSuccessFlag == true && numberFromParsedString[0] != 0)
          {
            vtkErrorMacro("LoadVffFile: The value entered for the data_offset must be 0.");
            parameterParseSuccess = false;
          }
          else
          {
            data_offset = numberFromParsedString[0];
            parametersCurrentlySet[10] = true;
          }
        }

        else if (currentStringFromFile.find("handlescatter", 0) != std::string::npos)
        {
          bool parseStringSuccessFlag = false;
          std::string parsedString;
          parsedString = this->GetValueForHeaderItem(currentStringFromFile, parseStringSuccessFlag);
          if (parseStringSuccessFlag == false)
          {
            vtkErrorMacro("LoadVffFile: A string was not entered for Handle Scatter. The value must be separated from the parameter with an '='. The value entered for Handle Scatter must be factor.");
            parameterParseSuccess = false;
          }
          else if (parseStringSuccessFlag == true && parsedString.compare("factor") != 0)
          {
            vtkErrorMacro("LoadVffFile: The value entered for Handle Scatter must be factor.");
            parameterParseSuccess = false;
          }
          else
          {
            handleScatter = parsedString; 
          }
        }

        else if (currentStringFromFile.find("referencescatterfactor", 0) != std::string::npos)
        {
          bool parseStringSuccessFlag = false;
          std::vector<double> numberFromParsedString(1,0);
          numberFromParsedString = this->ParseNumberOfNumbersFromString<double>(currentStringFromFile, parseStringSuccessFlag, 1);
          if (parseStringSuccessFlag == false)
          {
            vtkErrorMacro("LoadVffFile: A double was not entered for the Reference Scatter Factor. The value entered must be 1.");
            parameterParseSuccess = false;
          }
          else if (parseStringSuccessFlag == true && numberFromParsedString[0] != 1)
          {
            vtkErrorMacro("LoadVffFile: The value entered for the Reference Scatter Factor must be 1.");
            parameterParseSuccess = false;
          }
          else
          {
            referenceScatterFactor = numberFromParsedString[0];
          }
        }

        else if (currentStringFromFile.find("datascatterfactor", 0) != std::string::npos)
        {
          bool parseStringSuccessFlag = false;
          std::vector<double> numberFromParsedString(1,0);
          numberFromParsedString = this->ParseNumberOfNumbersFromString<double>(currentStringFromFile, parseStringSuccessFlag, 1);
          if (parseStringSuccessFlag == false)
          {
            vtkErrorMacro("LoadVffFile: A double was not entered for the Data Scatter Factor. The value entered must be 1.");
            parameterParseSuccess = false;
          }
          else if (parseStringSuccessFlag == true && numberFromParsedString[0] != 1)
          {
            vtkErrorMacro("LoadVffFile: The value entered for the Data Scatter Factor must be 1.");
            parameterParseSuccess = false;
          }
          else
          {
            dataScatterFactor = numberFromParsedString[0];
          }
        }

        else if (currentStringFromFile.find("filter", 0) != std::string::npos)
        {
          bool parseStringSuccessFlag = false;
          std::string parsedString;
          parsedString = this->GetValueForHeaderItem(currentStringFromFile, parseStringSuccessFlag);
          if (parseStringSuccessFlag == false)
          {
            vtkErrorMacro("LoadVffFile: A string was not entered for the filter. The value must be separated from the parameter with an '='. The value entered for the filter must be Ram-Lak.");
            parameterParseSuccess = false;
          }
          else if (parseStringSuccessFlag == true && parsedString.compare("ram-lak") != 0)
          {
            vtkErrorMacro("LoadVffFile: The value entered for filter must be Ram-Lak.");
          }
          else
          {
            filter = parsedString; 
          }
        }

        else if (currentStringFromFile.find("title", 0) != std::string::npos)
        {
          bool parseStringSuccessFlag = false;
          currentStringFromFile = this->GetValueForHeaderItem(currentStringFromFile, parseStringSuccessFlag);
          if (parseStringSuccessFlag == true)
          {
            currentStringFromFile = this->TrimSpacesFromEndsOfString(currentStringFromFile);
            if (currentStringFromFile.size() > 0)
            {
              title = currentStringFromFile; 

              // Parse out the name of the file from the file path given as the parameter title
              int locationToSplitString = currentStringFromFile.find_last_of("/\\");
              if (locationToSplitString != std::string::npos)
              {
                currentStringFromFile = currentStringFromFile.substr(locationToSplitString+1);
                currentStringFromFile = this->TrimSpacesFromEndsOfString(currentStringFromFile);
                name = currentStringFromFile;
                parametersCurrentlySet[11] = true;
              }
              else 
              {
                name = currentStringFromFile;
                parametersCurrentlySet[11] = true;
              }
            }
            else
            {
              parameterParseSuccess = false;
              vtkErrorMacro("LoadVffFile: A string was not entered for the title.");
            }
          }
          else
          {
            parameterParseSuccess = false;
            vtkErrorMacro("LoadVffFile: The value must be separated from the parameter with an '='.");
          }          
        }

        else if (currentStringFromFile.find("date", 0) != std::string::npos)
        {
          bool parseStringSuccessFlag = false;
          std::string parsedString = this->GetValueForHeaderItem(currentStringFromFile, parseStringSuccessFlag);
          if (parseStringSuccessFlag == true)
          {
            date = parsedString;
            isDateCurrentlySet = true;
          }
          else
          {
            parameterParseSuccess = false;
            vtkErrorMacro("LoadVffFile: A string was not entered for the date. The value must be separated from the parameter with an '='.");
          }
        }
       
        currentStringFromFile = "";

      }

      else
      {
        nextCharacter = ::tolower(nextCharacter);
        currentStringFromFile += nextCharacter;
      }
    }

    // Checks that the end of the header has been reached and that all of the required parameters have been set
    if (parameterParseSuccess == true && parametersCurrentlySet == parametersToSet)
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

      floatVffVolumeData->SetScalarType(VTK_FLOAT);
      floatVffVolumeData->SetNumberOfScalarComponents(bands);
      floatVffVolumeData->AllocateScalars();

      // Reads the line feed that comes directly before the image data from the file
      char nextCharacter = readFileStream.get();

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
                float value = 0.0;

                // Applies scaling and offset to the value read from the file
                value = (*valueFromFile)*data_scale+data_offset;

                // Sets the point in the image data to the scaled and offset value
                (*floatPtr) = value;
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
      vffVolumeNode->SetAndObserveImageData(floatVffVolumeData);
      vtkSmartPointer<vtkMRMLScalarVolumeDisplayNode> vffVolumeDisplayNode = vtkSmartPointer<vtkMRMLScalarVolumeDisplayNode>::New();
      this->GetMRMLScene()->AddNode(vffVolumeDisplayNode);

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

