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

// SlicerRt includes
#include "SlicerRtCommon.h"
#include "vtkMRMLContourStorageNode.h"

// MRML includes
#include "vtkMRMLContourModelDisplayNode.h"
#include "vtkMRMLContourNode.h"

// VTK includes
#include <vtkCallbackCommand.h>
#include <vtkCollection.h>
#include <vtkDataFileFormatHelper.h>
#include <vtkDataIOManager.h>
#include <vtkDataSetSurfaceFilter.h>
#include <vtkITKArchetypeImageSeriesScalarReader.h>
#include <vtkImageChangeInformation.h>
#include <vtkMRMLScene.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkPolyDataReader.h>
#include <vtkPolyDataWriter.h>
#include <vtkStringArray.h>
#include <vtkUnstructuredGrid.h>
#include <vtkUnstructuredGridReader.h>
#include <vtkXMLDataElement.h>
#include <vtkXMLUtilities.h>
#include <vtksys/Directory.hxx>
#include <vtksys/SystemTools.hxx>

// VTK ITK includes
#include "vtkITKImageWriter.h"

// STL includes
#include <sstream>

//----------------------------------------------------------------------------
namespace
{

  //----------------------------------------------------------------------------
  void ApplyImageSeriesReaderWorkaround(vtkMRMLContourStorageNode * storageNode,
    vtkITKArchetypeImageSeriesReader * reader,
    const std::string& filename)
  {
    // TODO: this is a workaround for an issue in itk::ImageSeriesReader
    // where is assumes that all the filenames that have been passed
    // to it are a dimension smaller than the image it is asked to create
    // (i.e. a list of .jpg files that form a volume).
    // In our case though, we can have file lists that include both the
    // header and bulk data, like .hdr/.img pairs.  So we need to
    // be careful not to send extra filenames to the reader if the
    // format is multi-file for the same volume
    //
    // check for Analyze and similar format- if the archetype is
    // one of those, then don't send the rest of the list
    //
    std::string fileExt=vtkMRMLStorageNode::GetLowercaseExtensionFromFileName(filename);
    if ( fileExt != std::string(".hdr")
      && fileExt != std::string(".img")
      && fileExt != std::string(".mhd")
      && fileExt != std::string(".nhdr") )
    {
      for (int n = 0; n < storageNode->GetNumberOfFileNames(); n++)
      {
        std::string nthFileName = storageNode->GetFullNameFromNthFileName(n);
        vtkDebugWithObjectMacro(storageNode,
          "ReadData: got full name for " << n << "th file: " << nthFileName
          << ", adding it to reader, current num files on it = "
          << reader->GetNumberOfFileNames());
        reader->AddFileName(nthFileName.c_str());
      }
    }
  }
} // end of anonymous namespace

//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLContourStorageNode);

//----------------------------------------------------------------------------
vtkMRMLContourStorageNode::vtkMRMLContourStorageNode()
: CenterImage(0)
, SingleFile(1)
, UseOrientationFromFile(1)
{
  //this->InitializeSupportedWriteFileTypes();
  // this->InitializeSupportedReadFileTypes();
}

//----------------------------------------------------------------------------
vtkMRMLContourStorageNode::~vtkMRMLContourStorageNode()
{
}

//----------------------------------------------------------------------------
void vtkMRMLContourStorageNode::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkMRMLStorageNode::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
bool vtkMRMLContourStorageNode::CanReadInReferenceNode(vtkMRMLNode *refNode)
{
  return refNode->IsA("vtkMRMLContourNode");
}

