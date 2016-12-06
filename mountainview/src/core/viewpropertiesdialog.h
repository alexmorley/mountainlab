#ifndef VIEWPROPERTIESDIALOG_H
#define VIEWPROPERTIESDIALOG_H

#include <QDialog>

class RenderOptionSet;
class ViewPropertyEditor;
class ViewPropertiesDialog : public QDialog {
    Q_OBJECT
public:
    ViewPropertiesDialog(QWidget *parent = 0);
    void setRenderOptions(RenderOptionSet* renderOptions);
    RenderOptionSet* renderOptions() const;
    QSize sizeHint() const;
signals:
    void applied();
private:
    RenderOptionSet* m_renderOptions = nullptr;
    ViewPropertyEditor *m_editor;
};


#endif // VIEWPROPERTIESDIALOG_H
