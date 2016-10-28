#ifndef RENDERABLE_H
#define RENDERABLE_H

class QPainter;
#include <QRectF>

class Renderable
{
public:
    Renderable();
    ~Renderable() {}

    void renderView(QPainter *painter, const QRectF &rect = QRectF());
};

#endif // RENDERABLE_H