//----------------------------------------------------------------------------
int vtkMRMLContourStorageNode::ReadDataInternal(vtkMRMLNode *refNode)
{
  vtkMRMLContourNode *contourNode = dynamic_cast <vtkMRMLContourNode *> (refNode);

  std::string fullName = this->GetFullNameFromFileName();
  if (fullName == std::string(""))
  {
    vtkErrorMacro("ReadDataInternal: File name not specified");
    return 0;
  }

  // check that the file exists
  if (vtksys::SystemTools::FileExists(fullName.c_str()) == false)
  {
    vtkErrorMacro("ReadDataInternal: model file '" << fullName.c_str() << "' not found.");
    return 0;
  }

  std::string path = vtksys::SystemTools::GetFilenamePath(fullName);

  vtkSmartPointer<vtkXMLDataElement> element = vtkSmartPointer<vtkXMLDataElement>::Take(vtkXMLUtilities::ReadElementFromFile(fullName.c_str()));
  int result = 1;
  try
  {
    std::string labelmapFile = path + std::string("/") + std::string(element->GetAttribute("LabelmapFilename"));
    // Give the labelmap a chance to load if it exists
    if( vtksys::SystemTools::FileExists(labelmapFile.c_str()) )
    {
      std::string origFilename(this->GetFileName());
      this->SetFileName(labelmapFile.c_str());
      this->ReadImageDataInternal(contourNode);
      this->SetFileName(origFilename.c_str());
    }

    // Build out other for closed surface and ribbon model
    std::string ribbonFile = path + std::string("/") + std::string(element->GetAttribute("RibbonModelFilename"));
    if( vtksys::SystemTools::FileExists(ribbonFile.c_str()) )
    {
      // Load the ribbon model!
      vtkSmartPointer<vtkPolyData> model = vtkSmartPointer<vtkPolyData>::New();
      if( this->ReadModelDataInternal(contourNode, model, ribbonFile.c_str(), SlicerRtCommon::CONTOUR_RIBBON_MODEL_NODE_NAME_POSTFIX.c_str()) )
      {
        contourNode->SetAndObserveRibbonModelPolyData( model );
      }
      else
      {
        vtkErrorMacro("Unable to read model data for ribbon model.");
        result = 0;
      }
    }

    std::string closedSurfaceFile = path + std::string("/") + std::string(element->GetAttribute("ClosedSurfaceModelFilename"));
    if( vtksys::SystemTools::FileExists(closedSurfaceFile.c_str()) )
    {
      // Load the closed surface!
      vtkSmartPointer<vtkPolyData> model = vtkSmartPointer<vtkPolyData>::New();
      if( this->ReadModelDataInternal(contourNode, model, closedSurfaceFile.c_str(), SlicerRtCommon::CONTOUR_CLOSED_SURFACE_MODEL_NODE_NAME_POSTFIX.c_str()) )
      {
        contourNode->SetAndObserveClosedSurfacePolyData( model );
      }
      else
      {
        vtkErrorMacro("Unable to read model data for closed surface model.");
        result = 0;
      }
    }

    std::string roiPointsFile = path + std::string("/") + std::string(element->GetAttribute("RoiPointsFilename"));
    if( vtksys::SystemTools::FileExists(roiPointsFile.c_str()) )
    {
      // Load the rt roi points
      vtkSmartPointer<vtkPolyData> model = vtkSmartPointer<vtkPolyData>::New();
      if( this->ReadModelDataInternal(NULL, model, roiPointsFile.c_str(), "", false) )
      {
        contourNode->SetDicomRtRoiPoints( model );
      }
      else
      {
        vtkErrorMacro("Unable to read model data for rt roi model.");
        result = 0;
      }
    }
  }
  catch (...)
  {
    result = 0;
  }

  return result;
}

