/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Kevin Wang, RMP, PMH
  
==============================================================================*/

// Qt includes
#include <QCheckBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>

// SlicerQt includes
#include "qSlicerProtonDoseModuleWidget.h"
#include "ui_qSlicerProtonDoseModule.h"

// ProtonDose includes
#include "vtkSlicerProtonDoseModuleLogic.h"
#include "vtkMRMLProtonDoseNode.h"

// MRML includes
#include <vtkMRMLVolumeNode.h>
#include <vtkMRMLProtonBeamsNode.h>

// STD includes
#include <sstream>
#include <string>

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_ProtonDose
class qSlicerProtonDoseModuleWidgetPrivate: public Ui_qSlicerProtonDoseModule
{
  Q_DECLARE_PUBLIC(qSlicerProtonDoseModuleWidget);
protected:
  qSlicerProtonDoseModuleWidget* const q_ptr;
public:
  qSlicerProtonDoseModuleWidgetPrivate(qSlicerProtonDoseModuleWidget &object);
  ~qSlicerProtonDoseModuleWidgetPrivate();
  vtkSlicerProtonDoseModuleLogic* logic() const;
};

//-----------------------------------------------------------------------------
// qSlicerProtonDoseModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerProtonDoseModuleWidgetPrivate::qSlicerProtonDoseModuleWidgetPrivate(qSlicerProtonDoseModuleWidget& object)
  : q_ptr(&object)
{
}

//-----------------------------------------------------------------------------
qSlicerProtonDoseModuleWidgetPrivate::~qSlicerProtonDoseModuleWidgetPrivate()
{
}

//-----------------------------------------------------------------------------
vtkSlicerProtonDoseModuleLogic*
qSlicerProtonDoseModuleWidgetPrivate::logic() const
{
  Q_Q(const qSlicerProtonDoseModuleWidget);
  return vtkSlicerProtonDoseModuleLogic::SafeDownCast(q->logic());
} 

//-----------------------------------------------------------------------------
// qSlicerProtonDoseModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerProtonDoseModuleWidget::qSlicerProtonDoseModuleWidget(QWidget* _parent)
  : Superclass( _parent )
  , d_ptr( new qSlicerProtonDoseModuleWidgetPrivate(*this) )
{  
}

//-----------------------------------------------------------------------------
qSlicerProtonDoseModuleWidget::~qSlicerProtonDoseModuleWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerProtonDoseModuleWidget::setMRMLScene(vtkMRMLScene* scene)
{
  Q_D(qSlicerProtonDoseModuleWidget);

  printf ("setMRMLScene()\n");

  this->Superclass::setMRMLScene(scene);

  //qvtkReconnect( d->logic(), scene, vtkMRMLScene::EndImportEvent, this, SLOT(onSceneImportedEvent()) );

  // Find parameters node or create it if there is no one in the scene
  if (scene &&  d->logic()->GetProtonDoseNode() == 0)
  {
    vtkMRMLNode* node = scene->GetNthNodeByClass(0, "vtkMRMLProtonDoseNode");
    if (node)
    {
      this->setProtonDoseNode( vtkMRMLProtonDoseNode::SafeDownCast(node) );
    }
  }
}

//-----------------------------------------------------------------------------
void qSlicerProtonDoseModuleWidget::onSceneImportedEvent()
{
  this->onEnter();
}

//-----------------------------------------------------------------------------
void qSlicerProtonDoseModuleWidget::enter()
{
  printf ("enter()\n");
  this->onEnter();
  this->Superclass::enter();
}

