#ifndef RENDERABLE_H
#define RENDERABLE_H

class QPainter;
#include <QRectF>
#include <QVariant>

// TODO: Subclass QObject?
// TODO: Add parent?
class RenderOptionBase {
public:
    virtual ~RenderOptionBase(){}
    QString name() const { return m_name; }
    virtual QVariant defaultValue() const = 0;
    // mode: basic, advanced
    void setAttribute(const QString &attr, const QVariant &value) {
        m_attrs[attr] = value;
    }

    QVariant attribute(const QString &attr) const {
        return m_attrs.value(attr);
    }
    QStringList attributeNames() const { return m_attrs.keys(); }
    virtual QVariant value() const = 0;
    virtual void setValue(const QVariant &v) = 0;
protected:
    RenderOptionBase(const QString &name) : m_name(name) {}
    virtual RenderOptionBase* clone() const = 0;
private:
    QString m_name;
    QVariantMap m_attrs;
    friend class RenderOptionSet;
};

template<typename T> class RenderOption : public RenderOptionBase {
public:
    QVariant defaultValue() const { return QVariant::fromValue(m_default); }
    QVariant value() const { return QVariant::fromValue(m_value); }
    void setValue(const QVariant &v) { m_value = v.value<T>(); }
protected:
    RenderOption(const QString &name, T defaultValue)
        : RenderOptionBase(name), m_default(defaultValue), m_value(defaultValue) {}
    RenderOptionBase* clone() const {
        RenderOption<T>* cl = new RenderOption<T>(name(), m_default);
        return cl;
    }

private:
    T m_default;
    T m_value;
    friend class RenderOptionSet;
};

// TODO: Subclass QObject?
class RenderOptionSet {
public:

    virtual ~RenderOptionSet();
    inline QString name() const { return m_name; }
    void setName(const QString &n);
    void addSubSet(RenderOptionSet *set);
    RenderOptionSet* addSubSet(const QString &name);
    RenderOptionSet* removeSubSet(const QString &name);
    RenderOptionSet* removeSubSet(RenderOptionSet* set);

    const RenderOptionSet* subSet(const QString &name) const;
    template<typename T> RenderOption<T>* addOption(const QString &name, const T & defaultValue) {
        RenderOption<T>* opt = new RenderOption<T>(name, defaultValue);
        m_options.insert(name, opt);
        return opt;
    }

    const RenderOptionBase* option(const QString &name) const;
    QList<RenderOptionBase*> options() const;
    QList<RenderOptionSet *> sets() const;
    RenderOptionSet *extension() const;
    void setExtension(RenderOptionSet* e);
    //static from JSON
    /*
     {
        name: 'setname',
        options: [
            {
                name: 'Background',
                type: 'color',
                default: 'transparent',
            },

        ]
     }
     */
    virtual RenderOptionSet* clone() const;
    RenderOptionSet* parent() const;
protected:
    RenderOptionSet(const QString &name) : m_name(name) {}
    // subclass can override to create subsets of its own class (for future use)
    virtual RenderOptionSet* createNew(const QString &name) const;
    friend class Renderable;
private:
    QString m_name;
    RenderOptionSet* m_parent = nullptr;
    QMap<QString,RenderOptionBase*> m_options;
    QMap<QString,RenderOptionSet*> m_sets;
    RenderOptionSet* m_extension = nullptr;
};

//class RenderOptions {
//public:
//    void setValue(const QString &name, const QVariant &value);
//    QVariant value(const QString &name) const;
//    bool hasValue(const QString &name) const;
//    QStringList keys() const;
//    void clear();
//private:
//    QVariantMap m_data;
//};

class Renderable
{
public:
    Renderable();
    virtual ~Renderable() {}

    virtual void renderView(QPainter *painter, const RenderOptionSet *options, const QRectF &rect = QRectF());
    virtual RenderOptionSet* renderOptions() const { return nullptr; }
protected:
    RenderOptionSet* optionSet(const QString &name) const {
        return new RenderOptionSet(name);
    }
};

#endif // RENDERABLE_H
