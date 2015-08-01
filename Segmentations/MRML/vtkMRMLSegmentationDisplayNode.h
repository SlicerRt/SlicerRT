/*=auto=========================================================================

  Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $RCSfile: vtkMRMLSegmentationDisplayNode.h,v $
  Date:      $Date: 2006/03/19 17:12:29 $
  Version:   $Revision: 1.3 $

=========================================================================auto=*/

#ifndef __vtkMRMLSegmentationDisplayNode_h
#define __vtkMRMLSegmentationDisplayNode_h

#include "vtkMRMLLabelMapVolumeDisplayNode.h"

#include "vtkSlicerSegmentationsModuleMRMLExport.h"

class vtkMRMLColorTableNode;
class vtkVector3d;

/// \ingroup Segmentations
/// \brief MRML node for representing segmentation display attributes.
///
/// vtkMRMLSegmentationDisplayNode nodes describe how volume is displayed.
class VTK_SLICER_SEGMENTATIONS_MODULE_MRML_EXPORT vtkMRMLSegmentationDisplayNode : public vtkMRMLLabelMapVolumeDisplayNode
{
public:
  // Define constants
  static const std::string GetColorTableNodeNamePostfix() { return "_ColorTable"; };
  static const char* GetSegmentationColorNameBackground() { return "Background"; };
  static const char* GetSegmentationColorNameInvalid() { return "Invalid"; };
  static const char* GetSegmentationColorNameRemoved() { return "Removed"; };
  static unsigned short GetSegmentationColorIndexBackground() { return 0; };
  static unsigned short GetSegmentationColorIndexInvalid() { return 1; };

  /// Display properties per segment
  struct SegmentDisplayProperties
  {
    /// Visibility
    bool Visible;
    /// Displayed segment color (may be different than default color stored in segment)
    double Color[3];
    /// Segment opacity when displayed as poly data (labelmap will be opaque)
    double PolyDataOpacity;
  };

  typedef std::map<std::string, SegmentDisplayProperties> SegmentDisplayPropertiesMap;

public:
  static vtkMRMLSegmentationDisplayNode *New();
  vtkTypeMacro(vtkMRMLSegmentationDisplayNode,vtkMRMLLabelMapVolumeDisplayNode);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual vtkMRMLNode* CreateNodeInstance();

  /// Set node attributes from name/value pairs 
  virtual void ReadXMLAttributes( const char** atts);

  /// Write this node's information to a MRML file in XML format. 
  virtual void WriteXML(ostream& of, int indent);

  /// Get node XML tag name (like Volume, Model)
  virtual const char* GetNodeTagName() { return "SegmentationDisplay"; };

  /// Alternative method to propagate events generated in Display nodes
  virtual void ProcessMRMLEvents ( vtkObject * /*caller*/,
                                   unsigned long /*event*/,
                                   void * /*callData*/ );

public:
  /// Get name of representation that is displayed as poly data (determine if empty)
  char* GetPolyDataDisplayRepresentationName();
  /// Set name of representation that is displayed as poly data
  vtkSetStringMacro(PolyDataDisplayRepresentationName);

  /// Get enable transparency flag
  vtkGetMacro(EnableTransparencyInColorTable, bool);
  /// Set enable transparency flag
  vtkSetMacro(EnableTransparencyInColorTable, bool);
  /// Set enable transparency flag boolean functions
  vtkBooleanMacro(EnableTransparencyInColorTable, bool);

public:
  /// Create color table node for segmentation
  /// First two values are fixed: 0=Background, 1=Invalid
  /// The subsequent colors correspond to the segments in order of segment indices.
  /// \param segmentationNodeName Name of the segmentation node that is set to the color node with a postfix
  virtual vtkMRMLColorTableNode* CreateColorTableNode(const char* segmentationNodeName);

  /// Convenience function to set color to a segment.
  /// Makes changes in the color table node associated to the segmentation node.
  bool SetSegmentColor(std::string segmentId, double r, double g, double b, double a = 1.0);

  /// Get segment display properties for a specific segment
  /// \param segmentID Identifier of segment of which the properties are queried
  /// \param color Output argument for segment color
  /// \param polyDataOpacity Output argument for poly data opacity
  /// \return Success flag
  bool GetSegmentDisplayProperties(std::string segmentID, SegmentDisplayProperties &properties);

  /// Set segment display properties. Add new entry if not in list already
  void SetSegmentDisplayProperties(std::string segmentID, SegmentDisplayProperties &properties);

  /// Remove segment display properties
  void RemoveSegmentDisplayProperties(std::string segmentID);

  /// Clear segment display properties
  void ClearSegmentDisplayProperties();

  /// Determine and set automatic opacities for segments using topological hierarchies.
  /// Stores value in opacity component of \sa SegmentDisplayProperties.
  /// \return Success flag
  bool CalculateAutoOpacitiesForSegments();

  /// Collect representation names that are stored as poly data
  void GetPolyDataRepresentationNames(std::vector<std::string> &representationNames);

// Python compatibility functions
public:
  /// Get segment visibility by segment ID. Convenience function for python compatibility.
  bool GetSegmentVisibility(std::string segmentID);
  /// Set segment visibility by segment ID. Convenience function for python compatibility.
  void SetSegmentVisibility(std::string segmentID, bool visible);

  /// Get segment color by segment ID. Convenience function for python compatibility.
  vtkVector3d GetSegmentColor(std::string segmentID);
  /// Set segment color by segment ID. Convenience function for python compatibility.
  void SetSegmentColor(std::string segmentID, vtkVector3d color);

  /// Get segment poly data opacity by segment ID. Convenience function for python compatibility.
  double GetSegmentPolyDataOpacity(std::string segmentID);
  /// Set segment poly data opacity by segment ID. Convenience function for python compatibility.
  void SetSegmentPolyDataOpacity(std::string segmentID, double opacity);

protected:
  /// Decide which poly data representation to use for 3D display
  /// If master representation is a poly data then return master representation type.
  /// Otherwise return first poly data representation if any.
  /// Otherwise return closed surface representation (and then try to convert into it)
  const char* DeterminePolyDataDisplayRepresentationName();

  /// Set segment color in associated color table
  /// \return Success flag
  bool SetSegmentColorTableEntry(std::string segmentId, double r, double g, double b, double a);

protected:
  vtkMRMLSegmentationDisplayNode();
  virtual ~vtkMRMLSegmentationDisplayNode();
  vtkMRMLSegmentationDisplayNode(const vtkMRMLSegmentationDisplayNode&);
  void operator=(const vtkMRMLSegmentationDisplayNode&);

protected:
  /// Name of representation that is displayed as poly data
  /// (in the 3D view and in 2D views as slice intersection)
  char* PolyDataDisplayRepresentationName;

  /// List of segment display properties for all segments in associated segmentation.
  /// Maps segment identifier string (segment name by default) to properties.
  SegmentDisplayPropertiesMap SegmentationDisplayProperties;

  /// Flag determining whether transparency is allowed in the color table
  /// (thus the merged labelmap)
  bool EnableTransparencyInColorTable;
};

#endif