//-----------------------------------------------------------------------------
void qSlicerProtonDoseModuleWidget::onEnter()
{
  printf ("onEnter()\n");
  if (!this->mrmlScene())
  {
    return;
  }

  Q_D(qSlicerProtonDoseModuleWidget);

  // First check the logic if it has a parameter node
  if (d->logic() == NULL)
  {
    return;
  }
  vtkMRMLProtonDoseNode* paramNode = d->logic()->GetProtonDoseNode();

  // If we have a parameter node select it
  if (paramNode == NULL)
  {
    vtkMRMLNode* node = this->mrmlScene()->GetNthNodeByClass(0, "vtkMRMLProtonDoseNode");
    if (node)
    {
      paramNode = vtkMRMLProtonDoseNode::SafeDownCast(node);
      d->logic()->SetAndObserveProtonDoseNode(paramNode);
      return;
    }
    else 
    {
      vtkSmartPointer<vtkMRMLProtonDoseNode> newNode = vtkSmartPointer<vtkMRMLProtonDoseNode>::New();
      this->mrmlScene()->AddNode(newNode);
      d->logic()->SetAndObserveProtonDoseNode(newNode);
    }
  }

  updateWidgetFromMRML();
  updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerProtonDoseModuleWidget::updateWidgetFromMRML()
{
  Q_D(qSlicerProtonDoseModuleWidget);
  printf ("UpdateWidgetFrom MRML()\n"); fflush (stdout);

  vtkMRMLProtonDoseNode* paramNode = d->logic()->GetProtonDoseNode();
  if (paramNode && this->mrmlScene())
  {
    d->MRMLNodeComboBox_ParameterSet->setCurrentNode(paramNode);
    printf ("Found a parameter node\n"); fflush (stdout);
#if defined (commentout)
    if (!SlicerRtCommon::IsStringNullOrEmpty(paramNode->GetDoseVolumeNodeId()))
    {
      d->MRMLNodeComboBox_DoseVolume->setCurrentNode(paramNode->GetDoseVolumeNodeId());
    }
    else
    {
      this->doseVolumeNodeChanged(d->MRMLNodeComboBox_DoseVolume->currentNode());
    }
#endif
  }
}

//-----------------------------------------------------------------------------
void qSlicerProtonDoseModuleWidget::onLogicModified()
{
  Q_D(qSlicerProtonDoseModuleWidget);

  updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerProtonDoseModuleWidget::setup()
{
  printf ("setup()\n");
  Q_D(qSlicerProtonDoseModuleWidget);

  d->setupUi(this);
  this->Superclass::setup();

  // Make connections
  connect (d->MRMLNodeComboBox_ParameterSet, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(setProtonDoseNode(vtkMRMLNode*)));
#if defined (commentout)
  connect (d->MRMLNodeComboBox_DoseVolume, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(doseVolumeNodeChanged(vtkMRMLNode*)));
#endif

  // signal - slot Beam Parameters - buttonClick
  connect (d->comboBox_Beam, SIGNAL(activated(int)), this, SLOT(beamChanged()));
  connect (d->pushButton_changeName, SIGNAL(clicked()), this, SLOT(beamNameChanged()));
  connect (d->pushButton_AddBeam, SIGNAL(clicked()), this, SLOT(addBeam()));
  connect (d->pushButton_DeleteBeam, SIGNAL(clicked()), this, SLOT(deleteBeam()));
  connect (d->pushButton_LoadBeams, SIGNAL(clicked()), this, SLOT(loadBeam()));
  connect (d->pushButton_SaveBeams, SIGNAL(clicked()), this, SLOT(saveBeam()));

  // signal slot isocenter, jaws and energy - when editing is finished
  connect (d->lineEdit_IsoX, SIGNAL(editingFinished()), this, SLOT(editIso_x()));
  connect (d->lineEdit_IsoY, SIGNAL(editingFinished()), this, SLOT(editIso_y()));
  connect (d->lineEdit_IsoZ, SIGNAL(editingFinished()), this, SLOT(editIso_z()));
  connect (d->lineEdit_Range, SIGNAL(editingFinished()), this, SLOT(editRange()));
  connect (d->lineEdit_Mod, SIGNAL(editingFinished()), this, SLOT(editMod()));
  connect (d->lineEdit_XJaw, SIGNAL(editingFinished()), this, SLOT(editXJaw()));
  connect (d->lineEdit_YJaw, SIGNAL(editingFinished()), this, SLOT(editYJaw()));

  // signal slot beam rotation and couch rotation geometry
  connect (d->doubleSpinBox_Gantry, SIGNAL(valueChanged(double)), this, SLOT(gantryChanged(double)));
  connect (d->doubleSpinBox_Collimator, SIGNAL(valueChanged(double)), this, SLOT(collimatorChanged(double)));
  connect (d->doubleSpinBox_CouchRotation, SIGNAL(valueChanged(double)), this, SLOT(couchRotationChanged(double)));
  connect (d->doubleSpinBox_CouchPitch, SIGNAL(valueChanged(double)), this, SLOT(couchPitchChanged(double)));
  connect (d->doubleSpinBox_CouchRoll, SIGNAL(valueChanged(double)), this, SLOT(couchRollChanged(double)));

  // other signal slot 
  connect (d->pushButton_Apply, SIGNAL(clicked()), this, SLOT(applyClicked()));

  // GCS: what is this?
  connect( d->MRMLNodeComboBox_OutputHierarchy, SIGNAL( currentNodeChanged(vtkMRMLNode*) ), this, SLOT( outputHierarchyNodeChanged(vtkMRMLNode*) ) );

  // Handle scene change event if occurs
  qvtkConnect( d->logic(), vtkCommand::ModifiedEvent, this, SLOT( onLogicModified() ) );

  updateButtonsState();

  // initialisation of the beam
  beam_max = 1;
  beam_actual = 0;

  /* GCS -- to do this requires having a copy contructor.
     Instead, let's follow Kevin's method of adding to mrml scene, 
     and linking them using a heirarchy. */
#if defined (commentout)
  vtkMRMLProtonBeamsNode beam_default;

  beam.push_back(beam_default);

  beam[beam_actual].accessBeamName("Beam 1");
  beamQtUpdate();
#endif
}

//-----------------------------------------------------------------------------
void qSlicerProtonDoseModuleWidget::setProtonDoseNode(vtkMRMLNode *node)
{
  Q_D(qSlicerProtonDoseModuleWidget);

  vtkMRMLProtonDoseNode* paramNode = vtkMRMLProtonDoseNode::SafeDownCast(node);

  // Each time the node is modified, the qt widgets are updated
  qvtkReconnect( d->logic()->GetProtonDoseNode(), paramNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromMRML()) );

  d->logic()->SetAndObserveProtonDoseNode(paramNode);
  updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerProtonDoseModuleWidget::doseVolumeNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerProtonDoseModuleWidget);

  vtkMRMLProtonDoseNode* paramNode = d->logic()->GetProtonDoseNode();
  if (!paramNode || !this->mrmlScene() || !node)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveDoseVolumeNodeId(node->GetID());
  paramNode->DisableModifiedEventOff();

  updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerProtonDoseModuleWidget::outputHierarchyNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerProtonDoseModuleWidget);

  vtkMRMLProtonDoseNode* paramNode = d->logic()->GetProtonDoseNode();
  if (!paramNode || !this->mrmlScene() || !node)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveOutputHierarchyNodeId(node->GetID());
  paramNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerProtonDoseModuleWidget::storeSelectedTableItemText(QTableWidgetItem* selectedItem, QTableWidgetItem* previousItem)
{
  Q_D(qSlicerProtonDoseModuleWidget);

}

