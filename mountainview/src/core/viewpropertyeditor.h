#ifndef VIEWPROPERTYEDITOR_H
#define VIEWPROPERTYEDITOR_H

#include <qttreepropertybrowser.h>
#include <renderable.h>

//class ViewProperty {
//public:

//    ViewProperty(const QString &name);
//    void setAttribute(const QString &name, const QVariant &value);
//    QVariant defaultValue();
//private:

//};

//class ViewPropertyGroup {
//public:

//};

class QtVariantPropertyManager;
class QtVariantEditorFactory;
class QtVariantProperty;
class QtProperty;
class ViewPropertyEditor : public QWidget
{
    Q_OBJECT
    Q_ENUMS(Mode)
public:
    ViewPropertyEditor(QWidget *parent = 0);
    enum Mode {
        Basic, Advanced
    };
    QtVariantPropertyManager *propertyManager() const;

    /* experimental API */
    void addProperty(const QString &propName, const QVariant &value, bool basic = true);
    void addRenderOptions(RenderOptionSet *set);
    void addRenderOptionsWithExtra(RenderOptionSet* main, RenderOptionSet *extra);

    QVariantMap values() const;
    void setValue(const QString &propName, const QVariant &value);
    QVariant value(const QString &propName) const;

    /* end of experimental API */
public slots:
    void apply();
signals:
    void propertiesChanged();
protected:
    void showEvent(QShowEvent *event);
    void relayoutProperties();
    void addRenderOptions(RenderOptionSet *set, QtProperty *parent);
private:
    QtVariantPropertyManager *m_propertyManager;
    QtVariantEditorFactory *m_editorFactory;
    QtTreePropertyBrowser *m_browser;
    QList<QtVariantProperty*> m_allProperties;
    QMap<QtProperty*, RenderOptionBase*> m_options;
    QMap<QtProperty*, RenderOptionSet*> m_sets;

};

#endif // VIEWPROPERTYEDITOR_H
