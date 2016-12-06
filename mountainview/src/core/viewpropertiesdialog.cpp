#include "viewpropertiesdialog.h"

#include "renderable.h"
#include "viewpropertyeditor.h"
#include <QDialogButtonBox>
#include <QLayout>
#include <QPushButton>

ViewPropertiesDialog::ViewPropertiesDialog(QWidget *parent)
    : QDialog(parent) {
    setWindowTitle("View Properties");
    QVBoxLayout *l = new QVBoxLayout(this);
    m_editor = new ViewPropertyEditor;
    l->addWidget(m_editor);
    QDialogButtonBox *bbox = new QDialogButtonBox(QDialogButtonBox::Save|QDialogButtonBox::Apply|QDialogButtonBox::Close, Qt::Horizontal);
    l->addWidget(bbox);
    QAbstractButton *applyButton = bbox->button(QDialogButtonBox::Apply);
    connect(bbox, SIGNAL(accepted()), SLOT(accept()));
    connect(bbox, SIGNAL(rejected()), SLOT(reject()));
    connect(bbox, &QDialogButtonBox::clicked, [applyButton,this](QAbstractButton*b) {
        if (b == applyButton) {
            m_editor->apply(); // modifies m_renderOptions
            emit applied();
        }
    });
}

void ViewPropertiesDialog::setRenderOptions(RenderOptionSet *renderOptions) {
    m_editor->addRenderOptions(renderOptions);
    m_renderOptions = renderOptions;
}

RenderOptionSet *ViewPropertiesDialog::renderOptions() const { return m_renderOptions; }

QSize ViewPropertiesDialog::sizeHint() const
{
    return QDialog::sizeHint().expandedTo(QSize(1, 400));
}