//----------------------------------------------------------------------------
int vtkMRMLContourStorageNode::WriteDataInternal(vtkMRMLNode *refNode)
{
  vtkMRMLContourNode *contourNode = vtkMRMLContourNode::SafeDownCast(refNode);

  std::string fullName = this->GetFullNameFromFileName();
  if (fullName == std::string(""))
  {
    vtkErrorMacro("vtkMRMLModelNode: File name not specified");
    return 0;
  }

  if (!contourNode->HasRepresentation(vtkMRMLContourNode::IndexedLabelmap) && !contourNode->HasRepresentation(vtkMRMLContourNode::RibbonModel) && !contourNode->HasRepresentation(vtkMRMLContourNode::ClosedSurfaceModel))
  {
    vtkErrorMacro("Cannot write contour data representations, all representations are NULL!");
    return 0;
  }

  vtkSmartPointer<vtkXMLDataElement> element = vtkSmartPointer<vtkXMLDataElement>::Take(this->CreateXMLElement(fullName));

  std::string path = vtksys::SystemTools::GetFilenamePath(fullName);

  if( contourNode->HasRepresentation(vtkMRMLContourNode::IndexedLabelmap) )
  {
    std::string labelmapFile = path + std::string("/") + std::string(element->GetAttribute("LabelmapFilename"));
    // Back up filename, functions below mess it all up
    std::string origFilename(this->GetFileName());
    this->SetFileName(labelmapFile.c_str());
    this->WriteImageDataInternal(contourNode);
    this->SetFileName(origFilename.c_str());
  }

  if( contourNode->HasRepresentation(vtkMRMLContourNode::RibbonModel) )
  {
    std::string ribbonFile = path + std::string("/") + std::string(element->GetAttribute("RibbonModelFilename"));
    this->WriteModelDataInternal(contourNode->GetRibbonModelPolyData(), ribbonFile.c_str());
  }

  if( contourNode->HasRepresentation(vtkMRMLContourNode::ClosedSurfaceModel) )
  {
    std::string closedSurfaceFile = path + std::string("/") + std::string(element->GetAttribute("ClosedSurfaceModelFilename"));
    this->WriteModelDataInternal(contourNode->GetClosedSurfacePolyData(), closedSurfaceFile.c_str());
  }

  std::string pointsFile = path + std::string("/") + std::string(element->GetAttribute("RoiPointsFilename"));
  this->WriteModelDataInternal(contourNode->GetDicomRtRoiPoints(), pointsFile.c_str());

  // TODO : any way to cleanly save out the MetaDataDictionary?
  // TODO : do we even have to?

  vtkXMLUtilities::WriteElementToFile(element, fullName.c_str());

  return 1;
}

//----------------------------------------------------------------------------
void vtkMRMLContourStorageNode::InitializeSupportedReadFileTypes()
{
  this->SupportedReadFileTypes->InsertNextValue("Contour (.ctr)");
}

//----------------------------------------------------------------------------
void vtkMRMLContourStorageNode::InitializeSupportedWriteFileTypes()
{
  this->SupportedWriteFileTypes->InsertNextValue("Contour (.ctr)");
}

//----------------------------------------------------------------------------
const char* vtkMRMLContourStorageNode::GetDefaultWriteFileExtension()
{
  return "ctr";
}

//----------------------------------------------------------------------------
int vtkMRMLContourStorageNode::WriteModelDataInternal( vtkPolyData* polyData, const char* filename )
{
  // Write out the ribbon model data if it exists
  vtkNew<vtkPolyDataWriter> writer;
  writer->SetFileName(filename);
  writer->SetFileType(this->GetUseCompression() ? VTK_BINARY : VTK_ASCII );
  writer->SetInput( polyData );
  try
  {
    writer->Write();
  }
  catch (...)
  {
    return 0;
  }

  return 1;
}

//----------------------------------------------------------------------------
bool vtkMRMLContourStorageNode::ReadModelDataInternal( vtkMRMLContourNode* contourNode, vtkPolyData* outModel, const char* filename, const char* suffix, bool createDisplayNode )
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
    surfaceFilter->SetInput(unstructuredGridReader->GetOutput());
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

  if( createDisplayNode )
  {
    // Create display node for the new model data
    vtkSmartPointer<vtkMRMLContourModelDisplayNode> displayNode = vtkSmartPointer<vtkMRMLContourModelDisplayNode>::New();
    std::string nodeName = std::string(contourNode->GetName()) + std::string(suffix) + SlicerRtCommon::CONTOUR_DISPLAY_NODE_SUFFIX;
    displayNode->SetName(nodeName.c_str());
    displayNode->SetInputPolyData(outModel);
    this->GetScene()->AddNode(displayNode);
    contourNode->AddAndObserveDisplayNodeID(displayNode->GetID());
  }

  return true;
}

