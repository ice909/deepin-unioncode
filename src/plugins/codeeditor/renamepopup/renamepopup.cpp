#include "renamepopup.h"

#include "common/common.h"

#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QLabel>
#include <QEventLoop>

class RenamePopupPrivate
{
    friend class RenamePopup;
    QString oldName{""};
    QLineEdit *renameEdit{nullptr};
    QLabel *renameLabel{nullptr};
    QVBoxLayout *vLayout{nullptr};
    QEventLoop loop;
};

RenamePopup *RenamePopup::instance()
{
    static RenamePopup ins;
    return &ins;
}

RenamePopup::RenamePopup(QWidget *parent)
    :d (new RenamePopupPrivate())
{
    RenamePopup::setWindowFlag(Qt::Popup, true);

    d->renameEdit = new QLineEdit();
    d->renameLabel = new QLabel();
    d->vLayout = new QVBoxLayout();

    QObject::connect(d->renameEdit, &QLineEdit::editingFinished, [=](){
        emit this->editingFinished(d->renameEdit->text());
        this->close();
        d->loop.exec();
    });

    d->vLayout->addWidget(d->renameLabel);
    d->vLayout->addWidget(d->renameEdit);
    RenamePopup::setLayout(d->vLayout);
}

RenamePopup::~RenamePopup()
{
    if (d) {
        delete d;
    }
}

void RenamePopup::setOldName(const QString &name)
{
    d->oldName = name;
}

QString RenamePopup::oldName()
{
    return d->oldName;
}

int RenamePopup::exec(const QPoint &pos)
{
    this->move(pos);
    this->show();
    return d->loop.exec();
}

int RenamePopup::exec()
{
    this->show();
    return d->loop.exec();
}

void RenamePopup::showEvent(QShowEvent *event)
{
    Q_UNUSED(event);
    d->renameLabel->setText(RenamePopup::tr("Rename %0 to:").arg(d->oldName));
    d->renameEdit->setFocus();
}

void RenamePopup::hideEvent(QHideEvent *event)
{
    Q_UNUSED(event);
    QWidget *parent = qobject_cast<QWidget*>(this->parent());
    if (parent) {
        parent->setFocus();
    }
}
