#include "renderable.h"

Renderable::Renderable()
{

}

void Renderable::renderView(QPainter *painter, const QVariantMap &options, const QRectF &rect)
{
    // empty
}

void RenderOptions::setValue(const QString &name, const QVariant &value)
{
    QStringList path = name.split('/');

}

QVariant RenderOptions::value(const QString &name) const
{
    QStringList path = name.split('/');
    QVariant val = m_data;
    for(int i=0; i<path.size()-1; ++i) {
        if (val.type() != QVariant::Map) return QVariant();
        const QString &component = path.at(i);
        val = val.toMap().value(component);
    }
    return val.toMap().value(path.last());
}

bool RenderOptions::hasValue(const QString &name) const
{

}

QStringList RenderOptions::keys() const
{

}

void RenderOptions::clear()
{
    m_data.clear();
}
