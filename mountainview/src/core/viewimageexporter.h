#ifndef VIEWIMAGEEXPORTER_H
#define VIEWIMAGEEXPORTER_H

#include <QObject>
#include <mvabstractview.h>
class QDialog;

class ViewImageExporter : public QObject
{
    Q_OBJECT
public:
    ViewImageExporter(QObject *parent = 0);

    virtual QDialog* createViewExportDialog(MVAbstractView *view, QWidget *parent = 0);
    virtual bool simpleExport(MVAbstractView *view);
};

#endif // VIEWIMAGEEXPORTER_H