// slot definition 
// Beam switch
void qSlicerProtonDoseModuleWidget::beamChanged()
{
	Q_D(qSlicerProtonDoseModuleWidget);
	beam_actual = d->comboBox_Beam->currentIndex();
	beamQtUpdate();
}

// beam name change
void qSlicerProtonDoseModuleWidget::beamNameChanged()
{
	Q_D(qSlicerProtonDoseModuleWidget);
	QString new_beam_name = QInputDialog::getText(this, "","Beam name:" , QLineEdit::Normal, d->comboBox_Beam->currentText());
	if (new_beam_name != "")
	{
		d->comboBox_Beam->setItemText(beam_actual,new_beam_name);
#if defined (commentout)
		beam[beam_actual].accessBeamName(new_beam_name.toStdString());
#endif
	}
}

// when a beam is added in the vector
void qSlicerProtonDoseModuleWidget::addBeam()
{
	std::stringstream ss;
	ss << beam_max+1;
	QString new_beam_name = QInputDialog::getText(this, "","New beam name:" , QLineEdit::Normal, QString::fromStdString("Beam "+ss.str()));
	if (new_beam_name !="")
	{
		beam_max++;
		beam_actual = beam_max-1;
		
#if defined (commentout)
		vtkMRMLProtonBeamsNode beam_default;
		beam.push_back(beam_default);
		beam[beam_actual].accessBeamName(new_beam_name.toStdString());

		Q_D(qSlicerProtonDoseModuleWidget);

		d->comboBox_Beam->addItem(QString::fromStdString(beam[beam_actual].readBeamName()));
		d->comboBox_Beam->setCurrentIndex(beam_actual);
		beamQtUpdate(); // the created beam appears on the main window
#endif
	}
}

// the actual beam is deleted
void qSlicerProtonDoseModuleWidget::deleteBeam()
{
#if defined (commentout)
  if (beam_max == 1) // if it is the last beam, then the operation is refused
  {
    QMessageBox::warning(this, "Warning", "Last beam - delete refused");
  }
  else
  {
    Q_D(qSlicerProtonDoseModuleWidget); // if the last beam is deleted then we deleted only this one by a pop-back
    if (beam_actual == beam_max -1)
    {
      beam_actual--;
      beam_max--;
      d->comboBox_Beam->setCurrentIndex(beam_actual);
      d->comboBox_Beam->removeItem(beam_actual+1);
				
      beam.pop_back();
      beamQtUpdate();
    }
    else // if it is not the last one, then all the beams afterward are shifted backward and the last is deleted
    {
      int i;
      for (i = beam_actual; i < beam_max - 1;i++)
      {
        beam[i].accessBeamName(beam[i+1].readBeamName());
        beam[i].accessIsocenter_x(beam[i+1].readIsocenter_x());
        beam[i].accessIsocenter_y(beam[i+1].readIsocenter_y());
        beam[i].accessIsocenter_z(beam[i+1].readIsocenter_z());
        beam[i].accessGantryAngle(beam[i+1].readGantryAngle());
        beam[i].accessCollimatorAngle(beam[i+1].readCollimatorAngle());
        beam[i].accessRange(beam[i+1].readRange());
        beam[i].accessMod(beam[i+1].readMod());
        beam[i].accessJaw_x(beam[i+1].readJaw_x());
        beam[i].accessJaw_y(beam[i+1].readJaw_y());
        beam[i].accessCouchRotation(beam[i+1].readCouchRotation());
        beam[i].accessCouchPitch(beam[i+1].readCouchPitch());
        beam[i].accessCouchRoll(beam[i+1].readCouchRoll());
				
        d->comboBox_Beam->setItemText(i,QString::fromStdString(beam[i].readBeamName()));
      }
      d->comboBox_Beam->setCurrentIndex(beam_actual);
      d->comboBox_Beam->removeItem(beam_max-1);
      beam_max--;
      beam.pop_back();
      beamQtUpdate();
    }
  }
#endif
}

