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

#ifndef __vtkMRMLSegmentationStorageNode_h
#define __vtkMRMLSegmentationStorageNode_h

// Segmentation includes
#include "vtkSlicerSegmentationsModuleMRMLExport.h"

// MRML includes
#include "vtkMRMLStorageNode.h"

// ITK includes
#include <itkMetaDataDictionary.h>

class vtkMRMLSegmentationNode;
class vtkMatrix4x4;
class vtkPolyData;
class vtkOrientedImageData;
class vtkSegment;
class vtkXMLDataElement;

/// \brief MRML node for segmentation storage on disk.
///
/// Storage nodes has methods to read/write segmentations to/from disk.
class VTK_SLICER_SEGMENTATIONS_MODULE_MRML_EXPORT vtkMRMLSegmentationStorageNode : public vtkMRMLStorageNode
{
  /// TODO : storage node needs to know about all files related to the data node
  /// when saving to mrb, it does some fancy magic to package it all together

public:
  static vtkMRMLSegmentationStorageNode *New();
  vtkTypeMacro(vtkMRMLSegmentationStorageNode, vtkMRMLStorageNode);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual vtkMRMLNode* CreateNodeInstance();

  /// Read node attributes from XML file
  virtual void ReadXMLAttributes( const char** atts);

  /// Write this node's information to a MRML file in XML format.
  virtual void WriteXML(ostream& of, int indent);

  /// Copy the node's attributes to this object
  virtual void Copy(vtkMRMLNode *node);

  /// Get node XML tag name (like Storage, Model)
  virtual const char* GetNodeTagName()  {return "SegmentationStorage";};

  /// Return a default file extension for writing
  virtual const char* GetDefaultWriteFileExtension();

  /// Return true if the reference node can be read in
  virtual bool CanReadInReferenceNode(vtkMRMLNode *refNode);

  /// Do a temp write to update the file list in this storage node with all
  /// file names that are written when write out the ref node
  /// If move is 1, return the directory that contains the written files and
  /// only the written files, for use in a move instead of a double
  /// write. Otherwise return an empty string.
  std::string UpdateFileList(vtkSegment* segment, vtkMatrix4x4* IJKToRASMatrix, int move = 0);

protected:
  vtkMRMLSegmentationStorageNode();
  ~vtkMRMLSegmentationStorageNode();
  vtkMRMLSegmentationStorageNode(const vtkMRMLSegmentationStorageNode&);
  void operator=(const vtkMRMLSegmentationStorageNode&);

  /// Initialize all the supported read file types
  virtual void InitializeSupportedReadFileTypes();

  /// Initialize all the supported write file types
  virtual void InitializeSupportedWriteFileTypes();

  /// Read data and set it in the referenced node
  virtual int ReadDataInternal(vtkMRMLNode *refNode);

  /// Write data from a referenced node
  virtual int WriteDataInternal(vtkMRMLNode *refNode);

  /// Write poly data
  virtual int WritePolyDataInternal(vtkPolyData* polyData, std::string& filename);

  /// Write oriented image data
  virtual int WriteOrientedImageDataInternal(vtkOrientedImageData* imageData);

  /// Write segment data
  virtual int WriteSegmentInternal(vtkXMLDataElement* segmentElement, vtkSegment* segment, const std::string& path, vtkMatrix4x4* IJKToRASMatrix);

  /// Read poly data
  virtual bool ReadPolyDataInternal(vtkPolyData* outModel, const char* filename, const char* suffix);

  /// Read oriented image data
  virtual bool ReadOrientedImageDataInternal(vtkOrientedImageData* imageData, itk::MetaDataDictionary& outDictionary);

  /// Read segment data
  virtual vtkSegment* ReadSegmentInternal(const std::string& path, vtkXMLDataElement* segmentElement, itk::MetaDataDictionary& outDictionary);

  /// Write the location of the other files to disc
  virtual vtkXMLDataElement* CreateXMLElement(vtkMRMLSegmentationNode& node, const std::string& baseFilename);
};

#endif
