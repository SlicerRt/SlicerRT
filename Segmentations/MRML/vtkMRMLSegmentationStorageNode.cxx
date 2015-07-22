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

  This file was originally developed by Adam Rankin and Csaba Pinter, PerkLab, Queen's
  University and was supported through the Applied Cancer Research Unit program of Cancer
  Care Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

// Segmentations includes
#include "vtkMRMLSegmentationStorageNode.h"

#include "vtkSegmentation.h"
#include "vtkOrientedImageData.h"

// MRML includes
#include "vtkMRMLSegmentationNode.h"

// VTK includes
//#include <vtkCallbackCommand.h>
//#include <vtkDataFileFormatHelper.h>
//#include <vtkDataIOManager.h>
//#include <vtkDataSetSurfaceFilter.h>
//#include <vtkITKArchetypeImageSeriesScalarReader.h>
//#include <vtkImageChangeInformation.h>
#include <vtkMRMLScene.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
//#include <vtkPointData.h>
//#include <vtkPolyData.h>
//#include <vtkPolyDataReader.h>
//#include <vtkPolyDataWriter.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkXMLMultiBlockDataWriter.h>
#include <vtkXMLMultiBlockDataReader.h>
#include <vtkStringArray.h>
//#include <vtkTrivialProducer.h>
//#include <vtkUnstructuredGrid.h>
//#include <vtkUnstructuredGridReader.h>
//#include <vtkXMLDataElement.h>
//#include <vtkXMLUtilities.h>
#include <vtksys/Directory.hxx>
#include <vtksys/SystemTools.hxx>

//// VTK ITK includes
//#include "vtkITKImageWriter.h"

// ITK includes
#include <itkImageFileWriter.h>
#include <itkNrrdImageIO.h>
#include <itkExceptionObject.h>
#include <itkImageFileWriter.h>
#include <itkImageFileReader.h>
#include <itkMetaDataDictionary.h>
#include <itkMetaDataObject.h>

// STL & C++ includes
#include <iterator>
#include <sstream>

//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLSegmentationStorageNode);

//----------------------------------------------------------------------------
vtkMRMLSegmentationStorageNode::vtkMRMLSegmentationStorageNode()
{
}

//----------------------------------------------------------------------------
vtkMRMLSegmentationStorageNode::~vtkMRMLSegmentationStorageNode()
{
}