// when a beam is loaded
void qSlicerProtonDoseModuleWidget::loadBeam()
{
  Q_D(qSlicerProtonDoseModuleWidget);

#if defined (commentout)
	
  QString loadPath = QFileDialog::getOpenFileName(this,"Load a beam","D:/Plastimatch/scratch","Text files(*.txt)");
  {
    std::string ss;
    ss =loadPath.toStdString();
    ifstream beam_store(ss.c_str());
    std::string first_line;
    getline(beam_store, first_line);
    if (first_line == "Beam save for SlicerRt") // test to know if the file is a slicerRt save file
    {
      char a='a';
      int beam_number;
      std::string beamName;
      double data;
      Q_D(qSlicerProtonDoseModuleWidget);

      vtkMRMLProtonBeamsNode beam_default;
			
      beam_actual = 0;
      d->comboBox_Beam->setCurrentIndex(beam_actual);

      for (int j = 0; j < beam_max-1; j++) // all the beam we were working on are lost to use the one to be loaded
      {
        d->comboBox_Beam->removeItem(beam_max-j-1);
        beam.pop_back();
      }
			
      while (a != ':') // we read how many the is to be loaded
      {
        beam_store.get(a);
      }
      beam_store >> beam_number;
      beam_max = beam_number;
			
      for (int i=0; i < beam_number; i++) // and we load the beam data, beam per beam
      {
        if (i !=0)
        {
          beam.push_back(beam_default);
          d->comboBox_Beam->addItem(QString::fromStdString(beam[i].readBeamName()));
        }
        a = 'a';			
        while (a != ':')
        {
          beam_store.get(a);
        }
        getline(beam_store, beamName);
        beam[i].accessBeamName(beamName);
        d->comboBox_Beam->setItemText(i,QString::fromStdString(beam[i].readBeamName()));
        a='a';
        while (a != ':')
        {
          beam_store.get(a);
        }	
        beam_store >> data;
        beam[i].accessIsocenter_x(data);
        beam_store >> data;
        beam[i].accessIsocenter_y(data);
        beam_store >> data;
        beam[i].accessIsocenter_z(data);
        a='a';
        while (a != ':')
        {
          beam_store.get(a);
        }	
        beam_store >> data;
        beam[i].accessGantryAngle(data);
        beam_store >> data;
        beam[i].accessRange(data);
        beam_store >> data;
        beam[i].accessMod(data);
        a='a';
        while (a != ':')
        {
          beam_store.get(a);
        }	
        beam_store >> data;
        beam[i].accessCollimatorAngle(data);
        beam_store >> data;
        beam[i].accessJaw_x(data);
        beam_store >> data;
        beam[i].accessJaw_y(data);
        a='a';
        while (a != ':')
        {
          beam_store.get(a);
        }
        beam_store >> data;
        beam[i].accessCouchRotation(data);
        beam_store >> data;
        beam[i].accessCouchPitch(data);
        beam_store >> data;
        beam[i].accessCouchRoll(data);
      }	
      beamQtUpdate(); // the main window is updated with the last beam
    }
    else // if the file is not a slicerRt save file, then the loading is aborted
    {
      QMessageBox::warning(this, "Warning", "File loaded corrupted - load impossible");
    }
  }
#endif
}

void qSlicerProtonDoseModuleWidget::saveBeam() // save the beam data
{
  QString savePath = QFileDialog::getSaveFileName(this, "Save a beam","D:/Plastimatch/scratch", "Text files(*.txt)");
  std::string ss = savePath.toStdString();
  ofstream beam_store(ss.c_str());
  beam_store << "Beam save for SlicerRt" << endl;
  beam_store << "beam_number: " << beam_max << endl << endl;
  for (int i = 0; i < beam_max; i++) // process for each beam - writing in an extern file
  {
#if defined (commentout)
    beam_store << endl << "beam[" << i << "]" << endl;
    beam_store << "beam name:" << beam[i].readBeamName() << endl;
    beam_store << "iso (x,y,z):" << beam[i].readIsocenter_x() << " " << beam[i].readIsocenter_y() << " " << beam[i].readIsocenter_z() << endl;
    beam_store << "gantry (angle, range, mod):" << beam[i].readGantryAngle() << " " << beam[i].readRange() << " " << beam[i].readMod() << endl;
    beam_store << "collimator (angle, jaw x, jaw y):" << beam[i].readCollimatorAngle() << " " << beam[i].readJaw_x() << " " << beam[i].readJaw_y()<< endl;
    beam_store << "gantry (rotation, pitch, roll):" << beam[i].readCouchRotation()<< " " << beam[i].readCouchPitch() << " " << beam[i].readCouchRoll() << endl;
#endif
  }
}

