#include "renderable.h"

/*!
 * \section Render options architecture
 *
 * Render options are aggregated in a RenderOptionSet.
 * Each set can contain subsets and simple options
 * In addtition a render option set can be "extended" with
 * one other render option set. The set being extended acts
 * as if options and subsets from the extending set were its own.
 */


/*!
 * \section Render Options in QML
 *
 * mockup:
 *
 * import MountainLab.RenderOptions 1.0
 *
 * RenderOptionSet {
 *   id: root
 *
 *   extension: RenderOptionSet {
 *     ColorRenderOption {
 *       id: extColor
 *       name: "color"
 *       defaultValue: "black"
 *     }
 *   }
 *
 *
 *
 *   RenderOptionSet {
 *     id: panelSubset
 *     name: "panel"
 *     RenderOptionSet {
 *       name: "stroke"
 *       RenderOptionColor {
 *         name: "color"
 *         defaultValue: Qt.lighter(extColor.value, 1.1)
 *       }
 *       RenderOptionInt {
 *         name: "size"
 *         defaultValue: 2
 *       }
 *     }
 *   }
 *
 * }
 *
 */

Renderable::Renderable()
{

}

void Renderable::renderView(QPainter *painter, const RenderOptionSet *options, const QRectF &rect)
{
    // empty
}

//void RenderOptions::setValue(const QString &name, const QVariant &value)
//{
//    QStringList path = name.split('/');

//}

//QVariant RenderOptions::value(const QString &name) const
//{
//    QStringList path = name.split('/');
//    QVariant val = m_data;
//    for(int i=0; i<path.size()-1; ++i) {
//        if (val.type() != QVariant::Map) return;
//        const QString &component = path.at(i);
//        val = val.toMap().value(component);
//    }
//    return val.toMap().value(path.last());
//}

//bool RenderOptions::hasValue(const QString &name) const
//{

//}

//QStringList RenderOptions::keys() const
//{

//}

//void RenderOptions::clear()
//{
//    m_data.clear();
//}

RenderOptionSet::~RenderOptionSet() {
    qDeleteAll(m_options.values());
    qDeleteAll(m_sets.values());
    if (parent()) {
        parent()->removeSubSet(this);
    }
    if (m_extension) {
        delete m_extension;
        m_extension = nullptr;
    }
}

void RenderOptionSet::setName(const QString &n) { m_name = n; }

void RenderOptionSet::addSubSet(RenderOptionSet *set) {
    if (m_sets.contains(set->name()))
        return;
    set->m_parent = this;
    m_sets.insert(set->name(), set);
}

RenderOptionSet *RenderOptionSet::addSubSet(const QString &name) {
    if (m_sets.contains(name)) return nullptr;
    RenderOptionSet *s = createNew(name);
    addSubSet(s);
    return s;
}

RenderOptionSet* RenderOptionSet::removeSubSet(const QString &name)
{
    RenderOptionSet *ss = m_sets.value(name, nullptr);
    if (!ss) return nullptr;
    ss->m_parent = nullptr;
    m_sets.remove(name);
    return ss;
}

RenderOptionSet* RenderOptionSet::removeSubSet(RenderOptionSet *set)
{
    if (set->parent()!=this) return nullptr;
    if (!m_sets.remove(set->name())) return nullptr;
    return set;
}

const RenderOptionSet *RenderOptionSet::subSet(const QString &name) const {
    return m_sets.value(name, nullptr);
}

const RenderOptionBase *RenderOptionSet::option(const QString &name) const {
    const RenderOptionBase* opt = m_options.value(name, nullptr);
    const RenderOptionSet*  ext = extension();
    if (!opt && ext) {
        opt = ext->option(name);
    }
    return opt;
}

QList<RenderOptionBase *> RenderOptionSet::options() const { return m_options.values(); }

QList<RenderOptionSet *> RenderOptionSet::sets() const { return m_sets.values(); }

RenderOptionSet *RenderOptionSet::extension() const {
    if (m_extension)
        return m_extension;
    if (!parent()) return nullptr;
    return parent()->extension();
}

void RenderOptionSet::setExtension(RenderOptionSet *e) { m_extension = e; }

RenderOptionSet *RenderOptionSet::clone() const
{
    RenderOptionSet* newSet = new RenderOptionSet(name());
    QMapIterator<QString,RenderOptionBase*> optionsIter(m_options);
    while(optionsIter.hasNext()) {
        optionsIter.next();
        RenderOptionBase *clOption = optionsIter.value()->clone();
        newSet->m_options.insert(optionsIter.key(), clOption);
    }
    QMapIterator<QString,RenderOptionSet*> setsIter(m_sets);
    while(setsIter.hasNext()) {
        setsIter.next();
        RenderOptionSet *clSet = setsIter.value()->clone();
        newSet->m_sets.insert(setsIter.key(), clSet);
    }
    return newSet;
}

RenderOptionSet *RenderOptionSet::parent() const {
    return m_parent;
}

RenderOptionSet *RenderOptionSet::createNew(const QString &name) const {
    return new RenderOptionSet(name);
}
