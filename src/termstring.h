//
// C++ Interface: TermString
//
// Description:
//
//
// Author: hooey <hephooey@gmail.com>, (C) 2009
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef TERMSTRING_H
#define TERMSTRING_H

#include <QtCore/QString>
#include <QByteArray>

namespace QTerm
{
class TermString
{
public:
    TermString();
    TermString(const QString & str);
    ~TermString();
    void insert(int index, const QString & str);
    void append(const QString & str);
    void replace(int index, int length, const QString & str);
    void remove(int index, int length);
    int length();
    QString mid(int index, int len);
    QString string();
    int beginIndex(int pos);
    int size(int index);
    bool isPartial(int index);
    static int wcwidth(QChar ch);
private:
    struct interval {
        int first;
        int last;
    };
    static int bisearch(QChar ucs, const struct interval *table, int max);
    void updateIndex();
    QString m_string;
    QByteArray m_index;
};
} // namespace QTerm

#endif // TERMSTRING_H