void qSlicerProtonDoseModuleWidget::editIso_x() // update the isocenter data only when a number is edited in the editline
{
  Q_D(qSlicerProtonDoseModuleWidget);
	
  QString str = d->lineEdit_IsoX->text();

  std::string stringtest;
  int stringlength = 0;
  int test = 1;

  stringtest = str.toStdString();
  stringlength = stringtest.size();

#if defined (commentout)

  if (stringlength == 0) // if the line is void then 0 is automatically introduced
  {
    d->lineEdit_IsoX->setText("0");
    beam[beam_actual].accessIsocenter_x(0);
  }
  else
  {
    for (int i = 0; i< stringlength ; i++) // in this situation a number is edited
    {
      if (!isdigit(stringtest[i]) && stringtest[i] !='.' && stringtest[i] !='-')
      {
        test = 0;
      }
    }
  }
  if (test)
  {
    beam[beam_actual].accessIsocenter_x(str.toDouble());
  }
  else // if the string introduced is not a number, then it reads the previous value for the parameter
  {
    d->lineEdit_IsoX->setText(QString::number(beam[beam_actual].readIsocenter_x()));
  }
#endif
}

void qSlicerProtonDoseModuleWidget::editIso_y() // idem iso_x
{
  Q_D(qSlicerProtonDoseModuleWidget);
	
  QString str = d->lineEdit_IsoY->text();

  std::string stringtest;
  int stringlength = 0;
  int test = 1;

  stringtest = str.toStdString();
  stringlength = stringtest.size();

#if defined (commentout)
  if (stringlength == 0)
  {
    d->lineEdit_IsoY->setText("0");
    beam[beam_actual].accessIsocenter_y(0);
  }
  else
  {
    for (int i = 0; i< stringlength ; i++)
    {
      if (!isdigit(stringtest[i]) && stringtest[i] !='.' && stringtest[i] !='-')
      {
        test = 0;
      }
    }
  }
  if (test)
  {
    beam[beam_actual].accessIsocenter_y(str.toDouble());
  }
  else
  {
    d->lineEdit_IsoY->setText(QString::number(beam[beam_actual].readIsocenter_y()));
  }
#endif
}

void qSlicerProtonDoseModuleWidget::editIso_z() // idem iso_x
{
  Q_D(qSlicerProtonDoseModuleWidget);
	
  QString str = d->lineEdit_IsoZ->text();

  std::string stringtest;
  int stringlength = 0;
  int test = 1;

  stringtest = str.toStdString();
  stringlength = stringtest.size();

#if defined (commentout)
  if (stringlength == 0)
  {
    d->lineEdit_IsoZ->setText("0");
    beam[beam_actual].accessIsocenter_z(0);
  }
  else
  {
    for (int i = 0; i< stringlength ; i++)
    {
      if (!isdigit(stringtest[i]) && stringtest[i] !='.' && stringtest[i] !='-')
      {
        test = 0;
      }
    }
  }
  if (test)
  {
    beam[beam_actual].accessIsocenter_z(str.toDouble());
  }
  else
  {
    d->lineEdit_IsoZ->setText(QString::number(beam[beam_actual].readIsocenter_z()));
  }
#endif
}

void qSlicerProtonDoseModuleWidget::gantryChanged(double value)
{
  Q_D(qSlicerProtonDoseModuleWidget);

#if defined (commentout)
  printf ("Apparently the gantry angle changed (1)\n");
  beam[beam_actual].accessGantryAngle(value);

  vtkMRMLProtonDoseNode* paramNode = d->logic()->GetProtonDoseNode();
  if (!paramNode || !this->mrmlScene())
  {
    return;
  }

  printf ("Apparently we have a mrmlScene and a paramNode\n");

  paramNode->DisableModifiedEventOn();
  paramNode->SetGantryAngle(value);
  paramNode->DisableModifiedEventOff();
#endif
}

void qSlicerProtonDoseModuleWidget::editRange() // idem iso_x but for the range
{
  Q_D(qSlicerProtonDoseModuleWidget);
	
  QString str = d->lineEdit_Range->text();

  std::string stringtest;
  int stringlength = 0;
  int test = 1;

#if defined (commentout)
  stringtest = str.toStdString();
  stringlength = stringtest.size();

  if (stringlength == 0)
  {
    d->lineEdit_Range->setText("0");
    beam[beam_actual].accessRange(0);
  }
  else
  {
    for (int i = 0; i< stringlength ; i++)
    {
      if (!isdigit(stringtest[i]) && stringtest[i] !='.')
      {
        test = 0;
      }
    }
  }
  if (test)
  {
    if (str.toDouble() >= beam[beam_actual].readMod()) // we check that the Mod is not superior to the Range
    {
      beam[beam_actual].accessRange(str.toDouble());
    }
    else
    {
      beam[beam_actual].accessRange(str.toDouble());
      QMessageBox::warning(this,"", "Warning, Mod must be included between 0 and Range - Range set to 0");
      d->lineEdit_Mod->setText("0");
      beam[beam_actual].accessMod(0);
    }
  }
  else
  {
    d->lineEdit_Range->setText(QString::number(beam[beam_actual].readRange()));
  }
#endif
}

