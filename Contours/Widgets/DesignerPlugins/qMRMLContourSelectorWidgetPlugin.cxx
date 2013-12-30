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

#include "qMRMLContourSelectorWidgetPlugin.h"
#include "qMRMLContourSelectorWidget.h"

//-----------------------------------------------------------------------------
qMRMLContourSelectorWidgetPlugin::qMRMLContourSelectorWidgetPlugin(QObject* pluginParent)
  : QObject(pluginParent)
{
}

//-----------------------------------------------------------------------------
QWidget *qMRMLContourSelectorWidgetPlugin::createWidget(QWidget* parentWidget)
{
  qMRMLContourSelectorWidget* pluginWidget =
    new qMRMLContourSelectorWidget(parentWidget);
  return pluginWidget;
}

//-----------------------------------------------------------------------------
QString qMRMLContourSelectorWidgetPlugin::domXml() const
{
  return "<widget class=\"qMRMLContourSelectorWidget\" \
          name=\"ContourSelectorWidget\">\n"
          "</widget>\n";
}

//-----------------------------------------------------------------------------
QString qMRMLContourSelectorWidgetPlugin::includeFile() const
{
  return "qMRMLContourSelectorWidget.h";
}

//-----------------------------------------------------------------------------
bool qMRMLContourSelectorWidgetPlugin::isContainer() const
{
  return false;
}

//-----------------------------------------------------------------------------
QString qMRMLContourSelectorWidgetPlugin::name() const
{
  return "qMRMLContourSelectorWidget";
}
