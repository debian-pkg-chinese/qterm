/****************************************************************************
** Form interface generated from reading ui file 'articledialog.ui'
**
** Created: Sun Jan 5 19:46:28 2003
**      by: The User Interface Compiler ($Id: articledialog.h 1188 2010-06-12 19:43:42Z hephooey $)
**
** WARNING! All changes made in this file will be lost!
****************************************************************************/

#ifndef ARTICLEDIALOG_H
#define ARTICLEDIALOG_H

#include "ui_articledialog.h"
namespace QTerm
{
class articleDialog : public QDialog
{
    Q_OBJECT

public:
    articleDialog(QWidget* parent = 0, Qt::WFlags fl = 0);
    ~articleDialog();

    QString strArticle;
    Ui::articleDialog ui;
protected slots:
    void onSave();
};

} // namespace QTerm

#endif // ARTICLEDIALOG_H
