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

#ifndef __vtkMRMLSegmentationStorageNode_h
#define __vtkMRMLSegmentationStorageNode_h

// Segmentation includes
#include "vtkSlicerSegmentationsModuleMRMLExport.h"

// MRML includes
#include "vtkMRMLStorageNode.h"

// ITK includes
#include <itkImageRegionIteratorWithIndex.h>

class vtkMRMLSegmentationNode;
class vtkMatrix4x4;
class vtkPolyData;
class vtkOrientedImageData;
class vtkSegmentation;
class vtkSegment;
class vtkXMLDataElement; //TODO

/// \brief MRML node for segmentation storage on disk.
///
/// Storage nodes has methods to read/write segmentations to/from disk.
class VTK_SLICER_SEGMENTATIONS_MODULE_MRML_EXPORT vtkMRMLSegmentationStorageNode : public vtkMRMLStorageNode
{
  // Although internally binary labelmap representations can be of unsigned char, unsigned short
  // or short types, the output file is always unsigned char
  //TODO: This is a limitation for now
  typedef itk::Image<unsigned char, 4> BinaryLabelmap4DImageType;
  typedef itk::ImageRegionIteratorWithIndex<BinaryLabelmap4DImageType> BinaryLabelmap4DIteratorType;

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

protected:
  /// Initialize all the supported read file types
  virtual void InitializeSupportedReadFileTypes();

  /// Initialize all the supported write file types
  virtual void InitializeSupportedWriteFileTypes();

  /// Get data node that is associated with this storage node
  vtkMRMLSegmentationNode* GetAssociatedDataNode();

  /// Write data from a referenced node
  virtual int WriteDataInternal(vtkMRMLNode *refNode);

  /// Write binary labelmap representation to file
  virtual int WriteBinaryLabelmapRepresentation(vtkSegmentation* segmentation, std::string path);

  /// Write a poly data representation to file
  virtual int WritePolyDataRepresentation(vtkSegmentation* segmentation, std::string path);

  /// Read data and set it in the referenced node
  virtual int ReadDataInternal(vtkMRMLNode *refNode);

  /// Read binary labelmap representation to file
  virtual int ReadBinaryLabelmapRepresentation(vtkSegmentation* segmentation, std::string path);

  /// Read a poly data representation to file
  virtual int ReadPolyDataRepresentation(vtkSegmentation* segmentation, std::string path);

  /* TODO:
  /// Write oriented image data
  virtual int WriteOrientedImageDataInternal(vtkOrientedImageData* imageData);

  /// Read poly data
  virtual bool ReadPolyDataInternal(vtkPolyData* outModel, const char* filename, const char* suffix);

  /// Read oriented image data
  virtual bool ReadOrientedImageDataInternal(vtkOrientedImageData* imageData, itk::MetaDataDictionary& outDictionary);

  /// Read segment data
  virtual vtkSegment* ReadSegmentInternal(const std::string& path, vtkXMLDataElement* segmentElement, itk::MetaDataDictionary& outDictionary);

  /// Write the location of the other files to disc
  virtual vtkXMLDataElement* CreateXMLElement(vtkMRMLSegmentationNode& node, const std::string& baseFilename);
  */
protected:
  vtkMRMLSegmentationStorageNode();
  ~vtkMRMLSegmentationStorageNode();

private:
  vtkMRMLSegmentationStorageNode(const vtkMRMLSegmentationStorageNode&);  /// Not implemented.
  void operator=(const vtkMRMLSegmentationStorageNode&);  /// Not implemented.
};

#endif