//----------------------------------------------------------------------------
vtkXMLDataElement* vtkMRMLContourStorageNode::CreateXMLElement( const std::string& baseFilename )
{
  std::string extension = vtkMRMLStorageNode::GetLowercaseExtensionFromFileName(baseFilename);
  std::string fullNameWithoutExtension = vtksys::SystemTools::GetFilenameWithoutExtension(baseFilename);

  vtkXMLDataElement* element = vtkXMLDataElement::New();
  element->SetName("ContourNode");
  {
    std::stringstream ss;
    ss << fullNameWithoutExtension << SlicerRtCommon::CONTOUR_INDEXED_LABELMAP_NODE_NAME_POSTFIX << SlicerRtCommon::CONTOUR_LABELMAP_FILE_TYPE;
    element->SetAttribute("LabelmapFilename", ss.str().c_str());
  }
  {
    std::stringstream ss;
    ss << fullNameWithoutExtension << SlicerRtCommon::CONTOUR_RIBBON_MODEL_NODE_NAME_POSTFIX << SlicerRtCommon::CONTOUR_MODEL_FILE_TYPE;
    element->SetAttribute("RibbonModelFilename", ss.str().c_str());
  }
  {
    std::stringstream ss;
    ss << fullNameWithoutExtension << SlicerRtCommon::CONTOUR_CLOSED_SURFACE_MODEL_NODE_NAME_POSTFIX << SlicerRtCommon::CONTOUR_MODEL_FILE_TYPE;
    element->SetAttribute("ClosedSurfaceModelFilename", ss.str().c_str());
  }
  {
    std::stringstream ss;
    ss << fullNameWithoutExtension << "_RoiPoints" << SlicerRtCommon::CONTOUR_MODEL_FILE_TYPE;
    element->SetAttribute("RoiPointsFilename", ss.str().c_str());
  }
  return element;
}