void qSlicerProtonDoseModuleWidget::editMod() // idem iso_x but for mod
{
  Q_D(qSlicerProtonDoseModuleWidget);
	
  QString str = d->lineEdit_Mod->text();

  std::string stringtest;
  int stringlength = 0;
  int test = 1;

  stringtest = str.toStdString();
  stringlength = stringtest.size();

#if defined (commentout)
  if (stringlength == 0)
  {
    d->lineEdit_Mod->setText("0");
    beam[beam_actual].accessMod(0);
  }
  else
  {
    for (int i = 0; i< stringlength ; i++)
    {
      if (!isdigit(stringtest[i]) && stringtest[i] !='.')
      {
        test = 0;
      }
    }
  }
  if (test)
  {
    if (str.toDouble() <= beam[beam_actual].readRange()) // we check that the mod is not inferior to the range
    {
      beam[beam_actual].accessMod(str.toDouble());
    }
    else
    {
      QMessageBox::warning(this,"", "Warning, Mod must be included between 0 and Range - Range set to 0");
      d->lineEdit_Mod->setText("0");
      beam[beam_actual].accessMod(0);
    }
  }
  else
  {
    d->lineEdit_Mod->setText(QString::number(beam[beam_actual].readMod()));
  }
#endif
}

void qSlicerProtonDoseModuleWidget::collimatorChanged(double value) // when the collimator angle is changed
{
#if defined (commentout)
  beam[beam_actual].accessCollimatorAngle(value);
#endif
}

void qSlicerProtonDoseModuleWidget::editXJaw() // idem iso_x, but in the case of a string that is not a number, the editline is reset to 1
{
  Q_D(qSlicerProtonDoseModuleWidget);
	
  QString str = d->lineEdit_XJaw->text();

  std::string stringtest;
  int stringlength = 0;
  int test = 1;

  stringtest = str.toStdString();
  stringlength = stringtest.size();

#if defined (commentout)
  if (stringlength == 0)
  {
    d->lineEdit_XJaw->setText("1");
    beam[beam_actual].accessJaw_x(1);
  }
  else
  {
    for (int i = 0; i< stringlength ; i++)
    {
      if (!isdigit(stringtest[i]) && stringtest[i] !='.')
      {
        test = 0;
      }
    }
  }
  if (test)
  {
    if (str.toDouble() >= 1)
    {
      beam[beam_actual].accessJaw_x(str.toDouble());
    }
    else
    {
      beam[beam_actual].accessJaw_x(1);
      d->lineEdit_XJaw->setText(QString::number(beam[beam_actual].readJaw_x()));
    }
  }
  else
  {
    d->lineEdit_XJaw->setText(QString::number(beam[beam_actual].readJaw_x()));
  }
#endif
}

void qSlicerProtonDoseModuleWidget::editYJaw() // idem editXJaw
{
  Q_D(qSlicerProtonDoseModuleWidget);
	
  QString str = d->lineEdit_YJaw->text();

  std::string stringtest;
  int stringlength = 0;
  int test = 1;

  stringtest = str.toStdString();
  stringlength = stringtest.size();

#if defined (commentout)
  if (stringlength == 0)
  {
    d->lineEdit_YJaw->setText("1");
    beam[beam_actual].accessJaw_y(1);
  }
  else
  {
    for (int i = 0; i< stringlength ; i++)
    {
      if (!isdigit(stringtest[i]) && stringtest[i] !='.')
      {
        test = 0;
      }
    }
  }
  if (test)
  {
    if (str.toDouble() >= 1)
    {
      beam[beam_actual].accessJaw_y(str.toDouble());
    }
    else
    {
      beam[beam_actual].accessJaw_y(1);
      d->lineEdit_YJaw->setText(QString::number(beam[beam_actual].readJaw_y()));
    }
  }
  else
  {
    d->lineEdit_YJaw->setText(QString::number(beam[beam_actual].readJaw_y()));
  }
#endif
}

void qSlicerProtonDoseModuleWidget::couchRotationChanged(double value) // idem collimator angl but for couch rotation
{
#if defined (commentout)
  beam[beam_actual].accessCouchRotation(value);
#endif
}

void qSlicerProtonDoseModuleWidget::couchPitchChanged(double value) // idem couch rotation
{
#if defined (commentout)
  beam[beam_actual].accessCouchPitch(value);
#endif
}

void qSlicerProtonDoseModuleWidget::couchRollChanged(double value) // idem couch rotation
{
#if defined (commentout)
  beam[beam_actual].accessCouchRoll(value);
#endif
}