//----------------------------------------------------------------------------
void vtkMRMLSegmentationStorageNode::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkMRMLStorageNode::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkMRMLSegmentationStorageNode::ReadXMLAttributes(const char** atts)
{
  int disabledModify = this->StartModify();

  Superclass::ReadXMLAttributes(atts);

  const char* attName;
  const char* attValue;
  while (*atts != NULL)
  {
    attName = *(atts++);
    attValue = *(atts++);
  }

  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLSegmentationStorageNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);
  vtkIndent indent(nIndent);
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, StorageID
void vtkMRMLSegmentationStorageNode::Copy(vtkMRMLNode *anode)
{
  int disabledModify = this->StartModify();

  Superclass::Copy(anode);
  vtkMRMLSegmentationStorageNode *node = (vtkMRMLSegmentationStorageNode *) anode;

  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLSegmentationStorageNode::InitializeSupportedReadFileTypes()
{
  this->SupportedReadFileTypes->InsertNextValue("Segmentation 4D NRRD volume (.seg.nrrd)");
  this->SupportedReadFileTypes->InsertNextValue("Segmentation Multi-block dataset (.seg.vtm)");
}

//----------------------------------------------------------------------------
void vtkMRMLSegmentationStorageNode::InitializeSupportedWriteFileTypes()
{
  Superclass::InitializeSupportedWriteFileTypes();

  vtkMRMLSegmentationNode* segmentationNode = this->GetAssociatedDataNode();
  if (segmentationNode)
  {
    const char* masterRepresentation = segmentationNode->GetSegmentation()->GetMasterRepresentationName();
    if (!strcmp(masterRepresentation, vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName()))
    {
      // Binary labelmap -> 4D NRRD volume
      this->SupportedWriteFileTypes->InsertNextValue("Segmentation 4D NRRD volume (.seg.nrrd)");
    }
    else if ( !strcmp(masterRepresentation, vtkSegmentationConverter::GetSegmentationClosedSurfaceRepresentationName())
           || !strcmp(masterRepresentation, vtkSegmentationConverter::GetSegmentationPlanarContourRepresentationName()) )
    {
      // Closed surface or planar contours -> MultiBlock polydata
      this->SupportedWriteFileTypes->InsertNextValue("Segmentation Multi-block dataset (.seg.vtm)");
    }
  }
}

//----------------------------------------------------------------------------
vtkMRMLSegmentationNode* vtkMRMLSegmentationStorageNode::GetAssociatedDataNode()
{
  if (!this->GetScene())
  {
    return NULL;
  }

  std::vector<vtkMRMLNode*> segmentationNodes;
  unsigned int numberOfNodes = this->GetScene()->GetNodesByClass("vtkMRMLSegmentationNode", segmentationNodes);
  for (unsigned int nodeIndex=0; nodeIndex<numberOfNodes; nodeIndex++)
  {
    vtkMRMLSegmentationNode* node = vtkMRMLSegmentationNode::SafeDownCast(segmentationNodes[nodeIndex]);
    if (node)
    {
      const char* storageNodeID = node->GetStorageNodeID();
      if (storageNodeID && !strcmp(storageNodeID, this->ID))
      {
        return vtkMRMLSegmentationNode::SafeDownCast(node);
      }
    }
  }

  return NULL;
}

//----------------------------------------------------------------------------
const char* vtkMRMLSegmentationStorageNode::GetDefaultWriteFileExtension()
{
  vtkMRMLSegmentationNode* segmentationNode = this->GetAssociatedDataNode();
  if (segmentationNode)
  {
    const char* masterRepresentation = segmentationNode->GetSegmentation()->GetMasterRepresentationName();
    if (!strcmp(masterRepresentation, vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName()))
    {
      // Binary labelmap -> 4D NRRD volume
      return "seg.nrrd";
    }
    else if ( !strcmp(masterRepresentation, vtkSegmentationConverter::GetSegmentationClosedSurfaceRepresentationName())
           || !strcmp(masterRepresentation, vtkSegmentationConverter::GetSegmentationPlanarContourRepresentationName()) )
    {
      // Closed surface or planar contours -> MultiBlock polydata
      return "seg.vtm";
    }
  }

  // Master representation is not supported for writing to file
  return NULL;
}

//----------------------------------------------------------------------------
bool vtkMRMLSegmentationStorageNode::CanReadInReferenceNode(vtkMRMLNode *refNode)
{
  return refNode->IsA("vtkMRMLSegmentationNode");
}


//TODO: Useful snippets
//std::string extension = vtkMRMLStorageNode::GetLowercaseExtensionFromFileName(baseFilename);
//std::string fullNameWithoutExtension = vtksys::SystemTools::GetFilenameWithoutExtension(baseFilename);

//----------------------------------------------------------------------------
int vtkMRMLSegmentationStorageNode::ReadDataInternal(vtkMRMLNode *refNode)
{
  vtkMRMLSegmentationNode* segmentationNode = vtkMRMLSegmentationNode::SafeDownCast(refNode);
  if (!segmentationNode)
  {
    vtkErrorMacro("ReadDataInternal: Reference node is not a segmentation node");
    return 0;
  }

  std::string fullName = this->GetFullNameFromFileName();
  if (fullName == std::string(""))
  {
    vtkErrorMacro("ReadDataInternal: File name not specified");
    return 0;
  }

  // Check that the file exists
  if (vtksys::SystemTools::FileExists(fullName.c_str()) == false)
  {
    vtkErrorMacro("ReadDataInternal: segmentation file '" << fullName.c_str() << "' not found.");
    return 0;
  }

  // Try to read as labelmap first then as poly data
  if (this->ReadBinaryLabelmapRepresentation(segmentationNode->GetSegmentation(), fullName))
  {
    return 1;
  }
  else if (this->ReadPolyDataRepresentation(segmentationNode->GetSegmentation(), fullName))
  {
    return 1;
  }

  // Failed to read
  vtkErrorMacro("ReadDataInternal: File " << fullName << " could not be read neither as labelmap nor poly data");
  return 0;
}

//----------------------------------------------------------------------------
int vtkMRMLSegmentationStorageNode::ReadBinaryLabelmapRepresentation(vtkSegmentation* segmentation, std::string path)
{
  if (!vtksys::SystemTools::FileExists(path.c_str()))
  {
    vtkErrorMacro("ReadBinaryLabelmapRepresentation: Input file " << path << " does not exist!");
    return 0;
  }

  // Set up output segmentation
  if (!segmentation || segmentation->GetNumberOfSegments() > 0)
  {
    vtkErrorMacro("ReadBinaryLabelmapRepresentation: Output segmentation must exist and must be empty!");
    return 0;
  }

  // Read 4D NRRD image file
  typedef itk::ImageFileReader<BinaryLabelmap4DImageType> FileReaderType;
  FileReaderType::Pointer reader = FileReaderType::New();
  reader->SetFileName(path);
  try
  {
    reader->Update();
  }
  catch (itk::ImageFileReaderException &error)
  {
    vtkErrorMacro("ReadBinaryLabelmapRepresentation: Failed to load file " << path << " as segmentation. Exception:\n" << error);
    return 0;
  }
  BinaryLabelmap4DImageType::Pointer allSegmentLabelmapsImage = reader->GetOutput();

  // Get image properties
  BinaryLabelmap4DImageType::RegionType region = allSegmentLabelmapsImage->GetLargestPossibleRegion();
  BinaryLabelmap4DImageType::PointType origin = allSegmentLabelmapsImage->GetOrigin();
  BinaryLabelmap4DImageType::SpacingType spacing = allSegmentLabelmapsImage->GetSpacing();
  BinaryLabelmap4DImageType::DirectionType directions = allSegmentLabelmapsImage->GetDirection();

  // Read segment binary labelmaps
  vtkSegmentation::SegmentMap segmentMap = segmentation->GetSegments();
  for (int segmentIndex = region.GetIndex()[3]; segmentIndex < region.GetIndex()[3]+region.GetSize()[3]; ++segmentIndex)
  {
    BinaryLabelmap4DImageType::RegionType segmentRegion;
    BinaryLabelmap4DImageType::SizeType segmentRegionSize;
    BinaryLabelmap4DImageType::IndexType segmentRegionIndex;
    segmentRegionIndex[0] = segmentRegionIndex[1] = segmentRegionIndex[2] = 0;
    segmentRegionIndex[3] = segmentIndex;
    segmentRegionSize = region.GetSize();
    segmentRegionSize[3] = 1;
    segmentRegion.SetIndex(segmentRegionIndex);
    segmentRegion.SetSize(segmentRegionSize);

    // Iterate through current segment's region and read voxel values into segment labelmap
    BinaryLabelmap4DIteratorType segmentLabelmapIterator(allSegmentLabelmapsImage, segmentRegion);
    for (segmentLabelmapIterator.GoToBegin(); !segmentLabelmapIterator.IsAtEnd(); ++segmentLabelmapIterator)
    {
      unsigned char voxelValue = segmentLabelmapIterator.Get();
      //TODO: Use values. This is for testing
      int i=0; ++i;
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkMRMLSegmentationStorageNode::ReadPolyDataRepresentation(vtkSegmentation* segmentation, std::string path)
{
  if (!vtksys::SystemTools::FileExists(path.c_str()))
  {
    vtkErrorMacro("ReadPolyDataRepresentation: Input file " << path << " does not exist!");
    return 0;
  }

  // Set up output segmentation
  if (!segmentation || segmentation->GetNumberOfSegments() > 0)
  {
    vtkErrorMacro("ReadPolyDataRepresentation: Output segmentation must exist and must be empty!");
    return 0;
  }

  // Read multiblock dataset from disk
  vtkSmartPointer<vtkXMLMultiBlockDataReader> reader = vtkSmartPointer<vtkXMLMultiBlockDataReader>::New();
  reader->SetFileName(path.c_str());
  reader->Update();
  vtkMultiBlockDataSet* multiBlockDataset = vtkMultiBlockDataSet::SafeDownCast(reader->GetOutput());
  if (!multiBlockDataset)
  {
    vtkErrorMacro("ReadPolyDataRepresentation: Failed to read file " << path);
    return 0;
  }

  // Read segment poly datas
  for (int blockIndex=0; blockIndex<multiBlockDataset->GetNumberOfBlocks(); ++blockIndex)
  {
    //TODO: This is for testing
    int i=0; ++i;

    // Determine if poly data is closed surface or planar contours based on whether it contains cells or not
    //TODO:
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkMRMLSegmentationStorageNode::WriteDataInternal(vtkMRMLNode *refNode)
{
  std::string fullName = this->GetFullNameFromFileName();
  if (fullName == std::string(""))
  {
    vtkErrorMacro("vtkMRMLModelNode: File name not specified");
    return 0;
  }

  vtkMRMLSegmentationNode *segmentationNode = vtkMRMLSegmentationNode::SafeDownCast(refNode);
  if (segmentationNode == NULL)
  {
    vtkErrorMacro("Segmentation node expected. Unable to write node to file.");
    return 0;
  }

  // Write only master representation
  //TODO: Change file extension based on master representation type (by default it will be nrrd)
  vtkSegmentation* segmentation = segmentationNode->GetSegmentation();
  const char* masterRepresentation = segmentation->GetMasterRepresentationName();
  if (!strcmp(masterRepresentation, vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName()))
  {
    // Binary labelmap -> 4D NRRD volume
    return this->WriteBinaryLabelmapRepresentation(segmentation, fullName);
  }
  else if ( !strcmp(masterRepresentation, vtkSegmentationConverter::GetSegmentationClosedSurfaceRepresentationName())
         || !strcmp(masterRepresentation, vtkSegmentationConverter::GetSegmentationPlanarContourRepresentationName()) )
  {
    // Closed surface or planar contours -> MultiBlock polydata
    return this->WritePolyDataRepresentation(segmentation, fullName);
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkMRMLSegmentationStorageNode::WriteBinaryLabelmapRepresentation(vtkSegmentation* segmentation, std::string fullName)
{
  if (!segmentation || segmentation->GetNumberOfSegments() == 0)
  {
    vtkErrorMacro("WriteBinaryLabelmapRepresentation: Invalid segmentation to write to disk");
    return 0;
  }

  // Get and check master representation
  const char* masterRepresentation = segmentation->GetMasterRepresentationName();
  if (!masterRepresentation || strcmp(masterRepresentation, vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName()))
  {
    vtkErrorMacro("WriteBinaryLabelmapRepresentation: Invalid master representation to write as image data");
    return 0;
  }

  // Determine merged labelmap dimensions and properties
  std::string commonGeometryString = segmentation->DetermineCommonLabelmapGeometry();
  vtkSmartPointer<vtkOrientedImageData> commonGeometryImage = vtkSmartPointer<vtkOrientedImageData>::New();
  vtkSegmentationConverter::DeserializeImageGeometry(commonGeometryString, commonGeometryImage);
  int* commonGeometryDimensions = commonGeometryImage->GetDimensions();
  int dimensions[4] = {commonGeometryDimensions[0],commonGeometryDimensions[1],commonGeometryDimensions[2],segmentation->GetNumberOfSegments()};
  double* commonGeometryOrigin = commonGeometryImage->GetOrigin();
  double originArray[4] = {commonGeometryOrigin[0],commonGeometryOrigin[1],commonGeometryOrigin[2],0.0};
  double* commonGeometrySpacing = commonGeometryImage->GetSpacing();
  double spacingArray[4] = {commonGeometrySpacing[0],commonGeometrySpacing[1],commonGeometrySpacing[2],1.0};
  double directionsArray[3][3] = {{1.0,0.0,0.0},{0.0,1.0,0.0},{0.0,0.0,1.0}};
  commonGeometryImage->GetDirections(directionsArray);

  // Determine ITK image properties
  BinaryLabelmap4DImageType::SizeType regionSize;
  BinaryLabelmap4DImageType::IndexType regionIndex;
  BinaryLabelmap4DImageType::RegionType region;
  BinaryLabelmap4DImageType::PointType origin;
  BinaryLabelmap4DImageType::SpacingType spacing;
  BinaryLabelmap4DImageType::DirectionType directions;
  for (int dim = 0; dim < 4; dim++)
  {
    regionIndex[dim] = 0;
    regionSize[dim] = dimensions[dim];
    spacing[dim] = spacingArray[dim];
    origin[dim] = originArray[dim];
  }
  region.SetSize(regionSize);
  region.SetIndex(regionIndex);
  // Normalize direction vectors
  for (unsigned int col=0; col<3; col++)
  {
    double len = 0;
    unsigned int row = 0;
    for (row=0; row<3; row++)
    {
      len += directionsArray[row][col] * directionsArray[row][col];
    }
    if (len == 0.0)
    {
      len = 1.0;
    }
    len = sqrt(len);
    for (row=0; row<3; row++)
    {
      directions[row][col] = directionsArray[row][col]/len;
    }
  }
  // Add fourth dimension to directions matrix
  directions[3][3] = 1.0;
  for (unsigned int index=0; index<3; index++)
  {
    directions[3][index] = 0.0;
    directions[index][3] = 0.0;
  }

  // Create merged labelmap image and set ITK image properties
  BinaryLabelmap4DImageType::Pointer mergedLabelmapImage = BinaryLabelmap4DImageType::New();
  mergedLabelmapImage->SetRegions(region);
  mergedLabelmapImage->SetOrigin(origin);
  mergedLabelmapImage->SetSpacing(spacing);
  mergedLabelmapImage->SetDirection(directions);
  mergedLabelmapImage->Allocate();

  // Dimensions of the output 4D NRRD file: (i, j, k, segment)
  vtkSegmentation::SegmentMap segmentMap = segmentation->GetSegments();
  unsigned int segmentIndex = 0;
  for (vtkSegmentation::SegmentMap::iterator segmentIt = segmentMap.begin(); segmentIt != segmentMap.end(); ++segmentIt, ++segmentIndex)
  {
    BinaryLabelmap4DImageType::RegionType segmentRegion;
    BinaryLabelmap4DImageType::SizeType segmentRegionSize;
    BinaryLabelmap4DImageType::IndexType segmentRegionIndex;
    segmentRegionIndex[0] = segmentRegionIndex[1] = segmentRegionIndex[2] = 0;
    segmentRegionIndex[3] = segmentIndex;
    segmentRegionSize = regionSize;
    segmentRegionSize[3] = 1;
    segmentRegion.SetIndex(segmentRegionIndex);
    segmentRegion.SetSize(segmentRegionSize);

    // Iterate through current segment labelmap and write voxel values
    BinaryLabelmap4DIteratorType segmentLabelmapIterator(mergedLabelmapImage, segmentRegion);
    for (segmentLabelmapIterator.GoToBegin(); !segmentLabelmapIterator.IsAtEnd(); ++segmentLabelmapIterator)
    {
      segmentLabelmapIterator.Set((unsigned char)segmentIndex);
      //TODO: Write actual values. This is for testing
    }

    // TODO: Set metadata
  }

  // Save segment IDs and members, also display colors in the NRRD header
  //TODO: Use MetaDataDictionary

  // Write image file to disk
  itk::NrrdImageIO::Pointer io = itk::NrrdImageIO::New();
  io->SetFileType(itk::ImageIOBase::Binary); //TODO: This was ASCII originally, change back if binary doesn't work

  typedef itk::ImageFileWriter<BinaryLabelmap4DImageType> WriterType;
  WriterType::Pointer nrrdWriter = WriterType::New();
  nrrdWriter->UseInputMetaDataDictionaryOn();
  nrrdWriter->SetInput(mergedLabelmapImage);
  nrrdWriter->SetImageIO(io);
  nrrdWriter->SetFileName(fullName);
  try
  {
    nrrdWriter->Update();
  }
  catch (itk::ExceptionObject e)
  {
    vtkErrorMacro("Failed to write segmentation to file " << fullName);
    return 0;
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkMRMLSegmentationStorageNode::WritePolyDataRepresentation(vtkSegmentation* segmentation, std::string path)
{
  if (!segmentation || segmentation->GetNumberOfSegments() == 0)
  {
    vtkErrorMacro("WritePolyDataRepresentation: Invalid segmentation to write to disk");
    return 0;
  }

  // Get and check master representation
  const char* masterRepresentation = segmentation->GetMasterRepresentationName();
  if ( !masterRepresentation
    || ( strcmp(masterRepresentation, vtkSegmentationConverter::GetSegmentationClosedSurfaceRepresentationName())
      && strcmp(masterRepresentation, vtkSegmentationConverter::GetSegmentationPlanarContourRepresentationName()) ) )
  {
    vtkErrorMacro("WritePolyDataRepresentation: Invalid master representation to write as poly data");
    return 0;
  }

  // Initialize dataset to write
  vtkSmartPointer<vtkMultiBlockDataSet> multiBlockDataset = vtkSmartPointer<vtkMultiBlockDataSet>::New();
  multiBlockDataset->SetNumberOfBlocks(segmentation->GetNumberOfSegments());

  // Add segment poly datas to dataset
  vtkSegmentation::SegmentMap segmentMap = segmentation->GetSegments();
  unsigned int segmentIndex = 0;
  for (vtkSegmentation::SegmentMap::iterator segmentIt = segmentMap.begin(); segmentIt != segmentMap.end(); ++segmentIt, ++segmentIndex)
  {
    vtkDataObject* polyDataRepresentation = segmentIt->second->GetRepresentation(masterRepresentation);
    multiBlockDataset->SetBlock(segmentIndex, polyDataRepresentation);

    // TODO: Set metadata
  }

  // Write multiblock dataset to disk
  vtkSmartPointer<vtkXMLMultiBlockDataWriter> writer = vtkSmartPointer<vtkXMLMultiBlockDataWriter>::New();
  writer->SetInputData(multiBlockDataset);
  writer->SetFileName(path.c_str());
  writer->SetDataModeToBinary();
  writer->Write();

  return 1;
}






/* TODO:
//----------------------------------------------------------------------------
bool vtkMRMLSegmentationStorageNode::ReadPolyDataInternal( vtkPolyData* outModel, const char* filename, const char* suffix )
{
  if( outModel == NULL )
  {
    return false;
  }
  vtkSmartPointer<vtkPolyDataReader> reader = vtkSmartPointer<vtkPolyDataReader>::New();
  reader->SetFileName(filename);
  vtkSmartPointer<vtkUnstructuredGridReader> unstructuredGridReader = vtkSmartPointer<vtkUnstructuredGridReader>::New();
  vtkSmartPointer<vtkDataSetSurfaceFilter> surfaceFilter = vtkSmartPointer<vtkDataSetSurfaceFilter>::New();

  unstructuredGridReader->SetFileName(filename);
  if (reader->IsFilePolyData())
  {
    reader->Update();
    outModel->DeepCopy(reader->GetOutput());
  }
  else if (unstructuredGridReader->IsFileUnstructuredGrid())
  {
    unstructuredGridReader->Update();
#if (VTK_MAJOR_VERSION <= 5)
    surfaceFilter->SetInput(unstructuredGridReader->GetOutput());
#else
    surfaceFilter->SetInputData(unstructuredGridReader->GetOutput());
#endif
    surfaceFilter->Update();
    outModel->DeepCopy(surfaceFilter->GetOutput());
  }
  else
  {
    vtkErrorMacro("File " << filename
      << " is not recognized as polydata nor as an unstructured grid.");
  }
  if (reader->GetOutput() == NULL)
  {
    vtkErrorMacro("Unable to read file " << filename);
    return false;
  }

  return true;
}

//----------------------------------------------------------------------------
int vtkMRMLSegmentationStorageNode::WriteOrientedImageDataInternal(vtkOrientedImageData* imageData)
{
  int result(0);
  if ( !segment->HasImageData() )
  {
    vtkErrorMacro("cannot write ImageData, it's NULL");
    return result;
  }

  // update the file list
  std::string moveFromDir = this->UpdateFileList(segment, IJKToRASMatrix, 1);

  std::string fullName(this->GetFileName());

  bool moveSucceeded = true;
  if (moveFromDir != std::string(""))
  {
    // the temp writing went okay, just move the files from there to where
    // they're supposed to go. It will fail if the temp dir is on a different
    // device, so fall back to a second write in that case.
    std::string targetDir = vtksys::SystemTools::GetParentDirectory(fullName.c_str());
    vtkDebugMacro("WriteData: moving files from temp dir " << moveFromDir << " to target dir " << targetDir);

    vtksys::Directory dir;
    dir.Load(moveFromDir.c_str());
    vtkDebugMacro("WriteData: tempdir " << moveFromDir.c_str() << " has " << dir.GetNumberOfFiles() << " in it");
    size_t fileNum;
    std::vector<std::string> targetPathComponents;
    vtksys::SystemTools::SplitPath(targetDir.c_str(), targetPathComponents);
    std::vector<std::string> sourcePathComponents;
    vtksys::SystemTools::SplitPath(moveFromDir.c_str(), sourcePathComponents);
    for (fileNum = 0; fileNum <  dir.GetNumberOfFiles(); ++fileNum)
    {
      const char *thisFile = dir.GetFile(static_cast<unsigned long>(fileNum));
      // skip the dirs
      if (strcmp(thisFile,".") &&
        strcmp(thisFile,".."))
      {
        targetPathComponents.push_back(thisFile);
        sourcePathComponents.push_back(thisFile);
        std::string targetFile = vtksys::SystemTools::JoinPath(targetPathComponents);
        // does the target file already exist?
        if (vtksys::SystemTools::FileExists(targetFile.c_str(), true))
        {
          // remove it
          vtkWarningMacro("WriteData: removing old version of file " << targetFile);
          if (!vtksys::SystemTools::RemoveFile(targetFile.c_str()))
          {
            vtkErrorMacro("WriteData: unable to remove old version of file " << targetFile);
          }
        }
        std::string sourceFile = vtksys::SystemTools::JoinPath(sourcePathComponents);
        vtkDebugMacro("WriteData: moving file number " << fileNum << ", " << sourceFile << " to " << targetFile);
        // thisFile needs a full path it's bare
        int renameReturn = std::rename(sourceFile.c_str(), targetFile.c_str());
        if (renameReturn != 0 )
        {
          perror( "Error renaming file" );
          vtkErrorMacro( "WriteData: Error renaming file to " << targetFile << ", renameReturn = " << renameReturn );
          // fall back to doing a second write
          moveSucceeded = false;
          break;
        }
        targetPathComponents.pop_back();
        sourcePathComponents.pop_back();
      }
    }
    // delete the temporary dir and all remaining contents
    bool dirRemoved = vtksys::SystemTools::RemoveADirectory(moveFromDir.c_str());
    if (!dirRemoved)
    {
      vtkWarningMacro("Failed to remove temporary write directory " << moveFromDir);
    }

  }
  else
  {
    // didn't move it
    moveSucceeded = false;
  }

  if (!moveSucceeded)
  {
    vtkDebugMacro("WriteData: writing out file with archetype " << fullName);

    vtkNew<vtkITKImageWriter> writer;
    writer->SetFileName(fullName.c_str());

#if (VTK_MAJOR_VERSION <= 5)
    writer->SetInput( segment->GetImageData() );
#else
    writer->SetInputConnection( segment->GetImageDataConnection() );
#endif
    writer->SetUseCompression(this->GetUseCompression());
    if (this->GetScene() &&
      this->GetScene()->GetDataIOManager() &&
      this->GetScene()->GetDataIOManager()->GetFileFormatHelper())
    {
      writer->SetImageIOClassName(this->GetScene()->GetDataIOManager()->GetFileFormatHelper()->
        GetClassNameFromFormatString("NRRD (.nrrd)")
        );
    }

    // set volume attributes
    writer->SetRasToIJKMatrix(IJKToRASMatrix);

    try
    {
      writer->Write();
    }
    catch (...)
    {
      result = 0;
    }
  }

  return result;
}

//----------------------------------------------------------------------------
bool vtkMRMLSegmentationStorageNode::ReadOrientedImageDataInternal(vtkOrientedImageData* imageData, itk::MetaDataDictionary& outDictionary)
{
  std::string filename = this->GetFullNameFromFileName();

  vtkSmartPointer<vtkITKArchetypeImageSeriesReader> reader = vtkSmartPointer<vtkITKArchetypeImageSeriesScalarReader>::New();
  reader->SetSingleFile(1);
  reader->SetUseOrientationFromFile(1);

  if (reader.GetPointer() == NULL)
  {
    vtkErrorMacro("ReadData: Failed to instantiate a file reader");
    return false;
  }

  reader->AddObserver( vtkCommand::ProgressEvent, this->MRMLCallbackCommand);

  if ( segment->HasImageData() )
  {
    segment->SetImageDataConnection(NULL);
  }

  // Set the list of file names on the reader
  reader->ResetFileNames();
  reader->SetArchetype(filename.c_str());

  // Workaround
  ApplyImageSeriesReaderWorkaround(this, reader, filename);

  // Center image
  reader->SetOutputScalarTypeToNative();
  reader->SetDesiredCoordinateOrientationToNative();
    reader->SetUseNativeOriginOn();
  //}

  try
  {
    vtkDebugMacro("ReadData: right before reader update, reader num files = " << reader->GetNumberOfFileNames());
    reader->Update();
  }
  catch (...)
  {
    std::string reader0thFileName;
    if (reader->GetFileName(0) != NULL)
    {
      reader0thFileName = std::string("reader 0th file name = ") + std::string(reader->GetFileName(0));
    }
    vtkErrorMacro("ReadData: Cannot read file as a volume of type "
      << (segment ? segment->GetUid() : "null")
      << "[" << "fullName = " << filename << "]\n"
      << "\tNumber of files listed in the node = "
      << this->GetNumberOfFileNames() << ".\n"
      << "\tFile reader says it was able to read "
      << reader->GetNumberOfFileNames() << " files.\n"
      << "\tFile reader used the archetype file name of " << reader->GetArchetype()
      << " [" << reader0thFileName.c_str() << "]\n");
    return false;
  }

  if (reader->GetOutput() == NULL || reader->GetOutput()->GetPointData() == NULL)
  {
    vtkErrorMacro("ReadData: Unable to read data from file: " << filename);
    return false;
  }

  vtkPointData * pointData = reader->GetOutput()->GetPointData();
  if (pointData->GetScalars() == NULL || pointData->GetScalars()->GetNumberOfTuples() == 0)
  {
    vtkErrorMacro("ReadData: Unable to read ScalarVolume data from file: " << filename );
    return false;
  }

  if ( reader->GetNumberOfComponents() != 1 )
  {
    vtkErrorMacro("ReadData: Not a scalar volume file: " << filename );
    return false;
  }

  // Set volume attributes
  outDictionary = reader->GetMetaDataDictionary();

  // Get all the file names from the reader
  if (reader->GetNumberOfFileNames() > 1)
  {
    vtkDebugMacro("Number of file names = " << reader->GetNumberOfFileNames()
      << ", number of slice location = " << reader->GetNumberOfSliceLocation());
    if (this->FileNameList.size() == 0)
    {
      // It is safe to assume that the file names in reader are unique.
      // Here we shortcut the n*log(n) unique insertion of  AddFileName().
      this->FileNameList = reader->GetFileNames();
    }
    else
    {
      // include the archetype, file 0, in the storage node's file list
      for (unsigned int n = 0; n < reader->GetNumberOfFileNames(); n++)
      {
        const char *thisFileName = reader->GetFileName(n);
#ifndef NDEBUG
        int currentSize =
#endif
          this->AddFileName(thisFileName);
        vtkDebugMacro("After adding file " << n << ", filename = " << thisFileName
          << " to this storage node's list, current size of the list = " << currentSize);
      }
    }
  }

  vtkNew<vtkImageChangeInformation> ici;
#if (VTK_MAJOR_VERSION <= 5)
  ici->SetInput(reader->GetOutput());
#else
  ici->SetInputData(reader->GetOutput());
#endif
  ici->SetOutputSpacing( 1, 1, 1 );
  ici->SetOutputOrigin( 0, 0, 0 );
  ici->Update();

  if (ici->GetOutput() == NULL)
  {
    vtkErrorMacro("vtkMRMLVolumeArchetypeStorageNode: Cannot read file: " << filename);
    return false;
  }

  vtkNew<vtkImageData> iciOutputCopy;
  iciOutputCopy->ShallowCopy(ici->GetOutput());
  vtkSmartPointer<vtkTrivialProducer> tp = vtkSmartPointer<vtkTrivialProducer>::New();
  tp->SetInputDataObject(iciOutputCopy.GetPointer());
  segment->SetImageDataConnection(tp->GetOutputPort());

  vtkMatrix4x4* mat = reader->GetRasToIjkMatrix();
  if ( mat == NULL )
  {
    vtkErrorMacro ("Reader returned NULL RasToIjkMatrix");
  }
  OutIJKToRASMatrix.DeepCopy(mat);

  return true;
}

//----------------------------------------------------------------------------
int vtkMRMLSegmentationStorageNode::WriteSegmentInternal(vtkXMLDataElement* segmentElement, vtkSegment* segment, const std::string& path, vtkMatrix4x4* IJKToRASMatrix)
{
  if( segment->HasImageData() )
  {
    // Back up filename, functions below mess it all up
    std::string origFilename(this->GetFileName());
    this->SetFileName(std::string(path + std::string("/") + std::string(segmentElement->GetAttribute("VoxelDataFilename"))).c_str());
    if( this->WriteImageDataInternal(segment, IJKToRASMatrix) != 1 )
    {
      vtkErrorMacro("Unable to write image data for segment " << segment->GetUid() <<". Aborting.");
      return 0;
    }
    this->SetFileName(origFilename.c_str());
  }

  if( segment->HasPolyData() )
  {
    std::string filename = path + std::string("/") + std::string(segmentElement->GetAttribute("ModelFilename"));
    if( this->WriteModelDataInternal(segment->GetPolyData(), filename) != 1 )
    {
      vtkErrorMacro("Unable to write poly data for segment " << segment->GetUid() <<". Aborting.");
      return 0;
    }
  }

  if( segment->HasRtRoiPoints() )
  {
    std::string filename = path + std::string("/") + std::string(segmentElement->GetAttribute("RoiPointsFilename"));
    if( this->WriteModelDataInternal(segment->GetRtRoiPoints(), filename) != 1 )
    {
      vtkWarningMacro("Unable to write poly data for segment " << segment->GetUid() <<".");
    }
  }

  return 1;
}
*/