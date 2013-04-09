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

  This file was originally developed by Csaba Pinter, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

#include "qMRMLPotentialPatientHierarchyListViewPlugin.h"
#include "qMRMLPotentialPatientHierarchyListView.h"

//-----------------------------------------------------------------------------
qMRMLPotentialPatientHierarchyListViewPlugin::qMRMLPotentialPatientHierarchyListViewPlugin(QObject* pluginParent)
  : QObject(pluginParent)
{
}

//-----------------------------------------------------------------------------
QWidget *qMRMLPotentialPatientHierarchyListViewPlugin::createWidget(QWidget* parentWidget)
{
  qMRMLPotentialPatientHierarchyListView* pluginWidget
    = new qMRMLPotentialPatientHierarchyListView(parentWidget);
  return pluginWidget;
}

//-----------------------------------------------------------------------------
QString qMRMLPotentialPatientHierarchyListViewPlugin::domXml() const
{
  return "<widget class=\"qMRMLPotentialPatientHierarchyListView\" \
          name=\"PotentialPatientHierarchyListView\">\n"
          "</widget>\n";
}

//-----------------------------------------------------------------------------
QString qMRMLPotentialPatientHierarchyListViewPlugin::includeFile() const
{
  return "qMRMLPotentialPatientHierarchyListView.h";
}

//-----------------------------------------------------------------------------
bool qMRMLPotentialPatientHierarchyListViewPlugin::isContainer() const
{
  return false;
}

//-----------------------------------------------------------------------------
QString qMRMLPotentialPatientHierarchyListViewPlugin::name() const
{
  return "qMRMLPotentialPatientHierarchyListView";
}