void qSlicerProtonDoseModuleWidget::beamQtUpdate() // this function update the actual beam parameters on the window reading in the vector<beam>
{
#if defined (commentout)
  Q_D(qSlicerProtonDoseModuleWidget);
  d->comboBox_Beam->setItemText(beam_actual,QString::fromStdString(beam[beam_actual].readBeamName()));
  d->lineEdit_IsoX->setText(QString::number(beam[beam_actual].readIsocenter_x()));
  d->lineEdit_IsoY->setText(QString::number(beam[beam_actual].readIsocenter_y()));
  d->lineEdit_IsoZ->setText(QString::number(beam[beam_actual].readIsocenter_z()));
  d->doubleSpinBox_Gantry->setValue(beam[beam_actual].readGantryAngle());
  d->doubleSpinBox_Collimator->setValue(beam[beam_actual].readCollimatorAngle());
  d->lineEdit_Range->setText(QString::number(beam[beam_actual].readRange()));
  d->lineEdit_Mod->setText(QString::number(beam[beam_actual].readMod()));
  d->lineEdit_XJaw->setText(QString::number(beam[beam_actual].readJaw_x()));
  d->lineEdit_YJaw->setText(QString::number(beam[beam_actual].readJaw_y()));
  d->doubleSpinBox_CouchRotation->setValue(beam[beam_actual].readCouchRotation());
  d->doubleSpinBox_CouchPitch->setValue(beam[beam_actual].readCouchPitch());
  d->doubleSpinBox_CouchRoll->setValue(beam[beam_actual].readCouchRoll());
#endif
}

void qSlicerProtonDoseModuleWidget::applyClicked() // Greg!!! Here is where you have to compute the dose
{
  configFileCreation();
		
  Q_D(qSlicerProtonDoseModuleWidget);

  QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

  // Compute the iso dose surface for the selected dose volume
  //d->logic()->ComputeProtonDose();								<------------------ ComputeProtonDose, found in logic directory ( I made another mark in the vtkSlicerProtonDoseModuleLogic file)

  //QApplication::restoreOverrideCursor();
}

//-----------------------------------------------------------------------------
void qSlicerProtonDoseModuleWidget::updateButtonsState()
{
  Q_D(qSlicerProtonDoseModuleWidget);

  vtkMRMLProtonDoseNode* paramNode = d->logic()->GetProtonDoseNode();
  bool applyEnabled = paramNode 
    && paramNode->GetDoseVolumeNodeId();
  applyEnabled = true;
  printf ("Setting apply button state to %d\n", applyEnabled); fflush(stdout);
  d->pushButton_Apply->setEnabled(applyEnabled);
}

