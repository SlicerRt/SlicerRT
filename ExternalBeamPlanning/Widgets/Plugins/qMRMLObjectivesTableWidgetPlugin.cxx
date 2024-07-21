#ifndef __qMRMLObjectivesTableWidgetPlugin_h
#define __qMRMLObjectivesTableWidgetPlugin_h

#include <QtUiPlugin/QDesignerCustomWidgetInterface>
#include "qMRMLObjectivesTableWidget.h" // Include your widget's header

class qMRMLObjectivesTableWidgetPlugin : public QObject, public QDesignerCustomWidgetInterface
{
    Q_OBJECT
    Q_INTERFACES(QDesignerCustomWidgetInterface)
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QDesignerCustomWidgetInterface")
#endif

public:
    qMRMLObjectivesTableWidgetPlugin(QObject *parent = nullptr) : QObject(parent) {}

    QWidget *createWidget(QWidget *parent) override {
        return new qMRMLObjectivesTableWidget(parent);
    }

    QString name() const override {
        return "qMRMLObjectivesTableWidget";
    }

    QString group() const override {
        return "Slicer [Your Module Name] Widgets";
    }

    QIcon icon() const override {
        // Return an icon for your widget if you have one
        return QIcon();
    }

    QString toolTip() const override {
        return "A table widget for displaying objectives";
    }

    QString whatsThis() const override {
        return "This widget is used to display and interact with objectives in the [Your Module Name] module.";
    }

    bool isContainer() const override {
        return false; // Change to true if your widget can contain other widgets
    }

    QString includeFile() const override {
        return "qMRMLObjectivesTableWidget.h"; // The header file for your widget
    }

    QString domXml() const override {
        return "<widget class=\"qMRMLObjectivesTableWidget\" name=\"objectivesTableWidget\">\n"
               "</widget>\n";
    }
};

#endif // __qMRMLObjectivesTableWidgetPlugin_h