#ifndef FILECHOOSEWIDGET_H
#define FILECHOOSEWIDGET_H

#include <QWidget>

#include <dlinkbutton.h>

class QPushButton;
class FileChooseWidget : public QWidget
{
    Q_OBJECT

public:
    explicit FileChooseWidget(QWidget *parent = nullptr);

signals:
    void packagesSelected(const QStringList files) const;

protected:
    void dragEnterEvent(QDragEnterEvent *e);
    void dropEvent(QDropEvent *e);

private slots:
    void chooseFiles();

private:
    Dtk::Widget::DLinkButton *m_fileChooseBtn;
};

#endif // FILECHOOSEWIDGET_H
