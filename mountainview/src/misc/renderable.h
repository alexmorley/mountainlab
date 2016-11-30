#ifndef RENDERABLE_H
#define RENDERABLE_H

class QPainter;
#include <QRectF>
#include <QVariant>

class RenderOptions {
public:
    void setValue(const QString &name, const QVariant &value);
    QVariant value(const QString &name) const;
    bool hasValue(const QString &name) const;
    QStringList keys() const;
    void clear();
private:
    QVariantMap m_data;
};

class Renderable
{
public:
    Renderable();
    ~Renderable() {}

    void renderView(QPainter *painter, const QVariantMap &options, const QRectF &rect = QRectF());
};

#endif // RENDERABLE_H