//----------------------------------------------------------------------------
int vtkMRMLContourStorageNode::WriteImageDataInternal( vtkMRMLContourNode* contourNode )
{
  int result(0);
  if ( !contourNode->HasRepresentation(vtkMRMLContourNode::IndexedLabelmap) )
  {
    vtkErrorMacro("cannot write ImageData, it's NULL");
    return result;
  }

  // update the file list
  std::string moveFromDir = this->UpdateFileList(contourNode, 1);

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

    writer->SetInput( contourNode->GetLabelmapImageData() );
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
    vtkNew<vtkMatrix4x4> mat;
    contourNode->GetRASToIJKMatrix(mat.GetPointer());
    writer->SetRasToIJKMatrix(mat.GetPointer());

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
std::string vtkMRMLContourStorageNode::UpdateFileList(vtkMRMLContourNode *refNode, int move)
{
  bool result = true;
  std::string returnString = "";
  // test whether refNode is a valid node to hold a volume
  if (!refNode->IsA("vtkMRMLContourNode") )
  {
    vtkErrorMacro("Reference node is not a vtkMRMLContourNode");
    return returnString;
  }

  if ( refNode == NULL || !refNode->HasRepresentation(vtkMRMLContourNode::IndexedLabelmap) )
  {
    vtkErrorMacro("UpdateFileList: cannot write ImageData, it's NULL");
    return returnString;
  }

  std::string oldName = std::string(this->GetFileName());
  if (oldName == std::string(""))
  {
    vtkErrorMacro("UpdateFileList: File name not specified");
    return returnString;
  }

  vtkDebugMacro("UpdateFileList: old file name = " << oldName);

  // clear out the old file list
  this->ResetFileNameList();

  // get the original directory
  std::string originalDir = vtksys::SystemTools::GetParentDirectory(oldName.c_str());
  std::vector<std::string> pathComponents;
  vtksys::SystemTools::SplitPath(originalDir.c_str(), pathComponents);
  // add a temp dir to it
  pathComponents.push_back(std::string("TempWrite") +
    vtksys::SystemTools::GetFilenameWithoutExtension(oldName));
  std::string tempDir = vtksys::SystemTools::JoinPath(pathComponents);
  vtkDebugMacro("UpdateFileList: deleting and then re-creating temp dir "<< tempDir.c_str());
  if (vtksys::SystemTools::FileExists(tempDir.c_str()))
  {
    result = vtksys::SystemTools::RemoveADirectory(tempDir.c_str());
  }
  if (!result)
  {
    vtkErrorMacro("UpdateFileList: Failed to delete directory '" << tempDir << "'.");
    return returnString;
  }
  result = vtksys::SystemTools::MakeDirectory(tempDir.c_str());
  if (!result)
  {
    vtkErrorMacro("UpdateFileList: Failed to create directory" << tempDir);
    return returnString;
  }
  // make a new name,
  pathComponents.push_back(vtksys::SystemTools::GetFilenameName(oldName));
  std::string tempName = vtksys::SystemTools::JoinPath(pathComponents);
  vtkDebugMacro("UpdateFileList: new archetype file name = " << tempName.c_str());

  // set up the writer and write
  vtkNew<vtkITKImageWriter> writer;
  writer->SetFileName(tempName.c_str());

  writer->SetInput( refNode->GetLabelmapImageData() );
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
  vtkNew<vtkMatrix4x4> mat;
  refNode->GetRASToIJKMatrix(mat.GetPointer());
  writer->SetRasToIJKMatrix(mat.GetPointer());

  try
  {
    writer->Write();
  }
  catch (itk::ExceptionObject& exception)
  {
    exception.Print(std::cerr);
    result = false;
  }

  if (!result)
  {
    vtkErrorMacro("UpdateFileList: Failed to write '" << tempName.c_str()
      << "'.");
    return returnString;
  }

  // look through the new dir and populate the file list
  vtksys::Directory dir;
  result = dir.Load(tempDir.c_str());
  vtkDebugMacro("UpdateFileList: tempdir " << tempDir.c_str() << " has " << dir.GetNumberOfFiles() << " in it");
  if (!result)
  {
    vtkErrorMacro("UpdateFileList: Failed to open directory '" << tempDir.c_str() << "'.");
    return returnString;
  }

  // take the archetype and temp dir off of the path
  pathComponents.pop_back();
  pathComponents.pop_back();
  std::string localDirectory = vtksys::SystemTools::JoinPath(pathComponents);
  std::string relativePath;

  if (this->IsFilePathRelative(localDirectory.c_str()))
  {
    vtkDebugMacro("UpdateFileList: the local directory is already relative, use it " << localDirectory);
    relativePath = localDirectory;
  }
  else
  {
    if (refNode->GetScene() != NULL &&
      strcmp(refNode->GetScene()->GetRootDirectory(), "") != 0)
    {
      // use the scene's root dir, all the files in the list will be
      // relative to it (the relative path is how you go from the root dir to
      // the dir in which the volume is saved)
      std::string rootDir = refNode->GetScene()->GetRootDirectory();
      if (rootDir.length() != 0 &&
        rootDir.find_last_of("/") == rootDir.length() - 1)
      {
        vtkDebugMacro("UpdateFileList: found trailing slash in : " << rootDir);
        rootDir = rootDir.substr(0, rootDir.length()-1);
      }
      vtkDebugMacro("UpdateFileList: got the scene root dir " << rootDir << ", local dir = " << localDirectory.c_str());
      // RelativePath requires two absolute paths, otherwise returns empty
      // string
      if (this->IsFilePathRelative(rootDir.c_str()))
      {
        vtkDebugMacro("UpdateFileList: have a relative directory in root dir (" << rootDir << "), using the local dir as a relative path.");
        // assume the relative local directory is relative to the root
        // directory
        relativePath = localDirectory;
      }
      else
      {
        relativePath = vtksys::SystemTools::RelativePath(rootDir.c_str(), localDirectory.c_str());
      }
    }
    else
    {
      // use the archetype's directory, so that all the files in the list will
      // be relative to it
      if (this->IsFilePathRelative(originalDir.c_str()))
      {
        relativePath = localDirectory;
      }
      else
      {
        // the RelativePath method needs two absolute paths
        relativePath = vtksys::SystemTools::RelativePath(originalDir.c_str(), localDirectory.c_str());
      }
      vtkDebugMacro("UpdateFileList: no scene root dir, using original dir = " << originalDir.c_str() << " and local dir " << localDirectory.c_str());
    }
  }
  // strip off any trailing slashes
  if (relativePath.length() != 0 &&
    relativePath.find_last_of("/")  != std::string::npos &&
    relativePath.find_last_of("/") == relativePath.length() - 1)
  {
    vtkDebugMacro("UpdateFileList: stripping off a trailing slash from relativePath '"<< relativePath.c_str() << "'");
    relativePath = relativePath.substr(0, relativePath.length() - 1);
  }
  vtkDebugMacro("UpdateFileList: using prefix of relative path '" << relativePath.c_str() << "'");
  // now get ready to join the relative path to thisFile
  std::vector<std::string> relativePathComponents;
  vtksys::SystemTools::SplitPath(relativePath.c_str(), relativePathComponents);

  // make sure that the archetype is added first! AddFile when it gets to it
  // in the dir will not add a duplicate
  std::string newArchetype = vtksys::SystemTools::GetFilenameName(tempName.c_str());
  vtkDebugMacro("Stripped archetype = " << newArchetype.c_str());
  relativePathComponents.push_back(newArchetype);
  std::string relativeArchetypeFile =  vtksys::SystemTools::JoinPath(relativePathComponents);
  vtkDebugMacro("Relative archetype = " << relativeArchetypeFile.c_str());
  relativePathComponents.pop_back();
  this->AddFileName(relativeArchetypeFile.c_str());

  bool addedArchetype = false;
  // now iterate through the directory files
  for (size_t fileNum = 0; fileNum < dir.GetNumberOfFiles(); ++fileNum)
  {
    // skip the dirs
    const char *thisFile = dir.GetFile(static_cast<unsigned long>(fileNum));
    if (strcmp(thisFile,".") &&
      strcmp(thisFile,".."))
    {
      vtkDebugMacro("UpdateFileList: adding file number " << fileNum << ", " << thisFile);
      if (newArchetype.compare(thisFile) == 0)
      {
        addedArchetype = true;
      }
      // at this point, the file name is bare of a directory, turn it into a
      // relative path from the original archetype
      relativePathComponents.push_back(thisFile);
      std::string relativeFile =  vtksys::SystemTools::JoinPath(relativePathComponents);
      relativePathComponents.pop_back();
      vtkDebugMacro("UpdateFileList: " << fileNum << ", using relative file name " << relativeFile.c_str());
      this->AddFileName(relativeFile.c_str());
    }
  }
  result = addedArchetype;
  if (!result)
  {
    std::stringstream addedFiles;
    std::copy(++this->FileNameList.begin(), this->FileNameList.end(),
      std::ostream_iterator<std::string>(addedFiles,", "));
    vtkErrorMacro("UpdateFileList: the archetype file '"
      << newArchetype.c_str() << "' wasn't written out when writting '"
      << tempName.c_str() << "' in '" << tempDir.c_str() << "'. "
      << "Only those " << dir.GetNumberOfFiles() - 2
      << " file(s) have been written: " << addedFiles.str().c_str() <<". "
      << "Old name is '" << oldName.c_str() << "'."
      );
    return returnString;
  }
  // restore the old file name
  vtkDebugMacro("UpdateFileList: resetting file name to " << oldName.c_str());
  this->SetFileName(oldName.c_str());

  if (move != 1)
  {
    // clean up temp directory
    vtkDebugMacro("UpdateFileList: removing temp dir " << tempDir);
    result = vtksys::SystemTools::RemoveADirectory(tempDir.c_str());
    if (!result)
    {
      vtkErrorMacro("UpdateFileList: failed to remove temp dir '"
        << tempDir.c_str() << "'." );
      return returnString;
    }
    return std::string("");
  }
  else
  {
    vtkDebugMacro("UpdateFileList: returning temp dir " << tempDir);
    return tempDir;
  }
}

//----------------------------------------------------------------------------
int vtkMRMLContourStorageNode::ReadImageDataInternal( vtkMRMLContourNode* contourNode )
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

  std::string filename = this->GetFullNameFromFileName();

  vtkSmartPointer<vtkITKArchetypeImageSeriesReader> reader = vtkSmartPointer<vtkITKArchetypeImageSeriesScalarReader>::New();
  reader->SetSingleFile( this->GetSingleFile() );
  reader->SetUseOrientationFromFile( this->GetUseOrientationFromFile() );

  if (reader.GetPointer() == NULL)
  {
    vtkErrorMacro("ReadData: Failed to instantiate a file reader");
    return 0;
  }

  reader->AddObserver( vtkCommand::ProgressEvent, this->MRMLCallbackCommand);

  if ( contourNode->HasRepresentation(vtkMRMLContourNode::IndexedLabelmap) )
  {
    contourNode->SetAndObserveLabelmapImageData(NULL);
  }

  // Set the list of file names on the reader
  reader->ResetFileNames();
  reader->SetArchetype(filename.c_str());

  // Workaround
  ApplyImageSeriesReaderWorkaround(this, reader, filename);

  // Center image
  reader->SetOutputScalarTypeToNative();
  reader->SetDesiredCoordinateOrientationToNative();
  if (this->CenterImage)
  {
    reader->SetUseNativeOriginOff();
  }
  else
  {
    reader->SetUseNativeOriginOn();
  }

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
      << (contourNode ? contourNode->GetNodeTagName() : "null")
      << "[" << "fullName = " << filename << "]\n"
      << "\tNumber of files listed in the node = "
      << this->GetNumberOfFileNames() << ".\n"
      << "\tFile reader says it was able to read "
      << reader->GetNumberOfFileNames() << " files.\n"
      << "\tFile reader used the archetype file name of " << reader->GetArchetype()
      << " [" << reader0thFileName.c_str() << "]\n");
    return 0;
  }

  if (reader->GetOutput() == NULL || reader->GetOutput()->GetPointData() == NULL)
  {
    vtkErrorMacro("ReadData: Unable to read data from file: " << filename);
    return 0;
  }

  vtkPointData * pointData = reader->GetOutput()->GetPointData();
  if (pointData->GetScalars() == NULL || pointData->GetScalars()->GetNumberOfTuples() == 0)
  {
    vtkErrorMacro("ReadData: Unable to read ScalarVolume data from file: " << filename );
    return 0;
  }

  if ( reader->GetNumberOfComponents() != 1 )
  {
    vtkErrorMacro("ReadData: Not a scalar volume file: " << filename );
    return 0;
  }

  // Set volume attributes
  contourNode->SetMetaDataDictionary( reader->GetMetaDataDictionary() );

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
  ici->SetInput(reader->GetOutput());
  ici->SetOutputSpacing( 1, 1, 1 );
  ici->SetOutputOrigin( 0, 0, 0 );
  ici->Update();

  if (ici->GetOutput() == NULL)
  {
    vtkErrorMacro("vtkMRMLVolumeArchetypeStorageNode: Cannot read file: " << filename);
    return 0;
  }

  vtkNew<vtkImageData> iciOutputCopy;
  iciOutputCopy->ShallowCopy(ici->GetOutput());
  contourNode->SetAndObserveLabelmapImageData(iciOutputCopy.GetPointer());

  vtkMatrix4x4* mat = reader->GetRasToIjkMatrix();
  if ( mat == NULL )
  {
    vtkErrorMacro ("Reader returned NULL RasToIjkMatrix");
  }
  contourNode->SetRASToIJKMatrix(mat);

  // Create display node for labelmap visualization
  // TODO : when contours are integrated into the core, link up the 2d display

  return 1;
}

