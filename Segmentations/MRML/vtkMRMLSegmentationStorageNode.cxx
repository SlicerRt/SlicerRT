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

This file was originally developed by Adam Rankin, PerkLab, Queen's University
and was supported through the Applied Cancer Research Unit program of Cancer Care
Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

// Segmentations
#include "vtkMRMLSegmentationStorageNode.h"

// MRML includes
#include "vtkMRMLSegmentationNode.h"

// VTK includes
//#include <vtkCallbackCommand.h>
//#include <vtkDataFileFormatHelper.h>
//#include <vtkDataIOManager.h>
//#include <vtkDataSetSurfaceFilter.h>
//#include <vtkITKArchetypeImageSeriesScalarReader.h>
//#include <vtkImageChangeInformation.h>
//#include <vtkMRMLScene.h>
//#include <vtkMatrix4x4.h>
//#include <vtkNew.h>
//#include <vtkObjectFactory.h>
//#include <vtkPointData.h>
//#include <vtkPolyData.h>
//#include <vtkPolyDataReader.h>
//#include <vtkPolyDataWriter.h>
//#include <vtkStringArray.h>
//#include <vtkTrivialProducer.h>
//#include <vtkUnstructuredGrid.h>
//#include <vtkUnstructuredGridReader.h>
//#include <vtkXMLDataElement.h>
//#include <vtkXMLUtilities.h>
//#include <vtksys/Directory.hxx>
//#include <vtksys/SystemTools.hxx>

//// VTK ITK includes
#include "vtkITKImageWriter.h"

// STL & C++ includes
#include <iterator>
#include <sstream>

//----------------------------------------------------------------------------
//namespace
//{
//
//  //----------------------------------------------------------------------------
//  void ApplyImageSeriesReaderWorkaround(vtkMRMLSegmentationStorageNode * storageNode,
//    vtkITKArchetypeImageSeriesReader * reader,
//    const std::string& filename)
//  {
//    // TODO: this is a workaround for an issue in itk::ImageSeriesReader
//    // where is assumes that all the filenames that have been passed
//    // to it are a dimension smaller than the image it is asked to create
//    // (i.e. a list of .jpg files that form a volume).
//    // In our case though, we can have file lists that include both the
//    // header and bulk data, like .hdr/.img pairs.  So we need to
//    // be careful not to send extra filenames to the reader if the
//    // format is multi-file for the same volume
//    //
//    // check for Analyze and similar format- if the archetype is
//    // one of those, then don't send the rest of the list
//    //
//    std::string fileExt=vtkMRMLStorageNode::GetLowercaseExtensionFromFileName(filename);
//    if ( fileExt != std::string(".hdr")
//      && fileExt != std::string(".img")
//      && fileExt != std::string(".mhd")
//      && fileExt != std::string(".nhdr") )
//    {
//      for (int n = 0; n < storageNode->GetNumberOfFileNames(); n++)
//      {
//        std::string nthFileName = storageNode->GetFullNameFromNthFileName(n);
//        vtkDebugWithObjectMacro(storageNode,
//          "ReadData: got full name for " << n << "th file: " << nthFileName
//          << ", adding it to reader, current num files on it = "
//          << reader->GetNumberOfFileNames());
//        reader->AddFileName(nthFileName.c_str());
//      }
//    }
//  }
//} // end of anonymous namespace

//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLSegmentationStorageNode);

