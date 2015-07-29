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
#include "vtkOrientedImageDataResample.h"

// MRML includes
#include "vtkMRMLSegmentationNode.h"

// VTK includes
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
#include <vtksys/Directory.hxx>
#include <vtksys/SystemTools.hxx>

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
static const std::string SERIALIZED_ARRAY_SEPARATOR = " ";
static const std::string SEGMENT_ID = "ID";
static const std::string SEGMENT_NAME = "Name";
static const std::string SEGMENT_DEFAULT_COLOR = "DefaultColor";
static const std::string SEGMENT_TAGS = "Tags";
static const std::string SEGMENT_EXTENT = "Extent";

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
void vtkMRMLSegmentationStorageNode::ResetSupportedWriteFileTypes()
{
  this->InitializeSupportedWriteFileTypes();
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
  segmentation->SetMasterRepresentationName(vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName());

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

  // Get metadata dictionary from image, read common geometry extent
  itk::MetaDataDictionary metadata = allSegmentLabelmapsImage->GetMetaDataDictionary();
  std::string commonExtent;
  itk::ExposeMetaData<std::string>(metadata, SEGMENT_EXTENT.c_str(), commonExtent);
  std::stringstream ssCommonExtent;
  ssCommonExtent << commonExtent;
  int commonGeometryExtent[6] = {0,-1,0,-1,0,-1};
  ssCommonExtent >> commonGeometryExtent[0] >> commonGeometryExtent[1] >> commonGeometryExtent[2] >> commonGeometryExtent[3] >> commonGeometryExtent[4] >> commonGeometryExtent[5];

  // Get image properties
  BinaryLabelmap4DImageType::RegionType itkRegion = allSegmentLabelmapsImage->GetLargestPossibleRegion();
  BinaryLabelmap4DImageType::PointType itkOrigin = allSegmentLabelmapsImage->GetOrigin();
  BinaryLabelmap4DImageType::SpacingType itkSpacing = allSegmentLabelmapsImage->GetSpacing();
  BinaryLabelmap4DImageType::DirectionType itkDirections = allSegmentLabelmapsImage->GetDirection();
  // Make image properties accessible for VTK
  double origin[3] = {itkOrigin[0], itkOrigin[1], itkOrigin[2]};
  double spacing[3] = {itkSpacing[0], itkSpacing[1], itkSpacing[2]};
  double directions[3][3] = {{1.0,0.0,0.0},{0.0,1.0,0.0},{0.0,0.0,1.0}};
  for (unsigned int col=0; col<3; col++)
  {
    for (unsigned int row=0; row<3; row++)
    {
      directions[row][col] = itkDirections[row][col];
    }
  }

  // Read segment binary labelmaps
  for (int segmentIndex = itkRegion.GetIndex()[3]; segmentIndex < itkRegion.GetIndex()[3]+itkRegion.GetSize()[3]; ++segmentIndex)
  {
    // Create segment
    vtkSmartPointer<vtkSegment> currentSegment = vtkSmartPointer<vtkSegment>::New();
    
    // Get metadata for current segment
    std::stringstream ssIdKey;
    ssIdKey << segmentIndex << SEGMENT_ID;
    std::string idKey = ssIdKey.str();
    std::string currentSegmentID;
    itk::ExposeMetaData<std::string>(metadata, idKey.c_str(), currentSegmentID);

    std::stringstream ssNameKey;
    ssNameKey << segmentIndex << SEGMENT_NAME;
    std::string nameKey = ssNameKey.str();
    std::string currentSegmentName;
    itk::ExposeMetaData<std::string>(metadata, nameKey.c_str(), currentSegmentName);
    currentSegment->SetName(currentSegmentName.c_str());

    std::stringstream ssDefaultColorKey;
    ssDefaultColorKey << segmentIndex << SEGMENT_DEFAULT_COLOR;
    std::string defaultColorKey = ssDefaultColorKey.str();
    std::string defaultColorValue;
    itk::ExposeMetaData<std::string>(metadata, defaultColorKey.c_str(), defaultColorValue);
    std::stringstream ssDefaultColorValue;
    ssDefaultColorValue << defaultColorValue;
    double currentSegmentDefaultColor[3] = {0.0,0.0,0.0};
    ssDefaultColorValue >> currentSegmentDefaultColor[0] >> currentSegmentDefaultColor[1] >> currentSegmentDefaultColor[2];
    currentSegment->SetDefaultColor(currentSegmentDefaultColor);

    std::stringstream ssExtentKey;
    ssExtentKey << segmentIndex << SEGMENT_EXTENT;
    std::string extentKey = ssExtentKey.str();
    std::string extentValue;
    itk::ExposeMetaData<std::string>(metadata, extentKey.c_str(), extentValue);
    std::stringstream ssExtentValue;
    ssExtentValue << extentValue;
    int currentSegmentExtent[6] = {0,-1,0,-1,0,-1};
    ssExtentValue >> currentSegmentExtent[0] >> currentSegmentExtent[1] >> currentSegmentExtent[2] >> currentSegmentExtent[3] >> currentSegmentExtent[4] >> currentSegmentExtent[5];

    //TODO: Parse tags with key SEGMENT_TAGS

    // Create binary labelmap volume
    vtkSmartPointer<vtkOrientedImageData> currentBinaryLabelmap = vtkSmartPointer<vtkOrientedImageData>::New();
    currentBinaryLabelmap->SetOrigin(origin);
    currentBinaryLabelmap->SetSpacing(spacing);
    currentBinaryLabelmap->SetDirections(directions);
    currentBinaryLabelmap->SetExtent(currentSegmentExtent);
#if (VTK_MAJOR_VERSION <= 5)
    currentBinaryLabelmap->SetScalarType(VTK_UNSIGNED_CHAR);
    currentBinaryLabelmap->SetNumberOfScalarComponents(1);
    currentBinaryLabelmap->AllocateScalars();
#else
    currentBinaryLabelmap->AllocateScalars(VTK_UNSIGNED_CHAR, 1);
#endif
    unsigned char* labelmapPtr = (unsigned char*)currentBinaryLabelmap->GetScalarPointerForExtent(currentSegmentExtent);
    
    // Define ITK region for current segment
    BinaryLabelmap4DImageType::RegionType segmentRegion;
    BinaryLabelmap4DImageType::SizeType segmentRegionSize;
    BinaryLabelmap4DImageType::IndexType segmentRegionIndex;
    segmentRegionIndex[0] = segmentRegionIndex[1] = segmentRegionIndex[2] = 0;
    segmentRegionIndex[3] = segmentIndex;
    segmentRegionSize = itkRegion.GetSize();
    segmentRegionSize[3] = 1;
    segmentRegion.SetIndex(segmentRegionIndex);
    segmentRegion.SetSize(segmentRegionSize);

    // Iterate through current segment's region and read voxel values into segment labelmap
    BinaryLabelmap4DIteratorType segmentLabelmapIterator(allSegmentLabelmapsImage, segmentRegion);
    for (segmentLabelmapIterator.GoToBegin(); !segmentLabelmapIterator.IsAtEnd(); ++segmentLabelmapIterator)
    {
      BinaryLabelmap4DImageType::IndexType segmentIndex = segmentLabelmapIterator.GetIndex();

      // Skip region outside extent of current segment (consider common extent boundaries)
      if ( segmentIndex[0] + commonGeometryExtent[0] < currentSegmentExtent[0]
        || segmentIndex[0] + commonGeometryExtent[0] > currentSegmentExtent[1]
        || segmentIndex[1] + commonGeometryExtent[2] < currentSegmentExtent[2]
        || segmentIndex[1] + commonGeometryExtent[2] > currentSegmentExtent[3]
        || segmentIndex[2] + commonGeometryExtent[4] < currentSegmentExtent[4]
        || segmentIndex[2] + commonGeometryExtent[4] > currentSegmentExtent[5] )
      {
        continue;
      }

      // Get voxel value
      unsigned char voxelValue = segmentLabelmapIterator.Get();

      // Set voxel value in current segment labelmap
      (*labelmapPtr) = voxelValue;
      ++labelmapPtr;
    }

    // Set loaded binary labelmap to segment
    currentSegment->AddRepresentation(vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName(), currentBinaryLabelmap);

    // Add segment to segmentation
    segmentation->AddSegment(currentSegment, currentSegmentID);
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

  // Create 4D labelmap image and set ITK image properties
  BinaryLabelmap4DImageType::Pointer itkLabelmapImage = BinaryLabelmap4DImageType::New();
  itkLabelmapImage->SetRegions(region);
  itkLabelmapImage->SetOrigin(origin);
  itkLabelmapImage->SetSpacing(spacing);
  itkLabelmapImage->SetDirection(directions);
  itkLabelmapImage->Allocate();

  // Create metadata dictionary, save extent of common geometry image
  itk::MetaDataDictionary metadata;
  int commonGeometryExtent[6] = {0,-1,0,-1,0,-1};
  commonGeometryImage->GetExtent(commonGeometryExtent);
  std::stringstream ssCommonExtent;
  ssCommonExtent << commonGeometryExtent[0] << SERIALIZED_ARRAY_SEPARATOR << commonGeometryExtent[1] << SERIALIZED_ARRAY_SEPARATOR << commonGeometryExtent[2]
    << SERIALIZED_ARRAY_SEPARATOR << commonGeometryExtent[3] << SERIALIZED_ARRAY_SEPARATOR << commonGeometryExtent[4] << SERIALIZED_ARRAY_SEPARATOR << commonGeometryExtent[5];
  std::string commonExtent = ssCommonExtent.str();
  itk::EncapsulateMetaData<std::string>(metadata, SEGMENT_EXTENT.c_str(), commonExtent);

  // Dimensions of the output 4D NRRD file: (i, j, k, segment)
  vtkSegmentation::SegmentMap segmentMap = segmentation->GetSegments();
  unsigned int segmentIndex = 0;
  for (vtkSegmentation::SegmentMap::iterator segmentIt = segmentMap.begin(); segmentIt != segmentMap.end(); ++segmentIt, ++segmentIndex)
  {
    std::string currentSegmentID = segmentIt->first;
    vtkSegment* currentSegment = segmentIt->second.GetPointer();

    // Get segment binary labelmap
    vtkOrientedImageData* currentBinaryLabelmap = vtkOrientedImageData::SafeDownCast(
      currentSegment->GetRepresentation(vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName()) );
    if (!currentBinaryLabelmap)
    {
      continue;
    }

    // Resample current binary labelmap representation to common geometry if necessary
    if (!vtkOrientedImageDataResample::DoGeometriesMatch(commonGeometryImage, currentBinaryLabelmap))
    {
      bool success = vtkOrientedImageDataResample::ResampleOrientedImageToReferenceOrientedImage(
        currentBinaryLabelmap, commonGeometryImage, currentBinaryLabelmap );
      if (!success)
      {
        vtkWarningMacro("WriteBinaryLabelmapRepresentation: Segment " << currentSegmentID << " cannot be resampled to common geometry!");
        continue;
      }
    }

    // Set metadata for current segment
    std::stringstream ssIdKey;
    ssIdKey << segmentIndex << SEGMENT_ID;
    std::string idKey = ssIdKey.str();
    itk::EncapsulateMetaData<std::string>(metadata, idKey.c_str(), currentSegmentID);

    std::stringstream ssNameKey;
    ssNameKey << segmentIndex << SEGMENT_NAME;
    std::string nameKey = ssNameKey.str();
    itk::EncapsulateMetaData<std::string>(metadata, nameKey.c_str(), std::string(currentSegment->GetName()));

    std::stringstream ssDefaultColorKey;
    ssDefaultColorKey << segmentIndex << SEGMENT_DEFAULT_COLOR;
    std::string defaultColorKey = ssDefaultColorKey.str();
    std::stringstream ssDefaultColorValue;
    ssDefaultColorValue << currentSegment->GetDefaultColor()[0] << SERIALIZED_ARRAY_SEPARATOR << currentSegment->GetDefaultColor()[1] << SERIALIZED_ARRAY_SEPARATOR << currentSegment->GetDefaultColor()[2];
    std::string defaultColorValue = ssDefaultColorValue.str();
    itk::EncapsulateMetaData<std::string>(metadata, defaultColorKey.c_str(), defaultColorValue);

    std::stringstream ssExtentKey;
    ssExtentKey << segmentIndex << SEGMENT_EXTENT;
    std::string extentKey = ssExtentKey.str();
    int currentSegmentExtent[6] = {0,-1,0,-1,0,-1};
    currentBinaryLabelmap->GetExtent(currentSegmentExtent);
    std::stringstream ssExtentValue;
    ssExtentValue << currentSegmentExtent[0] << SERIALIZED_ARRAY_SEPARATOR << currentSegmentExtent[1] << SERIALIZED_ARRAY_SEPARATOR << currentSegmentExtent[2]
      << SERIALIZED_ARRAY_SEPARATOR << currentSegmentExtent[3] << SERIALIZED_ARRAY_SEPARATOR << currentSegmentExtent[4] << SERIALIZED_ARRAY_SEPARATOR << currentSegmentExtent[5];
    std::string extentValue = ssExtentValue.str();
    itk::EncapsulateMetaData<std::string>(metadata, extentKey.c_str(), extentValue);

    //TODO: Store tags with key SEGMENT_TAGS

    // Define ITK region for the current segment
    BinaryLabelmap4DImageType::IndexType segmentRegionIndex;
    segmentRegionIndex[0] = segmentRegionIndex[1] = segmentRegionIndex[2] = 0;
    segmentRegionIndex[3] = segmentIndex;
    BinaryLabelmap4DImageType::SizeType segmentRegionSize;
    segmentRegionSize = regionSize;
    segmentRegionSize[3] = 1;
    BinaryLabelmap4DImageType::RegionType segmentRegion;
    segmentRegion.SetIndex(segmentRegionIndex);
    segmentRegion.SetSize(segmentRegionSize);

    // Get scalar pointer for binary labelmap representation. Only a few scalar types are supported
    int currentLabelScalarType = currentBinaryLabelmap->GetScalarType();
    if ( currentLabelScalarType != VTK_UNSIGNED_CHAR
      && currentLabelScalarType != VTK_UNSIGNED_SHORT
      && currentLabelScalarType != VTK_SHORT )
    {
      vtkWarningMacro("WriteBinaryLabelmapRepresentation: Segment " << currentSegmentID << " cannot be written! Binary labelmap scalar type must be unsigned char, unsighed short, or short!");
      continue;
    }
    void* voidScalarPointer = currentBinaryLabelmap->GetScalarPointer();
    unsigned char* labelmapPtrUChar = (unsigned char*)voidScalarPointer;
    unsigned short* labelmapPtrUShort = (unsigned short*)voidScalarPointer;
    short* labelmapPtrShort = (short*)voidScalarPointer;

    // Iterate through current segment labelmap and write voxel values
    BinaryLabelmap4DIteratorType segmentLabelmapIterator(itkLabelmapImage, segmentRegion);
    for (segmentLabelmapIterator.GoToBegin(); !segmentLabelmapIterator.IsAtEnd(); ++segmentLabelmapIterator)
    {
      // Get labelmap value at voxel
      unsigned short label = 0;
      if (currentLabelScalarType == VTK_UNSIGNED_CHAR)
      {
        label = (*labelmapPtrUChar);
      }
      else if (currentLabelScalarType == VTK_UNSIGNED_SHORT)
      {
        label = (*labelmapPtrUShort);
      }
      else if (currentLabelScalarType == VTK_SHORT)
      {
        label = (*labelmapPtrShort);
      }

      // Write voxel value to ITK image
      segmentLabelmapIterator.Set((unsigned char)label);

      ++labelmapPtrUChar;
      ++labelmapPtrUShort;
      ++labelmapPtrShort;
    }
  } // For each segment

  // Set metadata to ITK image
  itkLabelmapImage->SetMetaDataDictionary(metadata);

  // Write image file to disk
  itk::NrrdImageIO::Pointer io = itk::NrrdImageIO::New();
  io->SetFileType(itk::ImageIOBase::Binary); //TODO: This was ASCII originally, change back if binary doesn't work

  typedef itk::ImageFileWriter<BinaryLabelmap4DImageType> WriterType;
  WriterType::Pointer nrrdWriter = WriterType::New();
  nrrdWriter->UseInputMetaDataDictionaryOn();
  nrrdWriter->SetInput(itkLabelmapImage);
  nrrdWriter->SetImageIO(io);
  nrrdWriter->SetFileName(fullName);
  nrrdWriter->UseCompressionOn();
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

*/