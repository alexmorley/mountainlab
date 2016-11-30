#include "viewimageexporter.h"
#include <QDialog>
#include <QDialogButtonBox>
#include <QPainter>
#include <QFileDialog>
#include <QPdfWriter>
#include <QScrollArea>
#include <QSpinBox>
#include <QToolBar>
#include <QVBoxLayout>
#include "../ui_exportpreviewwidget.h"

#include <qttreepropertybrowser.h>
#include <qtpropertymanager.h>
#include <qteditorfactory.h>
#include <qtvariantproperty.h>
#include "viewpropertyeditor.h"

//class StrokePropertyManager : public QtAbstractPropertyManager {
//public:
//    StrokePropertyManager(QObject *parent = 0) : QtAbstractPropertyManager(parent) {
//        m_intPropertyManager = new QtIntPropertyManager(this);
////        intFactory->addPropertyManager(m_intPropertyManager);
//    }
//    bool isIntProp(QtProperty *property) const {
//        return m_intProps.contains(property);
//    }
//    QtIntPropertyManager *intManager() const {
//        return m_intPropertyManager;
//    }
//protected:
//    bool hasValue(const QtProperty *property) const {
//        return true;
//    }
//    QString valueText(const QtProperty *property) const {
//        return "Stroke";
//    }
//    QIcon valueIcon(const QtProperty *property) const {
//        return QIcon();
//    }
//    void initializeProperty(QtProperty *property){
//        QtProperty *widthProp = m_intPropertyManager->addProperty("width");
//        m_intPropertyManager->setValue(widthProp, 2);
//        m_intPropertyManager->setRange(widthProp, 1, 10);
//        property->addSubProperty(widthProp);
//        m_intProps.insert(widthProp);
//    }
//    void uninitializeProperty(QtProperty *property){
//        QList<QtProperty*> props = property->subProperties();
//        foreach(QtProperty *prop, props) {
//            property->removeSubProperty(prop);
//            m_intProps.remove(prop);
//            delete prop;
//        }
//    }
//    QtProperty *createProperty() {
//        return QtAbstractPropertyManager::createProperty();
//    }

//private:
//    QtIntPropertyManager *m_intPropertyManager;
//    QSet<QtProperty*> m_intProps;
//};

//class MySpinBoxFactory : public QtSpinBoxFactory {
//public:
//    MySpinBoxFactory(QObject *parent = 0) : QtSpinBoxFactory(parent) {}
//protected:
//    void connectPropertyManager(QtIntPropertyManager *manager) {
//        qDebug() << Q_FUNC_INFO;
//        QtSpinBoxFactory::connectPropertyManager(manager);
//    }

//    QWidget *createEditor(QtIntPropertyManager *manager, QtProperty *property,
//                QWidget *parent) {
//        qDebug() << Q_FUNC_INFO;
//        return QtSpinBoxFactory::createEditor(manager, property, parent);
//    }
//};

//class StrokeEditorFactory : public QtAbstractEditorFactory<StrokePropertyManager>
//{
//public:
//    StrokeEditorFactory(QObject *parent = 0)
//        : QtAbstractEditorFactory(parent) {
//        m_intFactory = new MySpinBoxFactory(this);

//    }
//    QtSpinBoxFactory *spinBoxFactory() const { return m_intFactory; }

//protected:
//    void connectPropertyManager(StrokePropertyManager *manager) {

//        QtIntPropertyManager *intManager = manager->findChild<QtIntPropertyManager*>();
//        qDebug() << Q_FUNC_INFO << intManager;
//        m_intFactory->addPropertyManager(intManager);
//    }
//    void disconnectPropertyManager(StrokePropertyManager *manager) {
//        QtIntPropertyManager *intManager = manager->findChild<QtIntPropertyManager*>();
//        m_intFactory->removePropertyManager(intManager);
//    }
//    QWidget *createEditor(StrokePropertyManager *manager, QtProperty *property,
//                    QWidget *parent) {
//        qDebug() << Q_FUNC_INFO << property->propertyName();
//        if(manager->isIntProp(property)) {
//            qDebug() << "IS INT PROP";
//            QtAbstractEditorFactoryBase *factory = m_intFactory;
//            return factory->createEditor(property, parent);
//        }
//        return 0;
//    }

//private:
//    QtSpinBoxFactory *m_intFactory;
//};

