#include "viewpropertyeditor.h"
#include <QVBoxLayout>
#include <qtvariantproperty.h>
#include <QtDebug>

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

void ViewPropertyEditor::addRenderOptions(RenderOptionSet *set)
{
    QList<RenderOptionBase*> opts = set->options();
    QList<RenderOptionBase*> extOpts = set->extension() ? set->extension()->options() : QList<RenderOptionBase*>();
    opts += extOpts;
    qSort(opts.begin(), opts.end(), [](RenderOptionBase* o1, RenderOptionBase* o2) {
        return o1->name() < o2->name();
    });
    foreach(RenderOptionBase *option, opts) {
        QtVariantProperty *prop = m_propertyManager->addProperty(option->defaultValue().type(), option->name());
        prop->setValue(option->value());
        foreach(const QString &attr, option->attributeNames()) {
            prop->setAttribute(attr, option->attribute(attr));
        }
        m_options.insert(prop, option);
        m_allProperties.append(prop);
    }
    foreach(RenderOptionSet *subset, set->sets()) {
        QtVariantProperty *group = m_propertyManager->addProperty(QtVariantPropertyManager::groupTypeId(), subset->name());
        m_sets.insert(group, subset);
        m_allProperties.append(group);
        addRenderOptions(subset, group);
    }
}

void ViewPropertyEditor::addRenderOptionsWithExtra(RenderOptionSet* main, RenderOptionSet *extra)
{
    // first add extra options
    QList<RenderOptionBase*> opts = extra->options();
    qSort(opts.begin(), opts.end(), [](RenderOptionBase* o1, RenderOptionBase* o2) {
        return o1->name() < o2->name();
    });
    foreach(RenderOptionBase *option, opts) {
        QtVariantProperty *prop = m_propertyManager->addProperty(option->defaultValue().type(), option->name());
        prop->setValue(option->value());
        foreach(const QString &attr, option->attributeNames()) {
            prop->setAttribute(attr, option->attribute(attr));
        }
        m_options.insert(prop, option);
        m_allProperties.append(prop);
    }
    foreach(RenderOptionSet *subset, extra->sets()) {
        QtVariantProperty *group = m_propertyManager->addProperty(QtVariantPropertyManager::groupTypeId(), subset->name());
        m_sets.insert(group, subset);
        m_allProperties.append(group);
        addRenderOptions(subset, group);
    }
    if (!main) return;
    // now add main options
    QtVariantProperty *mainGroup = m_propertyManager->addProperty(QtVariantPropertyManager::groupTypeId(), main->name());
    m_allProperties.append(mainGroup);
    opts = main->options();
    QList<RenderOptionBase*> extOpts = main->extension() ? main->extension()->options() : QList<RenderOptionBase*>();
    opts += extOpts;
    qSort(opts.begin(), opts.end(), [](RenderOptionBase* o1, RenderOptionBase* o2) {
        return o1->name() < o2->name();
    });
    foreach(RenderOptionBase *option, opts) {
        QtVariantProperty *prop = m_propertyManager->addProperty(option->defaultValue().type(), option->name());
        prop->setValue(option->value());
        foreach(const QString &attr, option->attributeNames()) {
            prop->setAttribute(attr, option->attribute(attr));
        }
        m_options.insert(prop, option);
        mainGroup->addSubProperty(prop);
    }
    foreach(RenderOptionSet *subset, main->sets()) {
        QtVariantProperty *group = m_propertyManager->addProperty(QtVariantPropertyManager::groupTypeId(), subset->name());
        m_sets.insert(group, subset);
        mainGroup->addSubProperty(group);
        addRenderOptions(subset, group);
    }
}

void ViewPropertyEditor::addRenderOptions(RenderOptionSet *set, QtProperty *parent)
{
    QList<RenderOptionBase*> opts = set->options();
    qSort(opts.begin(), opts.end(), [](RenderOptionBase* o1, RenderOptionBase* o2) {
        return o1->name() < o2->name();
    });
    foreach(RenderOptionBase *option, opts) {
        QtVariantProperty *prop = m_propertyManager->addProperty(option->defaultValue().type(), option->name());
        prop->setValue(option->value());
        foreach(const QString &attr, option->attributeNames()) {
            prop->setAttribute(attr, option->attribute(attr));
        }
        m_options.insert(prop, option);
        parent->addSubProperty(prop);
    }
    foreach(RenderOptionSet *subset, set->sets()) {
        QtVariantProperty *group = m_propertyManager->addProperty(QtVariantPropertyManager::groupTypeId(), subset->name());
        m_sets.insert(group, subset);
        parent->addSubProperty(group);
        addRenderOptions(subset, group);
    }
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

void ViewPropertyEditor::apply()
{
    QMapIterator<QtProperty*,RenderOptionBase*> iter(m_options);
    while (iter.hasNext()) {
        iter.next();
        QtVariantProperty *prop = static_cast<QtVariantProperty*>(iter.key());
        RenderOptionBase *opt = iter.value();
        opt->setValue(prop->value());
    }
}

void ViewPropertyEditor::showEvent(QShowEvent *event)
{
    relayoutProperties();
}

void ViewPropertyEditor::relayoutProperties()
{
    m_browser->clear();
    foreach(QtVariantProperty *prop, m_allProperties)
        m_browser->addProperty(prop);
}


