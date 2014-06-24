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

#ifndef __vtkMRMLContourStorageNode_h
#define __vtkMRMLContourStorageNode_h

// Contour includes
#include "vtkSlicerContoursModuleMRMLExport.h"

// MRML includes
#include "vtkMRMLStorageNode.h"

class vtkMRMLContourNode;
class vtkPolyData;
class vtkXMLDataElement;

/// \brief MRML node for contour storage on disk.
///
/// Storage nodes has methods to read/write contours to/from disk.
class VTK_SLICER_CONTOURS_MODULE_MRML_EXPORT vtkMRMLContourStorageNode : public vtkMRMLStorageNode
{
  /// TODO : storage node needs to know about all files related to the data node
  /// when saving to mrb, it does some fancy magic to package it all together

public:
  static vtkMRMLContourStorageNode *New();
  vtkTypeMacro(vtkMRMLContourStorageNode, vtkMRMLStorageNode);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual vtkMRMLNode* CreateNodeInstance();

  ///
  /// Read node attributes from XML file
  virtual void ReadXMLAttributes( const char** atts);

  ///
  /// Write this node's information to a MRML file in XML format.
  virtual void WriteXML(ostream& of, int indent);

  ///
  /// Copy the node's attributes to this object
  virtual void Copy(vtkMRMLNode *node);

  /// Get node XML tag name (like Storage, Model)
  virtual const char* GetNodeTagName()  {return "ContourStorage";};

  /// Return a default file extension for writing
  virtual const char* GetDefaultWriteFileExtension();

  /// Return true if the reference node can be read in
  virtual bool CanReadInReferenceNode(vtkMRMLNode *refNode);

  ///
  /// Center image on read
  vtkGetMacro(CenterImage, int);
  vtkSetMacro(CenterImage, int);

  ///
  /// whether to read single file or the whole series
  vtkGetMacro(SingleFile, int);
  vtkSetMacro(SingleFile, int);

  ///
  /// Whether to use orientation from file
  vtkSetMacro(UseOrientationFromFile, int);
  vtkGetMacro(UseOrientationFromFile, int);

  ///
  /// Do a temp write to update the file list in this storage node with all
  /// file names that are written when write out the ref node
  /// If move is 1, return the directory that contains the written files and
  /// only the written files, for use in a move instead of a double
  /// write. Otherwise return an empty string.
  std::string UpdateFileList(vtkMRMLContourNode *refNode, int move = 0);

protected:
  vtkMRMLContourStorageNode();
  ~vtkMRMLContourStorageNode();
  vtkMRMLContourStorageNode(const vtkMRMLContourStorageNode&);
  void operator=(const vtkMRMLContourStorageNode&);

  int CenterImage;
  int SingleFile;
  int UseOrientationFromFile;

  /// Initialize all the supported read file types
  virtual void InitializeSupportedReadFileTypes();

  /// Initialize all the supported write file types
  virtual void InitializeSupportedWriteFileTypes();

  /// Read data and set it in the referenced node
  /// TODO : when source representation concept exists, then store the existing converted representations as xml attributes in either the contour node or contour storage node
  virtual int ReadDataInternal(vtkMRMLNode *refNode);

  /// Write data from a referenced node
  /// TODO : save only source representation - this implies the source representation mechanism exists
  virtual int WriteDataInternal(vtkMRMLNode *refNode);

  /// Write model data
  virtual int WriteModelDataInternal(vtkPolyData* polyData, std::string& filename);

  /// Write image data
  virtual int WriteImageDataInternal(vtkMRMLContourNode* contourNode);

  /// Read model data
  virtual bool ReadModelDataInternal( vtkMRMLContourNode* contourNode, vtkPolyData* outModel, const char* filename, const char* suffix);

  /// Read image data
  virtual int ReadImageDataInternal(vtkMRMLContourNode* contourNode);

  /// Write the location of the other files to disc
  virtual vtkXMLDataElement* CreateXMLElement(const std::string& baseFilename);
};

#endif