namespace {

class PreviewManager {
public:
    virtual ~PreviewManager() {}
    virtual QVariantMap renderOptions() const = 0;
    virtual void setupPainter(QPainter *painter) = 0;
};

class PreviewWidget : public QWidget {
public:
    PreviewWidget(QWidget *parent = 0) : QWidget(parent) {
        setBackgroundRole(QPalette::Base);
        setAutoFillBackground(true);
    }
    void setView(MVAbstractView *view) {
        m_view = view;
        resize(m_view->size());
        update();
    }
    void setManager(PreviewManager *manager) { m_manager = manager; }

protected:
    void paintEvent(QPaintEvent *pe) {
        QWidget::paintEvent(pe);
        if (!m_view) return;
        QPainter painter(this);
        QVariantMap options;
        if (m_manager) {
            m_manager->setupPainter(&painter);
            options = m_manager->renderOptions();
        }
        painter.save();
        if (m_view->viewFeatures() & MVAbstractView::RenderView)
            m_view->renderView(&painter, options);
        else {
            qWarning() << "View" << m_view->metaObject()->className() << "didn't report renderView capabilities. Using fallback code path";
            m_view->render(&painter);
        }
        painter.restore();
    }

private:
    MVAbstractView *m_view;
    PreviewManager *m_manager = nullptr;
};

class ExportPreviewDialog : public QDialog, public PreviewManager {
public:
    ExportPreviewDialog(QWidget *parent = 0) : QDialog(parent), ui(new Ui::ExportPreviewWidget) {
        ui->setupUi(this);
        m_preview = new PreviewWidget;
        m_preview->setManager(this);
        ui->scrollArea->setWidget(m_preview);
        ui->scrollArea->setWidgetResizable(false);

        propertyBrowser = ui->propertyBrowser;
        propertyBrowser->addProperty("Background", QColor(Qt::transparent));
        propertyBrowser->addProperty("Size", QVariant(QSize(2000,800)));
        propertyBrowser->addProperty("Font", QVariant(QFont()));

        connect(propertyBrowser, &ViewPropertyEditor::propertiesChanged, [this]() {
            m_preview->resize(propertyBrowser->values()["Size"].toSize());
            m_preview->update();
        });
//        connect(propertyBrowser, SIGNAL(propertiesChanged()), m_preview, SLOT(update()));
//        StrokePropertyManager *strokePropManager = new StrokePropertyManager(this);
//        StrokeEditorFactory *strokeFactory = new StrokeEditorFactory(this);

        /*
        QtVariantProperty *sizeGroup = propertyBrowser->propertyManager()->addProperty(QVariant::Size, "size");
        QtVariantProperty *aspectProp = propertyBrowser->propertyManager()->addProperty(QVariant::Bool, "Preserve aspect ratio");
        sizeGroup->addSubProperty(aspectProp);
        QtVariantProperty *genericGroup = propertyBrowser->propertyManager()->addProperty(QtVariantPropertyManager::groupTypeId(), "Generic");
        genericGroup->addSubProperty(sizeGroup);
        propertyBrowser->addProperty(genericGroup);
        QtVariantProperty *fontProp = propertyBrowser->propertyManager()->addProperty(QVariant::Font, "font");
        genericGroup->addSubProperty(fontProp);
        QtProperty *clusterGroup = propertyBrowser->propertyManager()->addProperty(QtVariantPropertyManager::groupTypeId(), "Cluster Panel");
        propertyBrowser->addProperty(clusterGroup);
//        propertyBrowser->setFactoryForManager(strokePropManager, strokeFactory);

        QtProperty *strokeProp = propertyBrowser->propertyManager()->addProperty(QtVariantPropertyManager::groupTypeId(), "stroke");
        QtVariantProperty *strokeWidth = propertyBrowser->propertyManager()->addProperty(QVariant::Int, "width");
        strokeProp->addSubProperty(strokeWidth);
        QtVariantProperty *aaProp = propertyBrowser->propertyManager()->addProperty(QVariant::Bool, "smooth");
        strokeProp->addSubProperty(aaProp);
        clusterGroup->addSubProperty(strokeProp);
        */
    }
    void setView(MVAbstractView *view) {
        m_view = view;
        m_preview->setView(view);
        propertyBrowser->setValue("Size", m_view->size());
        propertyBrowser->setValue("Font", m_view->font());
    }

    void accept() {
        if (!m_view) return;
        QStringList filters;
        filters << "Images (*.png *.jpg *.bmp)";
        filters << "Portable Document Format (*.pdf)";
#ifdef QT_SVG_LIB
        filters << "Scalable Vector Format (*.svg)";
#endif
        QString selectedFilter;
        QString path = QFileDialog::getSaveFileName(window(),
                                                    tr("Export view..."),
                                                    QString(),
                                                    filters.join(";;"),
                                                    &selectedFilter);

        if (path.isEmpty()) return;
        if (selectedFilter == filters.at(1)) {
            QPdfWriter writer(path);
            const int resolution = 100;
            writer.setResolution(resolution);
            writer.setPageSize(QPageSize(QSizeF(ui->propertyBrowser->values()["Size"].toSize())/resolution, QPageSize::Inch));
            writer.setPageMargins(QMargins(0,0,0,0));
            //        writer.setPageSize(QPdfWriter::A4);
            //        writer.setPageOrientation(QPageLayout::Landscape);
            QPainter painter(&writer);
            setupPainter(&painter);
            m_view->renderView(&painter, renderOptions());
        } else if (selectedFilter == filters.at(0)){
            QImage img(ui->propertyBrowser->values()["Size"].toSize(), QImage::Format_ARGB32);
            img.fill(Qt::transparent);
            QPainter painter(&img);
            setupPainter(&painter);
            m_view->renderView(&painter, renderOptions());
            img.save(path);
        }
        QDialog::accept();
    }

    void setupPainter(QPainter *painter) {
        QRect r(QPoint(0,0), propertyBrowser->value("Size").toSize());
        QColor bgColor = propertyBrowser->value("Background").value<QColor>();
        if (bgColor != Qt::transparent)
            painter->fillRect(r, bgColor);
        QFont f = propertyBrowser->value("Font").value<QFont>();
        painter->setFont(f);
    }
    QVariantMap renderOptions() const {
        return propertyBrowser->values();
    }

private:
    QSharedPointer<Ui::ExportPreviewWidget> ui;
    MVAbstractView *m_view = nullptr;
    PreviewWidget *m_preview;
    ViewPropertyEditor *propertyBrowser;
};

}

ViewImageExporter::ViewImageExporter(QObject *parent) : QObject(parent)
{
}

QDialog *ViewImageExporter::createViewExportDialog(MVAbstractView *view, QWidget *parent)
{
    ExportPreviewDialog *dialog = new ExportPreviewDialog(parent);
    dialog->setView(view);
    dialog->resize(800,600);
    return dialog;
}

bool ViewImageExporter::simpleExport(MVAbstractView *view)
{
    return true;
}
