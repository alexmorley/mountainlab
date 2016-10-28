#include "viewimageexporter.h"
#include <QDialog>
#include <QDialogButtonBox>
#include <QPainter>
#include <QPdfWriter>
#include <QScrollArea>
#include <QSpinBox>
#include <QToolBar>
#include <QVBoxLayout>
#include "../ui_exportpreviewwidget.h"

namespace {

class PreviewManager {
public:
    virtual ~PreviewManager() {}
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
        if (m_manager) m_manager->setupPainter(&painter);
        painter.save();
        if (m_view->viewFeatures() & MVAbstractView::RenderView)
            m_view->renderView(&painter);
        else
            m_view->render(&painter);
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
        connect(ui->width, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this]{
            m_preview->resize(ui->width->value(), ui->height->value());
        });
        connect(ui->height, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this]{
            m_preview->resize(ui->width->value(), ui->height->value());
        });
        QFont f = font();
        ui->fontSize->setValue(f.pointSize());

        connect(ui->fontSizeEnabled, SIGNAL(toggled(bool)), m_preview, SLOT(update()));
        connect(ui->fontSize, SIGNAL(valueChanged(int)), m_preview, SLOT(update()));
    }
    void setView(MVAbstractView *view) {
        m_view = view;
        m_preview->setView(view);
        ui->width->setValue(m_view->width());
        ui->height->setValue(m_view->height());
    }

    void accept() {
        if (!m_view) return;
        QPdfWriter writer("/tmp/export.pdf");
        const int resolution = 100;
        writer.setResolution(resolution);
        writer.setPageSize(QPageSize(QSizeF(ui->width->value(), ui->height->value())/resolution, QPageSize::Inch));
        writer.setPageMargins(QMargins(0,0,0,0));
        //        writer.setPageSize(QPdfWriter::A4);
        //        writer.setPageOrientation(QPageLayout::Landscape);
        QPainter painter(&writer);
        setupPainter(&painter);
        m_view->renderView(&painter);
#if 0
        QImage img(ui->width->value(), ui->height->value(), QImage::Format_ARGB32);
        img.fill(Qt::transparent);
        QPainter painter(&img);
        m_view->renderView(&painter);
        img.save("/tmp/export.png", "PNG");
#endif
        QDialog::accept();
    }

    void setupPainter(QPainter *painter) {
        if (ui->fontSizeEnabled->isChecked()) {
            QFont f = painter->font();
            f.setPointSize(ui->fontSize->value());
            painter->setFont(f);
        }
    }

private:
    QSharedPointer<Ui::ExportPreviewWidget> ui;
    MVAbstractView *m_view = nullptr;
    PreviewWidget *m_preview;
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