//----------------------------------------------------------------------------
void vtkMRMLContourStorageNode::ReadXMLAttributes(const char** atts)
{
  int disabledModify = this->StartModify();

  Superclass::ReadXMLAttributes(atts);

  const char* attName;
  const char* attValue;
  while (*atts != NULL)
  {
    attName = *(atts++);
    attValue = *(atts++);
    if (!strcmp(attName, "centerImage"))
    {
      std::stringstream ss;
      ss << attValue;
      ss >> this->CenterImage;
    }
    if (!strcmp(attName, "singleFile"))
    {
      std::stringstream ss;
      ss << attValue;
      ss >> this->SingleFile;
    }
    if (!strcmp(attName, "UseOrientationFromFile"))
    {
      std::stringstream ss;
      ss << attValue;
      ss >> this->UseOrientationFromFile;
    }
  }

  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLContourStorageNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);
  vtkIndent indent(nIndent);
  {
    std::stringstream ss;
    ss << this->CenterImage;
    of << indent << " centerImage=\"" << ss.str() << "\"";
  }
  {
    std::stringstream ss;
    ss << this->SingleFile;
    of << indent << " singleFile=\"" << ss.str() << "\"";
  }
  {
    std::stringstream ss;
    ss << this->UseOrientationFromFile;
    of << indent << " UseOrientationFromFile=\"" << ss.str() << "\"";
  }
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, StorageID
void vtkMRMLContourStorageNode::Copy(vtkMRMLNode *anode)
{
  int disabledModify = this->StartModify();

  Superclass::Copy(anode);
  vtkMRMLContourStorageNode *node = (vtkMRMLContourStorageNode *) anode;

  this->SetCenterImage(node->CenterImage);
  this->SetSingleFile(node->SingleFile);
  this->SetUseOrientationFromFile(node->UseOrientationFromFile);

  this->EndModify(disabledModify);
}