void qSlicerProtonDoseModuleWidget::configFileCreation() // Creation of the configuration proton file (Energy, collimator, roll & pitch are yet not working...) A default flat beam between 80 -> 120 MeV is created to be used by Plastimatch
{
#if defined (commentout)
  ofstream config_file ("D:/Plastimatch/scratch/beam.txt");
  config_file << "# proton dose configuration file" << endl << endl << "# flavor: algorithm selection" << endl <<"# threading: single, openmp, cuda" << endl << "# patient: input patient CT volume" << endl << "# dose: output dose volume" << endl << "# detail: low (no scatter/interplay) or high" << endl << endl;

  config_file << "[SETTINGS]" << endl << "flavor=a" << endl << "threading=single" << endl << "patient=lung.mha" << "dose=dose.mha" << "detail=low" << endl << endl;

  config_file <<"# center: center of aperture (mm)" << endl <<"# offset: distance from beam nozzle (mm)" << "# resolution: size of aperture/# of pencil beams" << endl;

  config_file << "[APERTURE]" << endl << "center=" << (beam[beam_actual].readJaw_x()-1)/2 << " " << (beam[beam_actual].readJaw_y()-1)/2 << endl << "offset=9287" << endl << "resolution=" << beam[beam_actual].readJaw_x() << " " << beam[beam_actual].readJaw_y() << endl << endl;

  beam[beam_actual].calculateSourcePosition();

  config_file << "# bragg_curve: file containing bragg curve" << endl << "# pos: position of nozzle" << endl << "# if bragg_curve is not specified, we generate" << endl << "# a synthetic bragg curve using:" << endl << "# depth : maximum water equiv. depth of bragg curve" << endl << "# res   : spatial resolution of bragg curve (cm)" << endl;

  config_file <<"[BEAM]" << endl << "pos=" << beam[beam_actual].readSourceX() << " " << beam[beam_actual].readSourceY() << " " << beam[beam_actual].readSourceZ() << endl << "isocenter=" << beam[beam_actual].readIsocenter_x() << " " << beam[beam_actual].readIsocenter_y() << " " << beam[beam_actual].readIsocenter_z() << endl << "depth=800" << endl <<"res=1" <<endl << endl;

  config_file << "[PEAK]" << endl << "energy=81.000000" << endl << "spread=50.000000" << endl << "weight=0.015750" << endl << endl << "[PEAK]" << endl << "energy=82.000000" << endl << "spread=50.000000" << endl << "weight=0.012891" << endl << endl << "[PEAK]" << endl << "energy=83.000000" << endl << "spread=50.000000" << endl << "weight=0.010377" << endl << endl << "[PEAK]" << endl << "energy=84.000000" << endl << "spread=50.000000" << endl << "weight=0.009923" << endl << endl << "[PEAK]" << endl << "energy=85.000000" << endl << "spread=50.000000" << endl << "weight=0.010222" << endl << endl << "[PEAK]" << endl << "energy=86.000000" << endl << "spread=50.000000" << endl << "weight=0.009924" << endl << endl << "[PEAK]" << endl << "energy=87.000000" << endl << "spread=50.000000" << endl << "weight=0.019520" << endl << endl << "[PEAK]" << endl << "energy=88.000000" << endl << "spread=50.000000" << endl << "weight=0.009921" << endl << endl << "[PEAK]" << endl << "energy=89.000000" << endl << "spread=50.000000" << endl << "weight=0.012375" << endl << endl << "[PEAK]" << endl << "energy=90.000000" << endl << "spread=50.000000" << endl << "weight=0.012264" << endl << endl << "[PEAK]" << endl << "energy=91.000000" << endl << "spread=50.000000" << endl << "weight=0.011746" << endl << endl << "[PEAK]" << endl << "energy=92.000000" << endl << "spread=50.000000" << endl << "weight=0.011886" << endl << endl << "[PEAK]" << endl << "energy=92.000000" << endl << "spread=50.000000" << endl << "weight=0.012891" << endl << endl << "[PEAK]" << endl << "energy=93.000000" << endl << "spread=50.000000" << endl << "weight=0.011350" << endl << endl << "[PEAK]" << endl << "energy=94.000000" << endl << "spread=50.000000" << endl << "weight=0.014945" << endl << endl << "[PEAK]" << endl << "energy=95.000000" << endl << "spread=50.000000" << endl << "weight=0.015619" << endl << endl << "[PEAK]" << endl << "energy=96.000000" << endl << "spread=50.000000" << endl << "weight=0.013529" << endl << endl << "[PEAK]" << endl << "energy=97.000000" << endl << "spread=50.000000" << endl << "weight=0.011389" << endl << endl << "[PEAK]" << endl << "energy=98.000000" << endl << "spread=50.000000" << endl << "weight=0.013359" << endl << endl << "[PEAK]" << endl << "energy=99.000000" << endl << "spread=50.000000" << endl << "weight=0.023084" << endl << endl << "[PEAK]" << endl << "energy=100.000000" << endl << "spread=50.000000" << endl << "weight=0.016826" << endl << endl << "[PEAK]" << endl << "energy=101.000000" << endl << "spread=50.000000" << endl << "weight=0.018758" << endl << endl << "[PEAK]" << endl << "energy=102.000000" << endl << "spread=50.000000" << endl << "weight=0.024458" << endl << endl << "[PEAK]" << endl << "energy=103.000000" << endl << "spread=50.000000" << endl << "weight=0.022037" << endl << endl << "[PEAK]" << endl << "energy=104.000000" << endl << "spread=50.000000" << endl << "weight=0.018894" << endl << endl << "[PEAK]" << endl << "energy=105.000000" << endl << "spread=50.000000" << endl << "weight=0.010599" << endl << endl << "[PEAK]" << endl << "energy=106.000000" << endl << "spread=50.000000" << endl << "weight=0.029428" << endl << endl << "[PEAK]" << endl << "energy=107.000000" << endl << "spread=50.000000" << endl << "weight=0.031212" << endl << endl << "[PEAK]" << endl << "energy=108.000000" << endl << "spread=50.000000" << endl << "weight=0.027284" << endl << endl << "[PEAK]" << endl << "energy=109.000000" << endl << "spread=50.000000" << endl << "weight=0.020866" << endl << endl << "[PEAK]" << endl << "energy=110.000000" << endl << "spread=50.000000" << endl << "weight=0.029228" << endl << endl << "[PEAK]" << endl << "energy=111.000000" << endl << "spread=50.000000" << endl << "weight=0.043610" << endl << endl << "[PEAK]" << endl << "energy=112.000000" << endl << "spread=50.000000" << endl << "weight=0.031641" << endl << endl << "[PEAK]" << endl << "energy=113.000000" << endl << "spread=50.000000" << endl << "weight=0.033131" << endl << endl << "[PEAK]" << endl << "energy=114.000000" << endl << "spread=50.000000" << endl << "weight=0.050920" << endl << endl << "[PEAK]" << endl << "energy=115.000000" << endl << "spread=50.000000" << endl << "weight=0.039499" << endl << endl << "[PEAK]" << endl << "energy=116.000000" << endl << "spread=50.000000" << endl << "weight=0.074600" << endl << endl << "[PEAK]" << endl << "energy=117.000000" << endl << "spread=50.000000" << endl << "weight=0.041772" << endl << endl << "[PEAK]" << endl << "energy=118.000000" << endl << "spread=50.000000" << endl << "weight=0.091955" << endl << endl << "[PEAK]" << endl << "energy=119.000000" << endl << "spread=50.000000" << endl << "weight=0.168672" << endl << endl << "[PEAK]" << endl << "energy=120.000000" << endl << "spread=50.000000" << endl << "weight=0.199063" << endl;
#endif
}
