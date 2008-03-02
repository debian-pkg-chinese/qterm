/***************************************************************************
 *   Copyright (C) 2005 by Max Howell <max.howell@methylblue.com>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/


//WARNING this is not meant for use outside this unit!


#ifndef QTERM_PROGRESSBAR_H
#define QTERM_PROGRESSBAR_H

#include <QProgressBar>
//Added by qt3to4:
#include <QLabel>

class QLabel;
class QPushButton;

namespace QTerm
{
    /**
     * @class KDE::ProgressBar
     * @short ProgressBar class with some useful additions
     */
    class ProgressBar : public QProgressBar
    {
        friend class StatusBar;

    public:
        /** @param text a 1-6 word description of the progress operation */
        ProgressBar &setDescription( const QString &text );

        /** @param text eg. Scanning, Reading. The state of the operation */
        ProgressBar &setStatus( const QString &text );

        /** set the recipient slot for the abort button */
        ProgressBar &setAbortSlot( QObject *receiver, const char *slot );

        void setDone();

        QString description() const { return m_description; }

    protected:
        ProgressBar( QWidget *parent, QLabel *label );
       ~ProgressBar();

        virtual void hide();

        QLabel *m_label;
        QString m_description;
        bool    m_done;

        QPushButton *m_abort;
    };
}

#endif