//----------------------------------------------------------------------------
vtkMRMLSegmentationStorageNode::vtkMRMLSegmentationStorageNode()
: CenterImage(0)
, SingleFile(1)
, UseOrientationFromFile(1)
{
  //this->InitializeSupportedWriteFileTypes();
  //this->InitializeSupportedReadFileTypes();
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
bool vtkMRMLSegmentationStorageNode::CanReadInReferenceNode(vtkMRMLNode *refNode)
{
  return refNode->IsA("vtkMRMLSegmentationNode");
}

//----------------------------------------------------------------------------
// TODO : when source representation concept exists, then store the existing converted representations as xml attributes in either the segmentation node or segmentation storage node
int vtkMRMLSegmentationStorageNode::ReadDataInternal(vtkMRMLNode *refNode)
{
  //vtkMRMLSegmentationNode *segmentationNode = dynamic_cast <vtkMRMLSegmentationNode *> (refNode);

  //std::string fullName = this->GetFullNameFromFileName();
  //if (fullName == std::string(""))
  //{
  //  vtkErrorMacro("ReadDataInternal: File name not specified");
  //  return 0;
  //}

  //// check that the file exists
  //if (vtksys::SystemTools::FileExists(fullName.c_str()) == false)
  //{
  //  vtkErrorMacro("ReadDataInternal: segmentation file '" << fullName.c_str() << "' not found.");
  //  return 0;
  //}

  //std::string path = vtksys::SystemTools::GetFilenamePath(fullName);
  //vtkSmartPointer<vtkXMLDataElement> element = vtkSmartPointer<vtkXMLDataElement>::Take(vtkXMLUtilities::ReadElementFromFile(fullName.c_str()));

  //vtkXMLDataElement* segmentList = element->GetNestedElement(0);
  //if(STRCASECMP(segmentList->GetName(), "Segments") != 0 )
  //{
  //  vtkErrorMacro("Incorrect XML structure in file: " << fullName << ". No Segments Tag under " << element->GetName());
  //  return 0;
  //}
  //itk::MetaDataDictionary dictionary;
  //vtkSmartPointer<vtkMatrix4x4> IJKToRASMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  //// The dictionary and ijk to ras matrix are duplicated in every segment's image data (.nrrd) file
  //// This loops over all possible segments and simply overwrites the data every time
  //// essentially, the matrix and meta data dictionary are pulled from the last segment's .nrrd file
  //for( int segmentIndex = 0; segmentIndex < segmentList->GetNumberOfNestedElements(); ++segmentIndex)
  //{
  //  vtkXMLDataElement* segment = element->GetNestedElement(segmentIndex);

  //  vtkSegment* segObj = this->ReadSegmentInternal(path, segment, dictionary, *IJKToRASMatrix);
  //  if( segObj == NULL )
  //  {
  //    vtkErrorMacro("Unable to create segment with index: " << segmentIndex << " and name: " << segment->GetName() );
  //    continue;
  //  }
  //  segmentationNode->AddAndObserveSegment(segObj);
  //}
  //segmentationNode->SetIJKToRASMatrix(IJKToRASMatrix);
  //segmentationNode->SetMetaDataDictionary(dictionary);

  return 1;
}

//----------------------------------------------------------------------------
vtkSegment* vtkMRMLSegmentationStorageNode::ReadSegmentInternal(const std::string& path, vtkXMLDataElement* segmentElement, itk::MetaDataDictionary& OutDictionary, vtkMatrix4x4& OutIJKToRASMatrix)
{
  //vtkSmartPointer<vtkSegment> segment = vtkSmartPointer<vtkSegment>::New();

  //if( segmentElement->GetAttribute("Uid") == NULL || STRCASECMP(segmentElement->GetAttribute("Uid"),"") == 0 )
  //{
  //  vtkErrorMacro("No UID stored for segment. Unable to load segment.");
  //  return NULL;
  //}
  //else
  //{
  //  segment->SetUid(segmentElement->GetAttribute("Uid"));
  //}

  //try
  //{
  //  std::string voxelFile = path + std::string("/") + std::string(segmentElement->GetAttribute("VoxelDataFilename"));
  //  // Give the labelmap a chance to load if it exists
  //  if( vtksys::SystemTools::FileExists(voxelFile.c_str()) )
  //  {
  //    std::string origFilename(this->GetFileName());
  //    this->SetFileName(voxelFile.c_str());
  //    this->ReadImageDataInternal(segment, OutDictionary, OutIJKToRASMatrix);
  //    this->SetFileName(origFilename.c_str());
  //  }

  //  std::string polyDataFile = path + std::string("/") + std::string(segmentElement->GetAttribute("ModelFilename"));
  //  if( vtksys::SystemTools::FileExists(polyDataFile.c_str()) )
  //  {
  //    // Load the poly data!
  //    vtkSmartPointer<vtkPolyData> model = vtkSmartPointer<vtkPolyData>::New();
  //    if( this->ReadModelDataInternal(model, polyDataFile.c_str(), SlicerRtCommon::SEGMENTATION_POLY_DATA_POSTFIX.c_str()) )
  //    {
  //      vtkSmartPointer<vtkTrivialProducer> tp = vtkSmartPointer<vtkTrivialProducer>::New();
  //      tp->SetInputDataObject(model);
  //      segment->SetPolyDataConnection(tp->GetOutputPort());
  //    }
  //    else
  //    {
  //      vtkErrorMacro("Unable to read poly data for segmentation.");
  //    }
  //  }

  //  std::string roiPointsFile = path + std::string("/") + std::string(segmentElement->GetAttribute("RoiPointsFilename"));
  //  if( vtksys::SystemTools::FileExists(roiPointsFile.c_str()) )
  //  {
  //    // Load the RTSS planar ROI points
  //    vtkSmartPointer<vtkPolyData> model = vtkSmartPointer<vtkPolyData>::New();
  //    if( this->ReadModelDataInternal(model, roiPointsFile.c_str(), SlicerRtCommon::SEGMENTATION_ROI_POINTS_POSTFIX.c_str()) )
  //    {
  //      vtkSmartPointer<vtkTrivialProducer> tp = vtkSmartPointer<vtkTrivialProducer>::New();
  //      tp->SetInputDataObject(model);
  //      segment->SetRtRoiPointsConnection(tp->GetOutputPort());
  //    }
  //    else
  //    {
  //      vtkErrorMacro("Unable to read model data for rt roi model.");
  //    }
  //  }
  //}
  //catch(...)
  //{
  //  segment->Delete();
  //  return NULL;
  //}

  //return segment;
  //TODO:
  return NULL;
}

//----------------------------------------------------------------------------
// TODO : save only source representation - this implies the source representation mechanism exists
int vtkMRMLSegmentationStorageNode::WriteDataInternal(vtkMRMLNode *refNode)
{
  //vtkMRMLSegmentationNode *segmentationNode = vtkMRMLSegmentationNode::SafeDownCast(refNode);

  //std::string fullName = this->GetFullNameFromFileName();
  //if (fullName == std::string(""))
  //{
  //  vtkErrorMacro("vtkMRMLModelNode: File name not specified");
  //  return 0;
  //}

  //std::string path = vtksys::SystemTools::GetFilenamePath(fullName);

  //vtkSmartPointer<vtkXMLDataElement> element = vtkSmartPointer<vtkXMLDataElement>::Take(this->CreateXMLElement(*segmentationNode, fullName));

  //vtkXMLDataElement* segmentList = element->GetNestedElement(0);
  //// Sanity check
  //if( segmentList->GetNumberOfNestedElements() != segmentationNode->GetNumberOfSegments() )
  //{
  //  vtkErrorMacro("XML segment list and segmentation node segment list do not match. Unable to write node to file.");
  //  return 0;
  //}

  //vtkSmartPointer<vtkMatrix4x4> IJKToRASMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  //segmentationNode->GetIJKToRASMatrix(IJKToRASMatrix);

  //for( int segmentIndex = 0; segmentIndex < segmentationNode->GetNumberOfSegments(); ++segmentIndex)
  //{
  //  vtkSegment* segment = segmentationNode->GetSegmentByIndex(segmentIndex);
  //  vtkXMLDataElement* segmentElement = segmentList->GetNestedElement(segmentIndex);

  //  this->WriteSegmentInternal(segmentElement, segment, path, IJKToRASMatrix);
  //}

  //vtkXMLUtilities::WriteElementToFile(element, fullName.c_str());

  return 1;
}

//----------------------------------------------------------------------------
void vtkMRMLSegmentationStorageNode::InitializeSupportedReadFileTypes()
{
  //this->SupportedReadFileTypes->InsertNextValue("Segmentation (.seg)");
}

//----------------------------------------------------------------------------
void vtkMRMLSegmentationStorageNode::InitializeSupportedWriteFileTypes()
{
  //this->SupportedWriteFileTypes->InsertNextValue("Segmentation (.seg)");
}

//----------------------------------------------------------------------------
const char* vtkMRMLSegmentationStorageNode::GetDefaultWriteFileExtension()
{
  //TODO:
  return "seg";
}

//----------------------------------------------------------------------------
int vtkMRMLSegmentationStorageNode::WriteModelDataInternal( vtkPolyData* polyData, std::string& filename )
{
//  // Write out the ribbon model data if it exists
//  vtkNew<vtkPolyDataWriter> writer;
//  writer->SetFileName(filename.c_str());
//  writer->SetFileType(this->GetUseCompression() ? VTK_BINARY : VTK_ASCII );
//#if (VTK_MAJOR_VERSION <= 5)
//  writer->SetInput( polyData );
//#else
//  writer->SetInputData( polyData );
//#endif
//  try
//  {
//    writer->Write();
//  }
//  catch (...)
//  {
//    return 0;
//  }

  return 1;
}

//----------------------------------------------------------------------------
bool vtkMRMLSegmentationStorageNode::ReadModelDataInternal( vtkPolyData* outModel, const char* filename, const char* suffix )
{
//  if( outModel == NULL )
//  {
//    return false;
//  }
//  vtkSmartPointer<vtkPolyDataReader> reader = vtkSmartPointer<vtkPolyDataReader>::New();
//  reader->SetFileName(filename);
//  vtkSmartPointer<vtkUnstructuredGridReader> unstructuredGridReader = vtkSmartPointer<vtkUnstructuredGridReader>::New();
//  vtkSmartPointer<vtkDataSetSurfaceFilter> surfaceFilter = vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
//
//  unstructuredGridReader->SetFileName(filename);
//  if (reader->IsFilePolyData())
//  {
//    reader->Update();
//    outModel->DeepCopy(reader->GetOutput());
//  }
//  else if (unstructuredGridReader->IsFileUnstructuredGrid())
//  {
//    unstructuredGridReader->Update();
//#if (VTK_MAJOR_VERSION <= 5)
//    surfaceFilter->SetInput(unstructuredGridReader->GetOutput());
//#else
//    surfaceFilter->SetInputData(unstructuredGridReader->GetOutput());
//#endif
//    surfaceFilter->Update();
//    outModel->DeepCopy(surfaceFilter->GetOutput());
//  }
//  else
//  {
//    vtkErrorMacro("File " << filename
//      << " is not recognized as polydata nor as an unstructured grid.");
//  }
//  if (reader->GetOutput() == NULL)
//  {
//    vtkErrorMacro("Unable to read file " << filename);
//    return false;
//  }

  return true;
}

//----------------------------------------------------------------------------
vtkXMLDataElement* vtkMRMLSegmentationStorageNode::CreateXMLElement( vtkMRMLSegmentationNode& node, const std::string& baseFilename )
{
  //std::string extension = vtkMRMLStorageNode::GetLowercaseExtensionFromFileName(baseFilename);
  //std::string fullNameWithoutExtension = vtksys::SystemTools::GetFilenameWithoutExtension(baseFilename);

  //vtkSmartPointer<vtkXMLDataElement> element = vtkSmartPointer<vtkXMLDataElement>::New();
  //element->SetName("SegmentationNode");

  //vtkSmartPointer<vtkXMLDataElement> segmentList = vtkSmartPointer<vtkXMLDataElement>::New();
  //segmentList->SetName("SegmentList");
  //element->AddNestedElement(segmentList);

  //for( int segmentIndex = 0; segmentIndex < node.GetNumberOfSegments(); ++segmentIndex )
  //{
  //  vtkSegment* segment = node.GetSegmentByIndex(segmentIndex);
  //  vtkSmartPointer<vtkXMLDataElement> segmentElement = vtkSmartPointer<vtkXMLDataElement>::New();
  //  segmentElement->SetName("Segment");
  //  segmentElement->SetAttribute("SegmentID", segment->GetUid().c_str());

  //  {
  //    std::stringstream ss;
  //    ss << fullNameWithoutExtension << segment->GetUid() << SlicerRtCommon::SEGMENTATION_INDEXED_LABELMAP_POSTFIX << SlicerRtCommon::VOXEL_FILE_TYPE;
  //    segmentElement->SetAttribute("VoxelDataFilename", ss.str().c_str());
  //  }
  //  {
  //    std::stringstream ss;
  //    ss << fullNameWithoutExtension << segment->GetUid() << SlicerRtCommon::SEGMENTATION_POLY_DATA_POSTFIX << SlicerRtCommon::MODEL_FILE_TYPE;
  //    segmentElement->SetAttribute("ModelFilename", ss.str().c_str());
  //  }
  //  {
  //    std::stringstream ss;
  //    ss << fullNameWithoutExtension << segment->GetUid() << SlicerRtCommon::SEGMENTATION_ROI_POINTS_POSTFIX << SlicerRtCommon::MODEL_FILE_TYPE;
  //    segmentElement->SetAttribute("RoiPointsFilename", ss.str().c_str());
  //  }

  //  segmentList->AddNestedElement(segmentElement);
  //}

  //return element;
  //TODO:
  return NULL;
}

//----------------------------------------------------------------------------
int vtkMRMLSegmentationStorageNode::WriteImageDataInternal( vtkSegment* segment, vtkMatrix4x4* IJKToRASMatrix )
{
//  int result(0);
//  if ( !segment->HasImageData() )
//  {
//    vtkErrorMacro("cannot write ImageData, it's NULL");
//    return result;
//  }
//
//  // update the file list
//  std::string moveFromDir = this->UpdateFileList(segment, IJKToRASMatrix, 1);
//
//  std::string fullName(this->GetFileName());
//
//  bool moveSucceeded = true;
//  if (moveFromDir != std::string(""))
//  {
//    // the temp writing went okay, just move the files from there to where
//    // they're supposed to go. It will fail if the temp dir is on a different
//    // device, so fall back to a second write in that case.
//    std::string targetDir = vtksys::SystemTools::GetParentDirectory(fullName.c_str());
//    vtkDebugMacro("WriteData: moving files from temp dir " << moveFromDir << " to target dir " << targetDir);
//
//    vtksys::Directory dir;
//    dir.Load(moveFromDir.c_str());
//    vtkDebugMacro("WriteData: tempdir " << moveFromDir.c_str() << " has " << dir.GetNumberOfFiles() << " in it");
//    size_t fileNum;
//    std::vector<std::string> targetPathComponents;
//    vtksys::SystemTools::SplitPath(targetDir.c_str(), targetPathComponents);
//    std::vector<std::string> sourcePathComponents;
//    vtksys::SystemTools::SplitPath(moveFromDir.c_str(), sourcePathComponents);
//    for (fileNum = 0; fileNum <  dir.GetNumberOfFiles(); ++fileNum)
//    {
//      const char *thisFile = dir.GetFile(static_cast<unsigned long>(fileNum));
//      // skip the dirs
//      if (strcmp(thisFile,".") &&
//        strcmp(thisFile,".."))
//      {
//        targetPathComponents.push_back(thisFile);
//        sourcePathComponents.push_back(thisFile);
//        std::string targetFile = vtksys::SystemTools::JoinPath(targetPathComponents);
//        // does the target file already exist?
//        if (vtksys::SystemTools::FileExists(targetFile.c_str(), true))
//        {
//          // remove it
//          vtkWarningMacro("WriteData: removing old version of file " << targetFile);
//          if (!vtksys::SystemTools::RemoveFile(targetFile.c_str()))
//          {
//            vtkErrorMacro("WriteData: unable to remove old version of file " << targetFile);
//          }
//        }
//        std::string sourceFile = vtksys::SystemTools::JoinPath(sourcePathComponents);
//        vtkDebugMacro("WriteData: moving file number " << fileNum << ", " << sourceFile << " to " << targetFile);
//        // thisFile needs a full path it's bare
//        int renameReturn = std::rename(sourceFile.c_str(), targetFile.c_str());
//        if (renameReturn != 0 )
//        {
//          perror( "Error renaming file" );
//          vtkErrorMacro( "WriteData: Error renaming file to " << targetFile << ", renameReturn = " << renameReturn );
//          // fall back to doing a second write
//          moveSucceeded = false;
//          break;
//        }
//        targetPathComponents.pop_back();
//        sourcePathComponents.pop_back();
//      }
//    }
//    // delete the temporary dir and all remaining contents
//    bool dirRemoved = vtksys::SystemTools::RemoveADirectory(moveFromDir.c_str());
//    if (!dirRemoved)
//    {
//      vtkWarningMacro("Failed to remove temporary write directory " << moveFromDir);
//    }
//
//  }
//  else
//  {
//    // didn't move it
//    moveSucceeded = false;
//  }
//
//  if (!moveSucceeded)
//  {
//    vtkDebugMacro("WriteData: writing out file with archetype " << fullName);
//
//    vtkNew<vtkITKImageWriter> writer;
//    writer->SetFileName(fullName.c_str());
//
//#if (VTK_MAJOR_VERSION <= 5)
//    writer->SetInput( segment->GetImageData() );
//#else
//    writer->SetInputConnection( segment->GetImageDataConnection() );
//#endif
//    writer->SetUseCompression(this->GetUseCompression());
//    if (this->GetScene() &&
//      this->GetScene()->GetDataIOManager() &&
//      this->GetScene()->GetDataIOManager()->GetFileFormatHelper())
//    {
//      writer->SetImageIOClassName(this->GetScene()->GetDataIOManager()->GetFileFormatHelper()->
//        GetClassNameFromFormatString("NRRD (.nrrd)")
//        );
//    }
//
//    // set volume attributes
//    writer->SetRasToIJKMatrix(IJKToRASMatrix);
//
//    try
//    {
//      writer->Write();
//    }
//    catch (...)
//    {
//      result = 0;
//    }
//  }
//
//  return result;
  //TODO:
  return 0;
}

//----------------------------------------------------------------------------
std::string vtkMRMLSegmentationStorageNode::UpdateFileList(vtkSegment* segment, vtkMatrix4x4* IJKToRASMatrix, int move)
{
//  bool result = true;
//  std::string returnString = "";
//
//  if ( segment == NULL || !segment->HasImageData() )
//  {
//    vtkErrorMacro("UpdateFileList: cannot write ImageData, it's NULL");
//    return returnString;
//  }
//
//  std::string oldName = std::string(this->GetFileName());
//  if (oldName == std::string(""))
//  {
//    vtkErrorMacro("UpdateFileList: File name not specified");
//    return returnString;
//  }
//
//  vtkDebugMacro("UpdateFileList: old file name = " << oldName);
//
//  // clear out the old file list
//  this->ResetFileNameList();
//
//  // get the original directory
//  std::string originalDir = vtksys::SystemTools::GetParentDirectory(oldName.c_str());
//  std::vector<std::string> pathComponents;
//  vtksys::SystemTools::SplitPath(originalDir.c_str(), pathComponents);
//  // add a temp dir to it
//  pathComponents.push_back(std::string("TempWrite") +
//    vtksys::SystemTools::GetFilenameWithoutExtension(oldName));
//  std::string tempDir = vtksys::SystemTools::JoinPath(pathComponents);
//  vtkDebugMacro("UpdateFileList: deleting and then re-creating temp dir "<< tempDir.c_str());
//  if (vtksys::SystemTools::FileExists(tempDir.c_str()))
//  {
//    result = vtksys::SystemTools::RemoveADirectory(tempDir.c_str());
//  }
//  if (!result)
//  {
//    vtkErrorMacro("UpdateFileList: Failed to delete directory '" << tempDir << "'.");
//    return returnString;
//  }
//  result = vtksys::SystemTools::MakeDirectory(tempDir.c_str());
//  if (!result)
//  {
//    vtkErrorMacro("UpdateFileList: Failed to create directory" << tempDir);
//    return returnString;
//  }
//  // make a new name,
//  pathComponents.push_back(vtksys::SystemTools::GetFilenameName(oldName));
//  std::string tempName = vtksys::SystemTools::JoinPath(pathComponents);
//  vtkDebugMacro("UpdateFileList: new archetype file name = " << tempName.c_str());
//
//  // set up the writer and write
//  vtkNew<vtkITKImageWriter> writer;
//  writer->SetFileName(tempName.c_str());
//
//#if (VTK_MAJOR_VERSION <= 5)
//  writer->SetInput( segment->GetImageData() );
//#else
//  writer->SetInputConnection( segment->GetImageDataConnection() );
//#endif
//  writer->SetUseCompression(this->GetUseCompression());
//  if (this->GetScene() &&
//    this->GetScene()->GetDataIOManager() &&
//    this->GetScene()->GetDataIOManager()->GetFileFormatHelper())
//  {
//    writer->SetImageIOClassName(this->GetScene()->GetDataIOManager()->GetFileFormatHelper()->
//      GetClassNameFromFormatString("NRRD (.nrrd)")
//      );
//  }
//
//  // set volume attributes
//  writer->SetRasToIJKMatrix(IJKToRASMatrix);
//
//  try
//  {
//    writer->Write();
//  }
//  catch (itk::ExceptionObject& exception)
//  {
//    exception.Print(std::cerr);
//    result = false;
//  }
//
//  if (!result)
//  {
//    vtkErrorMacro("UpdateFileList: Failed to write '" << tempName.c_str()
//      << "'.");
//    return returnString;
//  }
//
//  // look through the new dir and populate the file list
//  vtksys::Directory dir;
//  result = dir.Load(tempDir.c_str());
//  vtkDebugMacro("UpdateFileList: tempdir " << tempDir.c_str() << " has " << dir.GetNumberOfFiles() << " in it");
//  if (!result)
//  {
//    vtkErrorMacro("UpdateFileList: Failed to open directory '" << tempDir.c_str() << "'.");
//    return returnString;
//  }
//
//  // take the archetype and temp dir off of the path
//  pathComponents.pop_back();
//  pathComponents.pop_back();
//  std::string localDirectory = vtksys::SystemTools::JoinPath(pathComponents);
//  std::string relativePath;
//
//  if (this->IsFilePathRelative(localDirectory.c_str()))
//  {
//    vtkDebugMacro("UpdateFileList: the local directory is already relative, use it " << localDirectory);
//    relativePath = localDirectory;
//  }
//  else
//  {
//    if (this->GetScene() != NULL &&
//      strcmp(this->GetScene()->GetRootDirectory(), "") != 0)
//    {
//      // use the scene's root dir, all the files in the list will be
//      // relative to it (the relative path is how you go from the root dir to
//      // the dir in which the volume is saved)
//      std::string rootDir = this->GetScene()->GetRootDirectory();
//      if (rootDir.length() != 0 &&
//        rootDir.find_last_of("/") == rootDir.length() - 1)
//      {
//        vtkDebugMacro("UpdateFileList: found trailing slash in : " << rootDir);
//        rootDir = rootDir.substr(0, rootDir.length()-1);
//      }
//      vtkDebugMacro("UpdateFileList: got the scene root dir " << rootDir << ", local dir = " << localDirectory.c_str());
//      // RelativePath requires two absolute paths, otherwise returns empty
//      // string
//      if (this->IsFilePathRelative(rootDir.c_str()))
//      {
//        vtkDebugMacro("UpdateFileList: have a relative directory in root dir (" << rootDir << "), using the local dir as a relative path.");
//        // assume the relative local directory is relative to the root
//        // directory
//        relativePath = localDirectory;
//      }
//      else
//      {
//        relativePath = vtksys::SystemTools::RelativePath(rootDir.c_str(), localDirectory.c_str());
//      }
//    }
//    else
//    {
//      // use the archetype's directory, so that all the files in the list will
//      // be relative to it
//      if (this->IsFilePathRelative(originalDir.c_str()))
//      {
//        relativePath = localDirectory;
//      }
//      else
//      {
//        // the RelativePath method needs two absolute paths
//        relativePath = vtksys::SystemTools::RelativePath(originalDir.c_str(), localDirectory.c_str());
//      }
//      vtkDebugMacro("UpdateFileList: no scene root dir, using original dir = " << originalDir.c_str() << " and local dir " << localDirectory.c_str());
//    }
//  }
//  // strip off any trailing slashes
//  if (relativePath.length() != 0 &&
//    relativePath.find_last_of("/")  != std::string::npos &&
//    relativePath.find_last_of("/") == relativePath.length() - 1)
//  {
//    vtkDebugMacro("UpdateFileList: stripping off a trailing slash from relativePath '"<< relativePath.c_str() << "'");
//    relativePath = relativePath.substr(0, relativePath.length() - 1);
//  }
//  vtkDebugMacro("UpdateFileList: using prefix of relative path '" << relativePath.c_str() << "'");
//  // now get ready to join the relative path to thisFile
//  std::vector<std::string> relativePathComponents;
//  vtksys::SystemTools::SplitPath(relativePath.c_str(), relativePathComponents);
//
//  // make sure that the archetype is added first! AddFile when it gets to it
//  // in the dir will not add a duplicate
//  std::string newArchetype = vtksys::SystemTools::GetFilenameName(tempName.c_str());
//  vtkDebugMacro("Stripped archetype = " << newArchetype.c_str());
//  relativePathComponents.push_back(newArchetype);
//  std::string relativeArchetypeFile =  vtksys::SystemTools::JoinPath(relativePathComponents);
//  vtkDebugMacro("Relative archetype = " << relativeArchetypeFile.c_str());
//  relativePathComponents.pop_back();
//  this->AddFileName(relativeArchetypeFile.c_str());
//
//  bool addedArchetype = false;
//  // now iterate through the directory files
//  for (size_t fileNum = 0; fileNum < dir.GetNumberOfFiles(); ++fileNum)
//  {
//    // skip the dirs
//    const char *thisFile = dir.GetFile(static_cast<unsigned long>(fileNum));
//    if (strcmp(thisFile,".") &&
//      strcmp(thisFile,".."))
//    {
//      vtkDebugMacro("UpdateFileList: adding file number " << fileNum << ", " << thisFile);
//      if (newArchetype.compare(thisFile) == 0)
//      {
//        addedArchetype = true;
//      }
//      // at this point, the file name is bare of a directory, turn it into a
//      // relative path from the original archetype
//      relativePathComponents.push_back(thisFile);
//      std::string relativeFile =  vtksys::SystemTools::JoinPath(relativePathComponents);
//      relativePathComponents.pop_back();
//      vtkDebugMacro("UpdateFileList: " << fileNum << ", using relative file name " << relativeFile.c_str());
//      this->AddFileName(relativeFile.c_str());
//    }
//  }
//  result = addedArchetype;
//  if (!result)
//  {
//    std::stringstream addedFiles;
//    std::copy(++this->FileNameList.begin(), this->FileNameList.end(),
//      std::ostream_iterator<std::string>(addedFiles,", "));
//    vtkErrorMacro("UpdateFileList: the archetype file '"
//      << newArchetype.c_str() << "' wasn't written out when writting '"
//      << tempName.c_str() << "' in '" << tempDir.c_str() << "'. "
//      << "Only those " << dir.GetNumberOfFiles() - 2
//      << " file(s) have been written: " << addedFiles.str().c_str() <<". "
//      << "Old name is '" << oldName.c_str() << "'."
//      );
//    return returnString;
//  }
//  // restore the old file name
//  vtkDebugMacro("UpdateFileList: resetting file name to " << oldName.c_str());
//  this->SetFileName(oldName.c_str());
//
//  if (move != 1)
//  {
//    // clean up temp directory
//    vtkDebugMacro("UpdateFileList: removing temp dir " << tempDir);
//    result = vtksys::SystemTools::RemoveADirectory(tempDir.c_str());
//    if (!result)
//    {
//      vtkErrorMacro("UpdateFileList: failed to remove temp dir '"
//        << tempDir.c_str() << "'." );
//      return returnString;
//    }
//    return std::string("");
//  }
//  else
//  {
//    vtkDebugMacro("UpdateFileList: returning temp dir " << tempDir);
//    return tempDir;
//  }
  //TODO:
  return "";
}

//----------------------------------------------------------------------------
bool vtkMRMLSegmentationStorageNode::ReadImageDataInternal( vtkSegment* segment, itk::MetaDataDictionary& OutDictionary, vtkMatrix4x4& OutIJKToRASMatrix )
{
  //
  // vtkMRMLVolumeNode
  //   |
  //   |--vtkMRMLScalarVolumeNode
  //         |
  //         |----vtkMRMLDiffusionWeightedVolumeNode
  //         |
  //         |----vtkMRMLTensorVolumeNode
  //                  |
  //                  |---vtkMRMLDiffusionImageVolumeNode
  //                  |       |
  //                  |       |---vtkMRMLDiffusionTensorVolumeNode
  //                  |
  //                  |---vtkMRMLVectorVolumeNode
  //

//  std::string filename = this->GetFullNameFromFileName();
//
//  vtkSmartPointer<vtkITKArchetypeImageSeriesReader> reader = vtkSmartPointer<vtkITKArchetypeImageSeriesScalarReader>::New();
//  reader->SetSingleFile( this->GetSingleFile() );
//  reader->SetUseOrientationFromFile( this->GetUseOrientationFromFile() );
//
//  if (reader.GetPointer() == NULL)
//  {
//    vtkErrorMacro("ReadData: Failed to instantiate a file reader");
//    return false;
//  }
//
//  reader->AddObserver( vtkCommand::ProgressEvent, this->MRMLCallbackCommand);
//
//  if ( segment->HasImageData() )
//  {
//    segment->SetImageDataConnection(NULL);
//  }
//
//  // Set the list of file names on the reader
//  reader->ResetFileNames();
//  reader->SetArchetype(filename.c_str());
//
//  // Workaround
//  ApplyImageSeriesReaderWorkaround(this, reader, filename);
//
//  // Center image
//  reader->SetOutputScalarTypeToNative();
//  reader->SetDesiredCoordinateOrientationToNative();
//  if (this->CenterImage)
//  {
//    reader->SetUseNativeOriginOff();
//  }
//  else
//  {
//    reader->SetUseNativeOriginOn();
//  }
//
//  try
//  {
//    vtkDebugMacro("ReadData: right before reader update, reader num files = " << reader->GetNumberOfFileNames());
//    reader->Update();
//  }
//  catch (...)
//  {
//    std::string reader0thFileName;
//    if (reader->GetFileName(0) != NULL)
//    {
//      reader0thFileName = std::string("reader 0th file name = ") + std::string(reader->GetFileName(0));
//    }
//    vtkErrorMacro("ReadData: Cannot read file as a volume of type "
//      << (segment ? segment->GetUid() : "null")
//      << "[" << "fullName = " << filename << "]\n"
//      << "\tNumber of files listed in the node = "
//      << this->GetNumberOfFileNames() << ".\n"
//      << "\tFile reader says it was able to read "
//      << reader->GetNumberOfFileNames() << " files.\n"
//      << "\tFile reader used the archetype file name of " << reader->GetArchetype()
//      << " [" << reader0thFileName.c_str() << "]\n");
//    return false;
//  }
//
//  if (reader->GetOutput() == NULL || reader->GetOutput()->GetPointData() == NULL)
//  {
//    vtkErrorMacro("ReadData: Unable to read data from file: " << filename);
//    return false;
//  }
//
//  vtkPointData * pointData = reader->GetOutput()->GetPointData();
//  if (pointData->GetScalars() == NULL || pointData->GetScalars()->GetNumberOfTuples() == 0)
//  {
//    vtkErrorMacro("ReadData: Unable to read ScalarVolume data from file: " << filename );
//    return false;
//  }
//
//  if ( reader->GetNumberOfComponents() != 1 )
//  {
//    vtkErrorMacro("ReadData: Not a scalar volume file: " << filename );
//    return false;
//  }
//
//  // Set volume attributes
//  OutDictionary = reader->GetMetaDataDictionary();
//
//  // Get all the file names from the reader
//  if (reader->GetNumberOfFileNames() > 1)
//  {
//    vtkDebugMacro("Number of file names = " << reader->GetNumberOfFileNames()
//      << ", number of slice location = " << reader->GetNumberOfSliceLocation());
//    if (this->FileNameList.size() == 0)
//    {
//      // It is safe to assume that the file names in reader are unique.
//      // Here we shortcut the n*log(n) unique insertion of  AddFileName().
//      this->FileNameList = reader->GetFileNames();
//    }
//    else
//    {
//      // include the archetype, file 0, in the storage node's file list
//      for (unsigned int n = 0; n < reader->GetNumberOfFileNames(); n++)
//      {
//        const char *thisFileName = reader->GetFileName(n);
//#ifndef NDEBUG
//        int currentSize =
//#endif
//          this->AddFileName(thisFileName);
//        vtkDebugMacro("After adding file " << n << ", filename = " << thisFileName
//          << " to this storage node's list, current size of the list = " << currentSize);
//      }
//    }
//  }
//
//  vtkNew<vtkImageChangeInformation> ici;
//#if (VTK_MAJOR_VERSION <= 5)
//  ici->SetInput(reader->GetOutput());
//#else
//  ici->SetInputData(reader->GetOutput());
//#endif
//  ici->SetOutputSpacing( 1, 1, 1 );
//  ici->SetOutputOrigin( 0, 0, 0 );
//  ici->Update();
//
//  if (ici->GetOutput() == NULL)
//  {
//    vtkErrorMacro("vtkMRMLVolumeArchetypeStorageNode: Cannot read file: " << filename);
//    return false;
//  }
//
//  vtkNew<vtkImageData> iciOutputCopy;
//  iciOutputCopy->ShallowCopy(ici->GetOutput());
//  vtkSmartPointer<vtkTrivialProducer> tp = vtkSmartPointer<vtkTrivialProducer>::New();
//  tp->SetInputDataObject(iciOutputCopy.GetPointer());
//  segment->SetImageDataConnection(tp->GetOutputPort());
//
//  vtkMatrix4x4* mat = reader->GetRasToIjkMatrix();
//  if ( mat == NULL )
//  {
//    vtkErrorMacro ("Reader returned NULL RasToIjkMatrix");
//  }
//  OutIJKToRASMatrix.DeepCopy(mat);
//
  return true;
}

//----------------------------------------------------------------------------
void vtkMRMLSegmentationStorageNode::ReadXMLAttributes(const char** atts)
{
  //int disabledModify = this->StartModify();

  //Superclass::ReadXMLAttributes(atts);

  //const char* attName;
  //const char* attValue;
  //while (*atts != NULL)
  //{
  //  attName = *(atts++);
  //  attValue = *(atts++);
  //  if (!strcmp(attName, "centerImage"))
  //  {
  //    std::stringstream ss;
  //    ss << attValue;
  //    ss >> this->CenterImage;
  //  }
  //  if (!strcmp(attName, "singleFile"))
  //  {
  //    std::stringstream ss;
  //    ss << attValue;
  //    ss >> this->SingleFile;
  //  }
  //  if (!strcmp(attName, "UseOrientationFromFile"))
  //  {
  //    std::stringstream ss;
  //    ss << attValue;
  //    ss >> this->UseOrientationFromFile;
  //  }
  //}

  //this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLSegmentationStorageNode::WriteXML(ostream& of, int nIndent)
{
  //Superclass::WriteXML(of, nIndent);
  //vtkIndent indent(nIndent);
  //{
  //  std::stringstream ss;
  //  ss << this->CenterImage;
  //  of << indent << " centerImage=\"" << ss.str() << "\"";
  //}
  //{
  //  std::stringstream ss;
  //  ss << this->SingleFile;
  //  of << indent << " singleFile=\"" << ss.str() << "\"";
  //}
  //{
  //  std::stringstream ss;
  //  ss << this->UseOrientationFromFile;
  //  of << indent << " UseOrientationFromFile=\"" << ss.str() << "\"";
  //}
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, StorageID
void vtkMRMLSegmentationStorageNode::Copy(vtkMRMLNode *anode)
{
  //int disabledModify = this->StartModify();

  //Superclass::Copy(anode);
  //vtkMRMLSegmentationStorageNode *node = (vtkMRMLSegmentationStorageNode *) anode;

  //this->SetCenterImage(node->CenterImage);
  //this->SetSingleFile(node->SingleFile);
  //this->SetUseOrientationFromFile(node->UseOrientationFromFile);

  //this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
int vtkMRMLSegmentationStorageNode::WriteSegmentInternal(vtkXMLDataElement* segmentElement, vtkSegment* segment, const std::string& path, vtkMatrix4x4* IJKToRASMatrix)
{
  //if( segment->HasImageData() )
  //{
  //  // Back up filename, functions below mess it all up
  //  std::string origFilename(this->GetFileName());
  //  this->SetFileName(std::string(path + std::string("/") + std::string(segmentElement->GetAttribute("VoxelDataFilename"))).c_str());
  //  if( this->WriteImageDataInternal(segment, IJKToRASMatrix) != 1 )
  //  {
  //    vtkErrorMacro("Unable to write image data for segment " << segment->GetUid() <<". Aborting.");
  //    return 0;
  //  }
  //  this->SetFileName(origFilename.c_str());
  //}

  //if( segment->HasPolyData() )
  //{
  //  std::string filename = path + std::string("/") + std::string(segmentElement->GetAttribute("ModelFilename"));
  //  if( this->WriteModelDataInternal(segment->GetPolyData(), filename) != 1 )
  //  {
  //    vtkErrorMacro("Unable to write poly data for segment " << segment->GetUid() <<". Aborting.");
  //    return 0;
  //  }
  //}

  //if( segment->HasRtRoiPoints() )
  //{
  //  std::string filename = path + std::string("/") + std::string(segmentElement->GetAttribute("RoiPointsFilename"));
  //  if( this->WriteModelDataInternal(segment->GetRtRoiPoints(), filename) != 1 )
  //  {
  //    vtkWarningMacro("Unable to write poly data for segment " << segment->GetUid() <<".");
  //  }
  //}

  return 1;
}