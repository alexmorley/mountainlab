#include "viewpropertyeditor.h"
#include <QVBoxLayout>
#include <qtvariantproperty.h>

ViewPropertyEditor::ViewPropertyEditor(QWidget *parent)
    : QWidget(parent)
    , m_propertyManager(new QtVariantPropertyManager(this))
    , m_editorFactory(new QtVariantEditorFactory(this))
    , m_browser(new QtTreePropertyBrowser)
{
    QVBoxLayout *l = new QVBoxLayout(this);
    l->setMargin(0);
    l->addWidget(m_browser);
    m_browser->setPropertiesWithoutValueMarked(true);
    m_browser->setAlternatingRowColors(true);

    m_browser->setFactoryForManager(m_propertyManager, m_editorFactory);
    connect(m_propertyManager, SIGNAL(propertyChanged(QtProperty*)), SIGNAL(propertiesChanged()));

}

QtVariantPropertyManager *ViewPropertyEditor::propertyManager() const
{
    return m_propertyManager;
}

void ViewPropertyEditor::addProperty(const QString &propName, const QVariant &value, bool basic)
{
    QtVariantProperty *property = m_propertyManager->addProperty(value.type(), propName);
    property->setValue(value);
    m_allProperties.append(property);
}

QVariantMap ViewPropertyEditor::values() const
{
    QVariantMap result;
    foreach(QtVariantProperty *prop, m_allProperties) {
        QString propName = prop->propertyName();
        QVariant value = prop->value();
        result.insert(propName, value);
    }
    return result;
}

void ViewPropertyEditor::setValue(const QString &propName, const QVariant &value)
{
    foreach(QtVariantProperty *prop, m_allProperties) {
        if (prop->propertyName() == propName) {
            prop->setValue(value);
            return;
        }
    }
}

QVariant ViewPropertyEditor::value(const QString &propName) const
{
    foreach(QtVariantProperty *prop, m_allProperties) {
        if (prop->propertyName() == propName) {
            return prop->value();
        }
    }
    return QVariant();
}

void ViewPropertyEditor::showEvent(QShowEvent *event)
{
    relayoutProperties();
}

void ViewPropertyEditor::relayoutProperties()
{
    foreach(QtVariantProperty *prop, m_allProperties)
         m_browser->addProperty(prop);
}
