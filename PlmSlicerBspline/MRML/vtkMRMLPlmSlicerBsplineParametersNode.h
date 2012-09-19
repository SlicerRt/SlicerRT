/* -----------------------------------------------------------------------
   See COPYRIGHT.TXT and LICENSE.TXT for copyright and license information
   ----------------------------------------------------------------------- */
#ifndef __vtkMRMLPlmSlicerBsplineParametersNode_h
#define __vtkMRMLPlmSlicerBsplineParametersNode_h

#include "vtkMRML.h"
#include "vtkMRMLScene.h"
#include "vtkMRMLNode.h"
#include "vtkSlicerModuleMRMLExport.h"

class vtkMRMLAnnotationROINode;
class vtkMRMLVolumeNode;

/// \ingroup Slicer_QtModules_PlmSlicerBspline
class VTK_SLICER_PLMSLICERBSPLINE_MODULE_MRML_EXPORT vtkMRMLPlmSlicerBsplineParametersNode : public vtkMRMLNode
{     
  public:   

  static vtkMRMLPlmSlicerBsplineParametersNode *New();
  vtkTypeMacro(vtkMRMLPlmSlicerBsplineParametersNode,vtkMRMLNode);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual vtkMRMLNode* CreateNodeInstance();

  // Description:
  // Set node attributes
  virtual void ReadXMLAttributes( const char** atts);

  // Description:
  // Write this node's information to a MRML file in XML format.
  virtual void WriteXML(ostream& of, int indent);

  // Description:
  // Copy the node's attributes to this object
  virtual void Copy(vtkMRMLNode *node);

  // Description:
  // Get node XML tag name (like Volume, Model)
  virtual const char* GetNodeTagName() {return "PlmSlicerBsplineParameters";};

  // Description:
  // Update the stored reference to another node in the scene
  virtual void UpdateReferenceID(const char *oldID, const char *newID);

  // Description:
  // Updates this node if it depends on other nodes
  // when the node is deleted in the scene
  virtual void UpdateReferences();

  // Description:
  virtual void UpdateScene(vtkMRMLScene *scene);

  virtual void ProcessMRMLEvents ( vtkObject *caller, unsigned long event, void *callData);

  // Description:
#if 0
  vtkGetStringMacro (InputVolumeNodeID);
  void SetAndObserveInputVolumeNodeID(const char *volumeNodeID);
  vtkMRMLVolumeNode* GetInputVolumeNode();
  
  vtkGetStringMacro (OutputVolumeNodeID);
  void SetAndObserveOutputVolumeNodeID(const char *volumeNodeID);
  vtkMRMLVolumeNode* GetOutputVolumeNode();
#endif

  /* Volume MRML Combo-boxes */
  vtkSetStringMacro(FixedVolumeNodeID);
  vtkSetStringMacro(MovingVolumeNodeID);
  vtkSetStringMacro(WarpedVolumeNodeID);
  vtkSetStringMacro(XformVolumeNodeID);

  vtkGetStringMacro(FixedVolumeNodeID);
  vtkGetStringMacro(MovingVolumeNodeID);
  vtkGetStringMacro(WarpedVolumeNodeID);
  vtkGetStringMacro(XformVolumeNodeID);

  void SetAndObserveFixedVolumeNodeID(const char *volumeNodeID);
  void SetAndObserveMovingVolumeNodeID(const char *volumeNodeID);
  void SetAndObserveWarpedVolumeNodeID(const char *volumeNodeID);
  void SetAndObserveXformVolumeNodeID(const char *volumeNodeID);

  vtkMRMLVolumeNode* GetFixedVolumeNode();
  vtkMRMLVolumeNode* GetMovingVolumeNode();
  vtkMRMLVolumeNode* GetWarpedVolumeNode();
  vtkMRMLVolumeNode* GetXformVolumeNode();

  /* Simple QT widgets */
  vtkSetMacro(UseMSE, bool);
  vtkGetMacro(UseMSE, bool);
  vtkBooleanMacro(UseMSE, bool);

  vtkSetMacro(UseMI, bool);
  vtkGetMacro(UseMI, bool);
  vtkBooleanMacro(UseMI, bool);

  /* Control Grid Dimensions */
  vtkSetMacro(GridX, int);
  vtkSetMacro(GridY, int);
  vtkSetMacro(GridZ, int);

  vtkGetMacro(GridX, int);
  vtkGetMacro(GridY, int);
  vtkGetMacro(GridZ, int);

protected:
  vtkMRMLPlmSlicerBsplineParametersNode();
  ~vtkMRMLPlmSlicerBsplineParametersNode();

  vtkMRMLPlmSlicerBsplineParametersNode(const vtkMRMLPlmSlicerBsplineParametersNode&);
  void operator=(const vtkMRMLPlmSlicerBsplineParametersNode&);

  char *FixedVolumeNodeID;
  char *MovingVolumeNodeID;
  char *WarpedVolumeNodeID;
  char *XformVolumeNodeID;

  vtkMRMLVolumeNode* FixedVolumeNode;
  vtkMRMLVolumeNode* MovingVolumeNode;
  vtkMRMLVolumeNode* WarpedVolumeNode;
  vtkMRMLVolumeNode* XformVolumeNode;

  bool UseMSE;
  bool UseMI;

  double GridX;
  double GridY;
  double GridZ;

};

#endif

