/*******************************************************************************
FILENAME:      qtermframe.cpp
REVISION:      2001.10.4 first created.

AUTHOR:        kingson fiasco
*******************************************************************************/
/*******************************************************************************
                                    NOTE
 This file may be used, distributed and modified without limitation.
 *******************************************************************************/

#include "qterm.h"
#include "qtermwindow.h"
#include "qtermframe.h"
#include "qtermwndmgr.h"

#include "qtermscreen.h"
#include "qtermdecode.h"
#include "qtermtelnet.h"
#include "qtermconvert.h"
#include "qtermbuffer.h"
#include "qtermparam.h"
#include "addrdialog.h"
#include "qtermconfig.h"
#include "qtermbbs.h"
#include "msgdialog.h"
#include "qtermtextline.h"
#include "articledialog.h"
#include "popwidget.h"
#include "qtermzmodem.h"
#include "zmodemdialog.h"
#include "qtermpython.h"
#include "qtermhttp.h"
#include "qtermiplocation.h"
#include "osdmessage.h"
#include "statusBar.h"
#include "progressBar.h"
#include "qtermglobal.h"
#include "hostinfo.h"

#if !defined(_OS_WIN32_) && !defined(Q_OS_WIN32)
#include <unistd.h>
#else
#include <windows.h>
#endif


#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include <QResizeEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QEvent>
#include <QByteArray>
#include <QWheelEvent>
#include <QCloseEvent>
#include <QPixmap>
#include <QApplication>
#include <QClipboard>
#include <QToolButton>
#include <QMessageBox>
#include <QStatusBar>
#include <QFontDialog>
#include <QTextCodec>
#include <QSound>
#include <QTimer>
#include <QMenu>
#include <QTextBrowser>
#include <QInputDialog>
#include <QRegExp>
#include <QFileDialog>
#include <QTabWidget>
#include <QStringList>
#include <QProgressBar>
#include <QHBoxLayout>
#include <QtCore/QProcess>
#include <QtDebug>

namespace QTerm
{

// script thread
DAThread::DAThread(Window *win)
{
	pWin = win;
}

DAThread::~DAThread()
{

}

void DAThread::run()
{
	mutex.lock();
	QStringList strList;
	while(1)
	{
		// check it there is duplicated string
		// it starts from the end in the range of one screen height
		// so this is a non-greedy match
		QString strTemp = pWin->stripWhitespace(pWin->m_pBuffer->screen(0)->getText());
		int i=0;
		int start=0;
		QStringList::Iterator it = strList.end();
		while ( it != strList.begin() && i < pWin->m_pBuffer->line()-1)
		{
// 		for(QStringList::Iterator it=strList.end();
// 			it!=strList.begin(), i < pWin->m_pBuffer->line()-1; // not exceeeding the last screen
// 			--it, i++)
			--it;
			if(*it!=strTemp)
				continue;
			QStringList::Iterator it2 = it;
			bool dup=true;
			// match more to see if its duplicated
			for(int j=0; j<=i; j++, it2++)
			{
				QString str1 = pWin->stripWhitespace(
								pWin->m_pBuffer->screen(j)->getText());
				if(*it2!=str1)
				{
					dup = false;
					break;
				}
			}
			if(dup)
			{
				// set the start point
				start = i+1;
				break;
			}
		}
		// add new lines
		for(i=start;i<pWin->m_pBuffer->line()-1;i++)
			strList+=pWin->stripWhitespace(
			pWin->m_pBuffer->screen(i)->getText());

		// the end of article
		if( pWin->m_pBuffer->screen(
		pWin->m_pBuffer->line()-1)->getText().indexOf("%") == -1 )
			break;
		// continue
		pWin->m_pTelnet->write(" ", 1);
		if(!pWin->m_wcWaiting.wait(&mutex, 10000))	// timeout
		{
			//qApp->postEvent(pWin, new QCustomEvent(DAE_TIMEOUT));
			emit done(DAE_TIMEOUT);
			break;
		}
		i++;
	}
	#if defined(_OS_WIN32_) || defined(Q_OS_WIN32)
	strArticle = strList.join("\r\n");
	#else
	strArticle = strList.join("\n");
	#endif
	//qApp->postEvent(pWin, new QCustomEvent(DAE_FINISH));
	emit done(DAE_FINISH);
	mutex.unlock();
}

/*
void DAThread::run()
{
    QCString cstrCurrent0,cstrCurrent1;
    QCString cstrTemp;

    int pos0=-1,pos1=-1;

    cstrArticle =pWin->stripWhitespace(pWin->m_pBuffer->screen(0)->getText());
    #if defined(_OS_WIN32_) || defined(Q_OS_WIN32)
    cstrArticle += '\r';
    #endif
    cstrArticle += '\n';
	
    while(1)
    {
        cstrCurrent0=pWin->stripWhitespace(pWin->m_pBuffer->screen(0)->getText());
        cstrCurrent1=pWin->stripWhitespace(pWin->m_pBuffer->screen(1)->getText());

        cstrTemp = cstrCurrent0;
        #if defined(_OS_WIN32_) || defined(Q_OS_WIN32)
        cstrTemp += '\r';
        #endif
        cstrTemp +='\n';
        cstrTemp +=cstrCurrent1;

        pos0=cstrArticle.findRev(cstrTemp);

        if(pos0!=-1)
        {
            pos1=cstrArticle.find(cstrCurrent1,pos0);
            if(pos1!=-1)
                cstrArticle.truncate(pos1);
        }

        for(int i=1;i<pWin->m_pBuffer->line()-1;i++)
        {
            cstrArticle+=pWin->stripWhitespace(pWin->m_pBuffer->screen(i)->getText());
            #if defined(_OS_WIN32_) || defined(Q_OS_WIN32)
            cstrArticle += '\r';
            #endif
            cstrArticle+='\n';
        }

        if( pWin->m_pBuffer->screen(pWin->m_pBuffer->line()-1)->getText().find("%") == -1 )
            break;
        pWin->m_pTelnet->write(" ", 1);
		
		if(!pWin->m_wcWaiting.wait(10000))	// timeout
		{
			postEvent(pWin, new QCustomEvent(DAE_TIMEOUT));
			break;
		}
    }

	postEvent(pWin, new QCustomEvent(DAE_FINISH));
}
*/

char Window::direction[][5]=
{
	// 4
	"\x1b[1~",	// 0 HOME
	"\x1b[4~",	// 1 END
	"\x1b[5~",	// 2 PAGE UP
	"\x1b[6~",	// 3 PAGE DOWN
	// 3
	"\x1b[A",	// 4 UP
	"\x1b[B",	// 5 DOWN
	"\x1b[D",	// 6 LEFT
	"\x1b[C"	// 7 RIGHT
};

//constructor
Window::Window( Frame * frame, Param param, int addr, QWidget * parent, const char * name, Qt::WFlags wflags )
    : QMainWindow( parent, wflags ),m_strMessage(),location()
{

	m_pFrame = frame;
	m_param = param;
	m_nAddrIndex = addr;
	m_hostInfo = NULL;
	QString pathLib = Global::instance()->pathLib();
	setMouseTracking( true );

//init the textline list

	m_pBuffer = new Buffer( m_param.m_nRow, m_param.m_nCol, m_param.m_nScrollLines );
	if (param.m_nProtocolType == 0)
		m_pTelnet = new Telnet( m_param.m_strTerm.toLatin1(), 
						m_param.m_nRow, m_param.m_nCol, false );
	else {
		#ifndef SSH_ENABLED
		QMessageBox::warning(this, "sorry", 
						"SSH support is not compiled, QTerm can only use Telnet!");
		m_pTelnet = new Telnet( m_param.m_strTerm.toUtf8(), 
						m_param.m_nRow, m_param.m_nCol,false );
		#else
		m_pTelnet = new Telnet( m_param.m_strTerm.toUtf8(), 
						m_param.m_nRow, m_param.m_nCol,true );
		#endif
	}
	connect( m_pBuffer, SIGNAL(windowSizeChanged(int,int)), 
					m_pTelnet, SLOT(windowSizeChanged(int,int)) );
	m_pZmDialog = new zmodemDialog(this);
	m_pZmodem = new Zmodem( m_pTelnet, param.m_nProtocolType);
	m_pDecode = new Decode( m_pBuffer );
	m_pBBS	  = new BBS( m_pBuffer );
	m_pScreen = new Screen( this, m_pBuffer, &m_param, m_pBBS );

	m_pIPLocation = new IPLocation(pathLib);
	m_pMessage = new PageViewMessage(this);
	m_bCheckIP = m_pIPLocation->haveFile();
	m_pSound = NULL;

	setFocusProxy( m_pScreen);
	setCentralWidget( m_pScreen);
	connect(m_pFrame, SIGNAL(bossColor()), m_pScreen, SLOT(bossColor()));
	connect(m_pFrame, SIGNAL(scrollChanged()), m_pScreen, SLOT(updateScrollBar()));
	connect(m_pScreen, SIGNAL(inputEvent(QString *)), this, SLOT(inputHandle(QString *)));
	connect(m_pZmodem, SIGNAL(ZmodemState(int,int,const QByteArray&)), 
					this, SLOT(ZmodemState(int,int,const QByteArray&)));
	connect(m_pZmDialog, SIGNAL(canceled()), m_pZmodem, SLOT(zmodemCancel()));

	connect(m_pDecode, SIGNAL(mouseMode(bool)), this, SLOT(setMouseMode(bool)));

	m_popWin = new popWidget(this,m_pFrame);

	m_pMessage->display(tr("Not Connected"));
	statusBar()->setSizeGripEnabled(false);

	if(Global::instance()->showStatusBar())
		statusBar()->show();
	else
		statusBar()->hide();

	// disable the dock menu
// 	setDockMenuEnabled(false);

	m_pUrl = new QMenu(m_pScreen);
	/*
	m_pUrl->insertItem( tr("Preview image"), this, SLOT(previewLink()) );
	m_pUrl->insertItem( tr("Open link"), this, SLOT(openLink()) );
	m_pUrl->insertItem( tr("Copy link address"), this, SLOT(copyLink()) );
	m_pUrl->insertItem( tr("Save target as..."), this, SLOT(saveLink()) );
	*/

	m_pUrl->addAction( tr("Preview image"), this, SLOT(previewLink()) );
	m_pUrl->addAction( tr("Open link"), this, SLOT(openLink()) );
	m_pUrl->addAction( tr("Copy link address"), this, SLOT(copyLink()) );

	m_pMenu = new QMenu(m_pScreen);
	//FIXME: shortcut
	//m_pMenu->insertItem( QPixmap(pathLib+"pic/copy.png"), tr("Copy"), this, SLOT(copy()), Qt::CTRL+Qt::Key_Insert );
	//m_pMenu->insertItem( QPixmap(pathLib+"pic/paste.png"), tr("Paste"), this, SLOT(paste()), Qt::SHIFT+Qt::Key_Insert );
	//m_pMenu->insertItem( QPixmap(pathLib+"pic/article.png"), tr("Copy Article"), this, SLOT(copyArticle()), Qt::Key_F9 );
	m_pMenu->addAction( QPixmap(pathLib+"pic/copy.png"), tr("Copy"), this, SLOT(copy()) );
	m_pMenu->addAction( QPixmap(pathLib+"pic/paste.png"), tr("Paste"), this, SLOT(paste()) );
	m_pMenu->addAction( QPixmap(pathLib+"pic/article.png"), tr("Copy Article"), this, SLOT(copyArticle()) );
	m_pMenu->addSeparator();
	m_pMenu->addAction( QPixmap(pathLib+"pic/fonts.png"), tr("Font"), this, SLOT(font()) );
	m_pMenu->addAction( QPixmap(pathLib+"pic/color.png"), tr("Color"), this, SLOT(color()) );
	m_pMenu->addSeparator();
	m_pMenu->addAction( QPixmap(pathLib+"pic/pref.png"), tr("Setting"), this, SLOT(setting()) );

//connect telnet signal to slots
	connect(m_pTelnet,SIGNAL( readyRead(int) ),
		this,SLOT( readReady(int) ) );
	connect(m_pTelnet,SIGNAL( TelnetState(int) ),
		this,SLOT( TelnetState(int) ) );
	connect(m_pFrame, SIGNAL(statusBarChanged(bool)),
		this, SLOT(showStatusBar(bool)) );
// timers 
	m_idleTimer = new QTimer;
	connect( m_idleTimer, SIGNAL(timeout()), this, SLOT(idleProcess()) );
	m_replyTimer = new QTimer;
	connect( m_replyTimer, SIGNAL(timeout()), this, SLOT(replyProcess()) );
	m_tabTimer = new QTimer;
	connect( m_tabTimer, SIGNAL(timeout()), this, SLOT(blinkTab()) );
	m_reconnectTimer = new QTimer;
	connect( m_reconnectTimer, SIGNAL(timeout()), this, SLOT(reconnect()) );
	m_ipTimer = new QTimer;
	connect( m_ipTimer, SIGNAL(timeout()), this, SLOT(showIP()) );


// initial varibles
	m_bCopyColor= false;
	m_bCopyRect	= false;
	m_bAntiIdle	= true;
	m_bAutoReply= m_param.m_bAutoReply;
	m_bBeep		= !(Global::instance()->m_pref.strPlayer.isEmpty()||Global::instance()->m_pref.strWave.isEmpty());
	m_bMouse	= true;
	m_bWordWrap = false;
	m_bAutoCopy = true;
	m_bMessage	= false;
	m_bReconnect = m_param.m_bReconnect;

	m_pDAThread = 0;
	m_bConnected=false;
	m_bIdling = false;

	m_bSetChanged = false;
	
	m_bMouseX11 = false;
	m_bMouseClicked = false;
#ifdef SSH_ENABLED
	if (param.m_nProtocolType != 0)
		m_bDoingLogin = true;
	else
#endif
		m_bDoingLogin = false;

	cursor[0] = QCursor(QPixmap(pathLib+"cursor/home.xpm"));
	cursor[1] = QCursor(QPixmap(pathLib+"cursor/end.xpm"));
	cursor[2] = QCursor(QPixmap(pathLib+"cursor/pageup.xpm"));
	cursor[3] = QCursor(QPixmap(pathLib+"cursor/pagedown.xpm"));
	cursor[4] = QCursor(QPixmap(pathLib+"cursor/prev.xpm"));
	cursor[5] = QCursor(QPixmap(pathLib+"cursor/next.xpm"));
	cursor[6] = QCursor(QPixmap(pathLib+"cursor/exit.xpm"),0,10);
	cursor[7] = QCursor(QPixmap(pathLib+"cursor/hand.xpm"));
	cursor[8] = Qt::ArrowCursor;

	// the system wide script
	m_bPythonScriptLoaded = false;
#ifdef HAVE_PYTHON
	pModule = NULL;
// python thread module
	// get the global python thread lock
    PyEval_AcquireLock();

    // get a reference to the PyInterpreterState
    extern PyThreadState * mainThreadState;
    PyInterpreterState * mainInterpreterState = mainThreadState->interp;

    // create a thread state object for this thread
    PyThreadState * myThreadState = PyThreadState_New(mainInterpreterState);
    PyThreadState_Swap(myThreadState);

	PyImport_AddModule("qterm");
	Py_InitModule4("qterm", qterm_methods,
			NULL,(PyObject*)NULL,PYTHON_API_VERSION);


	if(m_param.m_bLoadScript && !m_param.m_strScriptFile.isEmpty() )
	{
		PyObject *pName = PyString_FromString( m_param.m_strScriptFile );
		pModule = PyImport_Import(pName);
		Py_DECREF(pName);
		if (pModule != NULL) 
			pDict = PyModule_GetDict(pModule);
		else
		{
			qDebug("Failed to PyImport_Import");
		}

		if(pDict != NULL )
			m_bPythonScriptLoaded = true;
		else
		{
			qDebug("Failed to PyModule_GetDict");
		}
	}
	
	//Clean up this thread's python interpreter reference
    PyThreadState_Swap(NULL);
    PyThreadState_Clear(myThreadState);
    PyThreadState_Delete(myThreadState);
    PyEval_ReleaseLock();
#endif //HAVE_PYTHON
	
	connectHost();
}

//destructor
Window::~Window()
{
	delete m_pTelnet;
	delete m_pBBS;
	delete m_pDecode;
	delete m_pBuffer;
	delete m_pZmodem;
	
	delete m_popWin;
	
	delete m_idleTimer;
	delete m_replyTimer;
	delete m_tabTimer;

	delete m_pMenu;
	delete m_pUrl;
	delete m_pScreen;
	delete m_reconnectTimer;
	delete m_pIPLocation;
	delete m_pMessage;
	delete m_pSound;
	delete m_hostInfo;

#ifdef HAVE_PYTHON
	// get the global python thread lock
	PyEval_AcquireLock();

	// get a reference to the PyInterpreterState
	extern PyThreadState * mainThreadState;
	PyInterpreterState * mainInterpreterState = mainThreadState->interp;
      
	// create a thread state object for this thread
	PyThreadState * myThreadState = PyThreadState_New(mainInterpreterState);
	PyThreadState_Swap(myThreadState);
      
	//Displose of current python module so we can reload in constructor.
	if(pModule!=NULL)
		Py_DECREF(pModule);

	//Clean up this thread's python interpreter reference
	PyThreadState_Swap(NULL);
	PyThreadState_Clear(myThreadState);
	PyThreadState_Delete(myThreadState);
	PyEval_ReleaseLock();
#endif // HAVE_PYTHON
}

//close event received
void Window::closeEvent ( QCloseEvent * clse)
{
	if( m_bConnected && Global::instance()->m_pref.bWarn )
	{
		QMessageBox mb( "QTerm",
			"Connected,Do you still want to exit?",
			QMessageBox::Warning,
			QMessageBox::Yes | QMessageBox::Default,
			QMessageBox::No  | QMessageBox::Escape ,
			0,this);
		if ( mb.exec() == QMessageBox::Yes )
		{
			m_pTelnet->close();
			saveSetting();
			m_pFrame->wndmgr->removeWindow(this);
			clse->accept();
		}
		else
			clse->ignore();
	}
	else
	{
		saveSetting();
		m_pFrame->wndmgr->removeWindow(this);
		clse->accept();
	}
}
/* ------------------------------------------------------------------------ */
/*	                                                                        */
/* 	                         Timer                                          */
/*                                                                          */
/* ------------------------------------------------------------------------ */
void Window::idleProcess()
{
	// do as autoreply when it is enabled
	if(m_replyTimer->isActive() && m_bAutoReply)
	{
		replyMessage();
		if(m_tabTimer->isActive())
	   	{
			m_tabTimer->stop();
			m_pFrame->wndmgr->blinkTheTab(this,TRUE);
    	}
		return;
	}

	m_bIdling = true;
	// system script can handle that
	#ifdef HAVE_PYTHON
	if(pythonCallback("antiIdle",Py_BuildValue("(l)",this)))
		return;
	#endif
	// the default function
	int length;	
	QByteArray cstr = parseString( m_param.m_strAntiString.toLocal8Bit(), &length );
	m_pTelnet->write( cstr, length );
}

void Window::replyProcess()
{
	// if AutoReply still enabled, then autoreply
	if(m_bAutoReply)
		replyMessage();
	else // else just stop the timer
		m_replyTimer->stop();

	if(m_tabTimer->isActive())
    {
		m_tabTimer->stop();
		m_pFrame->wndmgr->blinkTheTab(this,TRUE);
    }
}

void Window::blinkTab()
{
	static bool bVisible=TRUE;
	m_pFrame->wndmgr->blinkTheTab(this,bVisible);
	bVisible=!bVisible;
}

/* ------------------------------------------------------------------------ */
/*	                                                                        */
/* 	                         Mouse & Key                                    */
/*                                                                          */
/* ------------------------------------------------------------------------ */
void Window::enterEvent( QEvent * )
{
}

void Window::leaveEvent( QEvent * )
{
}

void Window::mouseDoubleClickEvent( QMouseEvent * me)
{
	//pythonMouseEvent(3, me->button(), me->state(), me->pos(),0);
}

void Window::mousePressEvent( QMouseEvent * me )
{
	// stop  the tab blinking
    if(m_tabTimer->isActive())
    {
		m_tabTimer->stop();
		m_pFrame->wndmgr->blinkTheTab(this,TRUE);
    }

	// Left Button for selecting
	if(me->button()&Qt::LeftButton && !(me->modifiers()))
	{
		// clear the selected before
		m_pBuffer->clearSelect();
		m_pScreen->m_ePaintState=Screen::NewData;
		m_pScreen->update();
	
		// set the selecting flag
		m_bSelecting = true;
		m_ptSelStart = m_pScreen->mapToChar( me->pos() );
		m_ptSelEnd = m_ptSelStart;
	}

	// Right Button
	if((me->button()&Qt::RightButton))
	{
		if(me->modifiers()&Qt::ControlModifier)
		{
			if(!m_pBBS->getUrl().isEmpty())		// on Url
				previewLink();
			return;
		}
		
		if(!(me->modifiers()))
		{
			if(!m_pBBS->getUrl().isEmpty())		// on Url
				m_pUrl->popup(me->globalPos());
			else
				m_pMenu->popup(me->globalPos());
			return;
		}
	}
	// Middle Button for paste
	if( me->button()&Qt::MidButton && !(me->modifiers()))
	{
		if( m_bConnected )
			if(!m_pBBS->getUrl().isEmpty())         // on Url
				previewLink();
			else
				pasteHelper(false);
		return;
	}

	// If it is a click, there should be a press event and a release event.
	m_bMouseClicked = true;

	// xterm mouse event
	//sendMouseState(0, me->button(), me->state(), me->pos());

	// python mouse event
	//pythonMouseEvent(0, me->button(), me->state(), me->pos(),0);
}


void Window::mouseMoveEvent( QMouseEvent * me)
{
	// selecting by leftbutton
	if( (me->buttons()&Qt::LeftButton) && m_bSelecting )
	{
        if(me->pos().y()< childrenRect().top())
			m_pScreen->scrollLine(-1);
        if(me->pos().y()> childrenRect().bottom())
			m_pScreen->scrollLine(1);
	
		m_ptSelEnd = m_pScreen->mapToChar( me->pos() );
		if( m_ptSelEnd!=m_ptSelStart )
		{
			m_pBuffer->setSelect( m_ptSelStart, m_ptSelEnd, m_bCopyRect );
			m_pScreen->m_ePaintState=Screen::NewData;
			m_pScreen->update();
		}
	}

	if(m_bMouse && m_bConnected)
	{
		// set cursor pos, repaint if state changed
		QRect rect;
		if( m_pBBS->setCursorPos( m_pScreen->mapToChar(me->pos()),rect ) )
			m_pScreen->repaint( m_pScreen->mapToRect(rect) );
		// judge if URL
		QRect rcOld;
		QRect rcOld_i_dont_need_you;
		QRect rcUrl_leave_me_alone = m_rcUrl;
		bool bUrl=false;
		if(Global::instance()->m_pref.bUrl)
		{
			if (m_pBBS->isIP(rcUrl_leave_me_alone, rcOld_i_dont_need_you) && m_bCheckIP)
			{
				if(rcUrl_leave_me_alone != rcOld_i_dont_need_you)
				{
// 					QString country,city;
// 					QString url = m_pBBS->getIP();
// 					if (m_pIPLocation->getLocation(url, country, city)) {
// 						statusBar()->message( G2U(country + city) );
// 						//m_pMessage->display(G2U(country + city), PageViewMessage::Info, 0);
// 					}
					if (!m_ipTimer->isActive()){
						m_ipTimer->setSingleShot(false);
						m_ipTimer->start(100);
					}
				}
			} else{
				//statusBar()->message("");
				//m_pMessage->hide();
				if (m_ipTimer->isActive())
					m_ipTimer->stop();
			}
			
			if(m_pBBS->isUrl(m_rcUrl, rcOld))
			{
				setCursor(Qt::PointingHandCursor);
				if(m_rcUrl!=rcOld)
				{
					//QToolTip::remove(this, m_pScreen->mapToRect(rcOld));
					//QToolTip::add(this, m_pScreen->mapToRect(m_rcUrl), m_pBBS->getUrl());
				}
				bUrl = true;
			}
			//else
			//QToolTip::remove(this, m_pScreen->mapToRect(rcOld));


		}

		if(!bUrl)
		{
			int nCursorType = m_pBBS->getCursorType(m_pScreen->mapToChar(me->pos()));
			if( nCursorType<=8 && nCursorType>=0 )
				setCursor(cursor[nCursorType]);
		}
	}
	// python mouse event
	//pythonMouseEvent(2, me->button(), me->state(), me->pos(),0);
}

void Window::mouseReleaseEvent( QMouseEvent * me )
{	
	if (!m_bMouseClicked)
		return;
	m_bMouseClicked = false;
	// other than Left Button ignored
	if( !(me->button()&Qt::LeftButton) || (me->modifiers()&Qt::KeyboardModifierMask))
	{
		//pythonMouseEvent(1, me->button(), me->state(), me->pos(),0);
		// no local mouse event
		//sendMouseState(3, me->button(), me->state(), me->pos());
		return;
	}
	
	// Left Button for selecting
	m_ptSelEnd = m_pScreen->mapToChar( me->pos() );
	if( m_ptSelEnd != m_ptSelStart && m_bSelecting )
	{
		m_pBuffer->setSelect( m_ptSelStart, m_ptSelEnd, m_bCopyRect );
		m_pScreen->m_ePaintState=Screen::NewData;
		m_pScreen->update();
		if( m_bAutoCopy )
			copy();
		m_bSelecting = false;
		return;
	}
	m_bSelecting = false;

	
	if(!m_bMouse || !m_bConnected)
		return;

	// url
	if(!m_pBBS->getUrl().isEmpty())
	{
		bool ok;
		QString caption = tr("Open URL");
		QString hint = "url";
		#if (QT_VERSION>=300)
		QString strUrl = QInputDialog::getText(this,caption, hint,
						QLineEdit::Normal, QString(m_pBBS->getUrl()), &ok);
		#else
		QString strUrl = QInputDialog::getText(this,caption, hint, 
						QString(m_pBBS->getUrl()), &ok);
		#endif
		if (ok)
		{
			QString strCmd=Global::instance()->m_pref.strHttp;
			if(strCmd.indexOf("%L")==-1) // no replace
				//QApplication::clipboard()->setText(strUrl);
				strCmd += " \"" + strUrl +"\"";
			else
				strCmd.replace("%L",  "\""+strUrl+ "\"");
				//cstrCmd.replace("%L",  strUrl.toLocal8Bit());
			
            //qDebug()<<"run command " << strCmd;
			//QProcess::startDetached(strCmd);
            //TODO: How to do this in Windows?
			system(strCmd.toUtf8().data());
		}
		return;
	}
	// mouse menu
	int nCursorType = m_pBBS->getCursorType(m_pScreen->mapToChar(me->pos()));
	if( nCursorType<4 && nCursorType>=0 )
		m_pTelnet->write(direction[nCursorType], 4);
	else if( nCursorType<7 && nCursorType>=4 )
		m_pTelnet->write(direction[nCursorType], 3);
	else if( nCursorType==7 )
	{
		char cr = CHAR_CR;
		QRect rc = m_pBBS->getSelectRect();
		switch(m_pBBS->getPageState())
		{
			case 0:
				if( !rc.isEmpty() )
				{
					char cMenu = m_pBBS->getMenuChar();
					m_pTelnet->write( &cMenu, 1 );
					m_pTelnet->write( &cr, 1 );
				}
				break;
			case 1:
				if( !rc.isEmpty() )
				{
					int n = rc.y() - m_pBuffer->caretY();
					// scroll lines
					if( n>0 )
						while( n )
						{
							m_pTelnet->write( direction[5], 4 );
							n--;
						}
					if( n<0 )
					{
						n = -n;
						while( n )
						{
							m_pTelnet->write( direction[4], 4 );
							n--;
						}
					}
					// don't forget to send a CHAR_CR at last to enter in
					m_pTelnet->write( &cr, 1 );
				}
				break;
			default:
				break;
		}
	}
	else if ( nCursorType == 8){
		char cr = CHAR_CR;
		m_pTelnet->write( &cr, 1 );
	}
}

void Window::wheelEvent( QWheelEvent *we)
{
	int j = we->delta()>0 ? 4 : 5;
	if(!(we->modifiers()))
	{
		if(Global::instance()->m_pref.bWheel && m_bConnected)
			m_pTelnet->write(direction[j], sizeof(direction[j]));
		return;
	}

	//pythonMouseEvent(4, Qt::NoButton, we->state(), we->pos(),we->delta());

	//sendMouseState(j, Qt::NoButton, we->state(), we->pos());
}

//keyboard input event
void Window::keyPressEvent( QKeyEvent * e )
{
#ifdef HAVE_PYTHON
	int state=0;
	if(e->state()&Qt::AltModifier)
		state |= 0x08;
	if(e->state()&Qt::ControlModifier)
		state |= 0x10;
	if(e->state()&Qt::ShiftModifier)
		state |= 0x20;
	pythonCallback("keyEvent",
					Py_BuildValue("liii", this, 0, state, e->key()));
#endif
    if ( !m_bConnected )
	{
		if(e->key() == Qt::Key_Return)
			reconnect();
		return;
	}

	// stop  the tab blinking
    if(m_tabTimer->isActive())
    {
		m_tabTimer->stop();
		m_pFrame->wndmgr->blinkTheTab(this,TRUE);
    }

	// message replying
	if(m_replyTimer->isActive())
		m_replyTimer->stop();

	if( e->modifiers() & Qt::MetaModifier )
	{
		if( e->key()>=Qt::Key_A && e->key()<=Qt::Key_Z )
		{	
			char ch = e->key() & 0x1f;
			m_pTelnet->write(&ch,1);
		}
		return;
	}

	// TODO get the input messages
//	if( m_bMessage && e->key()==Key_Return && m_pBuffer->caret().y()==1 )
//	{
//		m_strMessage += m_pBuffer->screen(1)->getText()+"\n";
//	}

	switch( e->key() )
	{
		case Qt::Key_Home:
			m_pTelnet->write( direction[0],4 );
		return;
		case Qt::Key_End:
			m_pTelnet->write( direction[1],4 );
		return;
		case Qt::Key_PageUp:
			m_pTelnet->write( direction[2],4 );
		return;
		case Qt::Key_PageDown:
			m_pTelnet->write( direction[3],4 );
		return;
		case Qt::Key_Up:
			m_pTelnet->write( direction[4],3 );
		return;
		case Qt::Key_Down:
			m_pTelnet->write( direction[5],3 );
		return;
		case Qt::Key_Left:
			m_pTelnet->write( direction[6],3 );
		return;
		case Qt::Key_Right:
			m_pTelnet->write( direction[7],3 );
		return;
		case Qt::Key_Delete: // stupid
			m_pTelnet->write( "\x1b[3~",4 );
		return;
		default:	
			break;
	}


	if( e->text().length() )
	{
		QByteArray cstrTmp = unicode2bbs(e->text());
		m_pTelnet->write( cstrTmp, cstrTmp.length() );
	}
}
/*
void Window::imStartEvent(QIMEvent * e)
{
	m_pScreen->imStartEvent(e);
}

void Window::imComposeEvent(QIMEvent * e)
{
	m_pScreen->imComposeEvent(e);
}

void Window::imEndEvent(QIMEvent * e)
{
	m_pScreen->imEndEvent(e);
}
*/
//connect slot
void Window::connectHost()
{
	if (m_hostInfo == NULL) {
		if (m_param.m_nProtocolType == 0)
			m_hostInfo = new TelnetInfo(m_param.m_strAddr , m_param.m_uPort);
		else {
			#ifndef SSH_ENABLED
			m_hostInfo = new TelnetInfo(m_param.m_strAddr , m_param.m_uPort);
			#else
			SSHInfo * sshInfo = new SSHInfo(m_param.m_strAddr , m_param.m_uPort);
			sshInfo->setUserName(m_param.m_strUser);
			sshInfo->setPassword(m_param.m_strPasswd);
			m_hostInfo = sshInfo;
			#endif
		}
	}

	m_pTelnet->setProxy( m_param.m_nProxyType, m_param.m_bAuth,
			m_param.m_strProxyHost, m_param.m_uProxyPort,
			m_param.m_strProxyUser, m_param.m_strProxyPasswd);
	
	m_pTelnet->connectHost( m_hostInfo );
}
/* ------------------------------------------------------------------------ */
/*	                                                                        */
/* 	                         Telnet State                                   */
/*                                                                          */
/* ------------------------------------------------------------------------ */
//read slot
void Window::readReady( int size )
{
// 	qDebug("readReady");
	//read it
	char * str = new char[size+1];
	m_pTelnet->read( str,size );
	str[size]='\0';

    int raw_size = m_pTelnet->raw_len();
    char * raw_str = new char[raw_size];
    m_pTelnet->read_raw(raw_str, raw_size);

    // read raw buffer
    m_pZmodem->ZmodemRcv( (uchar *)raw_str, raw_size, &(m_pZmodem->info));

if(m_pZmodem->transferstate == notransfer)
{	
	//decode
	m_pDecode->decode(str,size);
	
	if(m_bDoingLogin)
	{
		int n = m_pBuffer->caret().y();
		for(int y=n-5;y<n+5;y++ )
		{
			y=qMax(0,y);
			TextLine *pTextLine=m_pBuffer->screen(y);
			if(pTextLine==NULL)
				continue;
			QString str=pTextLine->getText();
			if(str.indexOf("guest")!=-1 && str.indexOf("new")!=-1)
			{
				doAutoLogin();
				break;
			}
		}
	}
	// page complete when caret at the right corner
	// this works for most but not for all
	TextLine * pTextLine = m_pBuffer->screen(m_pBuffer->line()-1);
	
	QString strText = stripWhitespace(pTextLine->getText());
	if( m_pBuffer->caret().y()==m_pBuffer->line()-1 &&
						m_pBuffer->caret().x()>=strText.length()-1 )
		m_wcWaiting.wakeAll();

	//QToolTip::remove(this, m_pScreen->mapToRect(m_rcUrl));

	// message received
	// 03/19/2003. the caret posion removed as a message judgement
	// because smth.org changed
    if( m_pDecode->bellReceive() ) //&& m_pBuffer->caret().y()==1 )
    {
		if( m_bBeep ) {
			m_pSound = new ExternalSound(Global::instance()->m_pref.strPlayer,
							Global::instance()->m_pref.strWave);
			if (m_pSound)
				m_pSound->play();
			delete m_pSound;
			m_pSound = NULL;
		}
		if(Global::instance()->m_pref.bBlinkTab)
			m_tabTimer->start(500);

		QString strMsg = m_pBBS->getMessage();
		if(!strMsg.isEmpty())
			m_strMessage += strMsg + "\n\n";

		m_bMessage = true;
	
		if(!isActiveWindow() || m_pFrame->wndmgr->activeWindow()!=this)
		{
            if (Global::instance()->dbusExist()) {
                QList<Global::Action> actions;
                actions.append(Global::Show_QTerm);
                Global::instance()->sendDBusNotification("New Message in QTerm", fromBBSCodec(strMsg.toLatin1()),actions);
            } else {
                m_popWin->setText(fromBBSCodec(strMsg.toLatin1()));
                m_popWin->popup();
            }
		}
		if(m_bAutoReply)
		{	
			#ifdef HAVE_PYTHON
			if(!pythonCallback("autoReply",Py_BuildValue("(l)",this)))
			{
			#endif
			// TODO: save messages
	        if ( m_bIdling )
				replyMessage();
			else
				m_replyTimer->start(m_param.m_nMaxIdle*1000/2);
			#ifdef HAVE_PYTHON
			}
			#endif
		}
		//m_pFrame->buzz();
	}
	
	// set page state
	m_pBBS->setPageState();
	//refresh screen
	m_pScreen->m_ePaintState=Screen::NewData;
	m_pScreen->update();
	
	#ifdef HAVE_PYTHON
	// python 
	pythonCallback("dataEvent",Py_BuildValue("(l)",this));
	#endif
}
	
    //delete the buf
    delete []str;
    delete []raw_str;

    if (m_pZmodem->transferstate == transferstop)
        m_pZmodem->transferstate = notransfer;
}

void Window::ZmodemState(int type, int value, const QString& status)
{
	QString strMsg;
    //to be completed
    switch(type)
    {
        case    RcvByteCount:
				m_pZmDialog->setProgress( value );
                break;
        case    SndByteCount:
				m_pZmDialog->setProgress( value );
                break;
        case    RcvTimeout:
                /* receiver did not respond, aborting */
			strMsg = QString("time out!");
				m_pZmDialog->addErrorLog(strMsg);
                break;
        case    SndTimeout:
                /* value is # of consecutive send timeouts */
			strMsg = QString("time out after trying %1 times").arg(value);
				m_pZmDialog->addErrorLog(strMsg);
                break;
        case    RmtCancel:
                /* remote end has cancelled */
			strMsg =QString("canceled by remote peer %1").arg(status);
				m_pZmDialog->addErrorLog(strMsg);
                break;
        case    ProtocolErr:
                /* protocol error has occurred, val=hdr */
				strMsg = QString("unhandled header %1 at state %2").arg(value).arg(status);
				m_pZmDialog->addErrorLog(strMsg);
                break;
        case    RemoteMessage:
                /* message from remote end */
				strMsg = QString("msg from remote peer: %1").arg(status);
				m_pZmDialog->addErrorLog(strMsg);
                break;
        case    DataErr:
                /* data error, val=error count */
				strMsg = QString("data errors %1").arg(value);
				m_pZmDialog->addErrorLog(strMsg);
                break;
        case    FileErr:
                /* error writing file, val=errno */
                strMsg = QString("falied to write file");
				m_pZmDialog->addErrorLog(strMsg);
                break;
        case    FileBegin:
                /* file transfer begins, str=name */
//                qWarning("starting file %s", status);
				m_pZmDialog->setFileInfo(G2U(status.toLatin1()),value);
				m_pZmDialog->setProgress(0);
				m_pZmDialog->clearErrorLog();
				m_pZmDialog->show();
				#if (QT_VERSION>=0x030200)
				m_pZmDialog->setModal(true);
				#endif
                break;
        case    FileEnd:
                /* file transfer ends, str=name */
//                qWarning("finishing file %s", status);
				m_pZmDialog->hide();
                break;
        case    FileSkip:
               /* file being skipped, str=name */
				strMsg = QString("skipping file %1").arg(status);
				m_pZmDialog->addErrorLog(strMsg);
                break;
    }

}

// telnet state slot
void Window::TelnetState(int state)
{
	switch( state )
	{
	case TSRESOLVING:
		//statusBar()->message( tr("resolving host name") );
		m_pMessage->display( tr("resolving host name") );
		break;
	case TSHOSTFOUND:
		//statusBar()->message( tr("host found") );
		m_pMessage->display( tr("host found") );
		break;
	case TSHOSTNOTFOUND:
		//statusBar()->message( tr("host not found") );
		m_pMessage->display( tr("host not found") );
		connectionClosed();
		break;
	case TSCONNECTING:
		//statusBar()->message( tr("connecting...") );
		m_pMessage->display( tr("connecting...") );
		break;
	case TSHOSTCONNECTED:
		//statusBar()->message( tr("connected") );
		m_pMessage->display( tr("connected") );
		m_bConnected = true;
		m_pFrame->updateMenuToolBar();
		if(m_param.m_bAutoLogin)
			m_bDoingLogin = true;
		break;
	case TSPROXYCONNECTED:
		//statusBar()->message( tr("connected to proxy" ) );
		m_pMessage->display( tr("connected to proxy" ) );
		break;
	case TSPROXYAUTH:
		//statusBar()->message( tr("proxy authentation") );
		m_pMessage->display( tr("proxy authentation") );
		break;
	case TSPROXYFAIL:
		//statusBar()->message( tr("proxy failed") );
		m_pMessage->display( tr("proxy failed") );
		disconnect();
		break;
	case TSREFUSED:
		//statusBar()->message( tr("connection refused") );
		m_pMessage->display( tr("connection refused") );
		connectionClosed();
		break;
	case TSREADERROR:
		//statusBar()->message( tr("error when reading from server") );
		m_pMessage->display( tr("error when reading from server"),PageViewMessage::Error );
		disconnect();
		break;
	case TSCLOSED:
		//statusBar()->message( tr("connection closed") );
		m_pMessage->display( tr("connection closed") );
		connectionClosed();
		if( m_param.m_bReconnect && m_bReconnect )
			reconnectProcess();
		break;
	case TSCLOSEFINISH:
		//statusBar()->message( tr("connection close finished") );
		m_pMessage->display( tr("connection close finished") );
		//connectionClosed();
		break;
	case TSCONNECTVIAPROXY:
		//statusBar()->message( tr("connect to host via proxy") );
		m_pMessage->display( tr("connect to host via proxy") );
		break;
	case TSEGETHOSTBYNAME:
		//statusBar()->message( tr("error in gethostbyname") );
		m_pMessage->display( tr("error in gethostbyname"), PageViewMessage::Error );
		connectionClosed();
		break;
	case TSEINIWINSOCK:
		//statusBar()->message( tr("error in startup winsock") );
		m_pMessage->display( tr("error in startup winsock"), PageViewMessage::Error );
		connectionClosed();
		break;
	case TSERROR:
		//statusBar()->message( tr("error in connection") );
		m_pMessage->display( tr("error in connection"), PageViewMessage::Error );
		disconnect();
		break;
	case TSPROXYERROR:
		//statusBar()->message( tr("eoor in proxy") );
		m_pMessage->display( tr("error in proxy"), PageViewMessage::Error );
		disconnect();
		break;
	case TSWRITED:
		// restart the idle timer	
		if(m_idleTimer->isActive())
			m_idleTimer->stop();
		if( m_bAntiIdle )
			m_idleTimer->start( m_param.m_nMaxIdle*1000 );
		m_bIdling = false;
		break;
	default:
		break;
	}

}
/* ------------------------------------------------------------------------ */
/*	                                                                        */
/* 	                         UI Slots                                       */
/*                                                                          */
/* ------------------------------------------------------------------------ */


void Window::copy( )
{
	QClipboard *clipboard = QApplication::clipboard();
	
	#if (QT_VERSION>=0x030100)
   // as most users complain they cant copy from qterm to other program by context ment, we copy
   // the text to clipboard except mouse selection
	if( m_param.m_nBBSCode==0 )
	{
		clipboard->setText(G2U(m_pBuffer->getSelectText(m_bCopyRect, m_bCopyColor, 
				   parseString((const char *)m_param.m_strEscape.toLatin1()))), 
					QClipboard::Clipboard );
		clipboard->setText(G2U(m_pBuffer->getSelectText(m_bCopyRect, m_bCopyColor, 
				   parseString((const char *)m_param.m_strEscape.toLatin1()))), 
					QClipboard::Selection );
	}
	else
	{
		clipboard->setText(B2U(m_pBuffer->getSelectText(m_bCopyRect, m_bCopyColor, 
				   parseString((const char *)m_param.m_strEscape.toLatin1()))), 
					QClipboard::Clipboard );
		clipboard->setText(B2U(m_pBuffer->getSelectText(m_bCopyRect, m_bCopyColor, 
				   parseString((const char *)m_param.m_strEscape.toLatin1()))), 
					QClipboard::Selection );
	}
	#else
	if( m_param.m_nBBSCode==0 )
		clipboard->setText(G2U(m_pBuffer->getSelectText(m_bCopyRect, m_bCopyColor, 
										 parseString((const char *)m_param.m_strEscape))));
	else
		clipboard->setText(B2U(m_pBuffer->getSelectText(m_bCopyRect, m_bCopyColor, 
										 parseString((const char *)m_param.m_strEscape))));
	#endif
}

void Window::paste()
{
	pasteHelper(true);
}

void Window::pasteHelper( bool clip )
{
	if( !m_bConnected )
		return;
	
	QClipboard *clipboard = QApplication::clipboard();
	QByteArray cstrText;
	
	if(Global::instance()->clipCodec()==Global::GBK)
	{
		#if (QT_VERSION>=0x030100)
		if(clip)
			cstrText=U2G( clipboard->text(QClipboard::Clipboard) );
		else
			cstrText=U2G( clipboard->text(QClipboard::Selection) );
		#else
		cstrText=U2G( clipboard->text() );
		#endif
		if( m_param.m_nBBSCode==1 )
		{
			char * str = m_converter.G2B( cstrText, cstrText.length() );
			cstrText = str;
			delete []str;
		}
	}
	else
	{
		#if (QT_VERSION>=0x030100)
		if(clip)
			cstrText=U2B( clipboard->text(QClipboard::Clipboard) );
		else
			cstrText=U2B( clipboard->text(QClipboard::Selection) );
		#else
		cstrText=U2B( clipboard->text() );
		#endif
		if( m_param.m_nBBSCode==0 )
		{
			char * str = m_converter.B2G( cstrText, cstrText.length() );
			cstrText = str;
			delete []str;
		}
	}

	if(!Global::instance()->escapeString().isEmpty())
	#if (QT_VERSION>=0x030100)
		cstrText.replace(parseString(Global::instance()->escapeString().toLatin1()), 
				 parseString((const char *)m_param.m_strEscape.toLatin1()));
	#else
		cstrText.replace(QRegExp(parseString(m_pFrame->m_cstrEscape)), 
						parseString((const char *)m_param.m_strEscape));
	#endif

	if( m_bWordWrap )
	{
		// convert to unicode for word wrap
		QString strText;
		if( m_param.m_nBBSCode==0 )
			strText = G2U(cstrText);
		else
			strText = B2U(cstrText);

		// insert '\n' as needed
		for( uint i=0; i< strText.length(); i++ )
		{
			uint j=i;
			uint k=0, l=0;
			while(strText.at(j)!=QChar('\n') && j<strText.length())
			{
				if(Global::instance()->m_pref.nWordWrap-(l-k)>=0 &&
					Global::instance()->m_pref.nWordWrap-(l-k)<2)
				{
					strText.insert(j,QChar('\n'));
					k=l;
					j++;
					break;
				}
				// double byte or not
				if(strText.at(j).row()=='\0')
					l++;
				else
					l+=2;
				j++;
			}
			i = j;
		}

		if( m_param.m_nBBSCode==0 )
			cstrText = U2G(strText);
		else
			cstrText = U2B(strText);
	}
	
	m_pTelnet->write(cstrText, cstrText.length());
}
void Window::copyArticle( )
{
	//return;

	if(!m_bConnected)
		return;
	
	m_pDAThread = new DAThread(this);
	connect( m_pDAThread, SIGNAL(done(int)), this, SLOT(jobDone(int)));
	m_pDAThread->start();
		
}
void Window::font()
{		
	bool ok;

	QResizeEvent* re =new QResizeEvent( m_pScreen->size(),m_pScreen->size());
	
	QFont font=QFontDialog::getFont( &ok,m_pScreen->getDispFont() );
	if(ok==true)
	{
		m_pScreen->setDispFont( font );
		QApplication::postEvent( m_pScreen,re );
	}
}


void Window::color()
{
	addrDialog set(this, true);
	
	set.param = m_param;
	set.updateData(false);		 
	set.ui.tabWidget->setCurrentIndex(1);

	if(set.exec()==1)
	{
		m_param = set.param;
		m_bSetChanged = true;

		QResizeEvent* re =new QResizeEvent( m_pScreen->size(),m_pScreen->size());
		m_pScreen->setSchema();
		QApplication::postEvent( m_pScreen,re );
	}

}

void Window::disconnect()
{
	m_pTelnet->close();
}

void Window::reconnect()
{
	if(!m_bConnected)
		connectHost();

}

void Window::showIP()
{
	QString country,city;
	QString url = m_pBBS->getIP();
	if (m_pIPLocation->getLocation(url, country, city)) {
		m_pMessage->display(G2U((country + city).toLatin1()), PageViewMessage::Info, 100);
	}
}

void Window::refresh( )
{
	//m_pScreen->repaint(true);
	m_pScreen->m_ePaintState=Screen::Show;
	m_pScreen->update();
}

void Window::showStatusBar(bool bShow)
{
	if(bShow)
		statusBar()->show();
	else
		statusBar()->hide();
}

void Window::runScript()
{
	// get the previous dir
	QString file = Global::instance()->getOpenFileName("Python File (*.py *.txt)", this);

	if(file.isEmpty())
		return;

	runScriptFile(file.toLocal8Bit());
}

void Window::stopScript()
{
}

void Window::viewMessages( )
{
	msgDialog msg(this);
	const char * size = Global::instance()->fileCfg()->getItemValue("global","msgdialog").toString().toLatin1();
	if(size!=NULL)
	{
		int x,y,cx,cy;
		sscanf(size,"%d %d %d %d",&x,&y,&cx,&cy);
		msg.resize(QSize(cx,cy));
		msg.move(QPoint(x,y));	
	}

	if( m_param.m_nBBSCode==0 ) 
		msg.ui.msgBrowser->setPlainText(G2U(m_strMessage.toLatin1()));
	else
		msg.ui.msgBrowser->setPlainText(B2U(m_strMessage.toLatin1()));
	msg.exec();

	QString strSize=QString("%1 %2 %3 %4").arg(msg.x()).arg(msg.y()).arg(msg.width()).arg(msg.height());
	Global::instance()->fileCfg()->setItemValue("global","msgdialog",strSize);
	Global::instance()->fileCfg()->save();

}

void Window::setting( )
{
	addrDialog set(this, true);
	
	set.param = m_param;
	set.updateData(false);

	if(set.exec()==1)
	{
		m_param = set.param;
		m_bSetChanged = true;

	}
}

void Window::antiIdle(bool isEnabled)
{
	m_bAntiIdle = isEnabled;
	// disabled
	if( !m_bAntiIdle && m_idleTimer->isActive() )
		m_idleTimer->stop();
	// enabled
	if( m_bAntiIdle && !m_idleTimer->isActive() )
		m_idleTimer->start(m_param.m_nMaxIdle*1000);
}

void Window::autoReply(bool isEnabled)
{
	m_bAutoReply = isEnabled;
	// disabled
	if( !m_bAutoReply && m_replyTimer->isActive() )
		m_replyTimer->stop();
	// enabled
//	if( m_bAutoReply && !m_replyTimer->isActive() )
//		m_replyTimer->start(m_param.m_nMaxIdle*1000/2);
}

void Window::connectionClosed()
{
	m_bConnected = false;

	if(m_idleTimer->isActive())
		m_idleTimer->stop();
	
	//statusBar()->message( tr("connection closed") );
	m_pMessage->display( tr("connection closed") );

	m_pFrame->updateMenuToolBar();

	setCursor(cursor[8]);
	
	QString strMsg = "";
	strMsg += "\n\n\n\r";
	strMsg += "\x1b[17C\x1b[0m===========================================\n\r"; 
	strMsg += "\x1b[17C Connection Closed, Press \x1b[1m\x1b[31;40mEnter\x1b[m\x1b[0m To Connect\n\r"; 
	strMsg += "\x1b[17C===========================================\n"; 

	m_pDecode->decode( strMsg.toLatin1(), strMsg.length() );
	m_pScreen->m_ePaintState=Screen::NewData;
	m_pScreen->update();
}

void Window::doAutoLogin()
{
	if( !m_param.m_strPreLogin.isEmpty() ) 
	{
		QByteArray temp = parseString(m_param.m_strPreLogin.toLatin1());
		m_pTelnet->write( (const char *)(temp), temp.length() );
	}
	if( !m_param.m_strUser.isEmpty() ) 
	{
		QByteArray temp = m_param.m_strUser.toLocal8Bit();
		m_pTelnet->write( (const char *)(temp), temp.length() );
		char ch=CHAR_CR;
		m_pTelnet->write( &ch, 1 );
	}
	if( !m_param.m_strPasswd.isEmpty() ) 
	{
		QByteArray temp = m_param.m_strPasswd.toLocal8Bit();
		m_pTelnet->write( (const char *)(temp), temp.length() );
		char ch=CHAR_CR;
		m_pTelnet->write( &ch, 1 );
	}
	// smth ignore continous input, so sleep 1 sec :)
	#if defined(_OS_WIN32_) || defined(Q_OS_WIN32)
	Sleep(1);
	#else
	sleep(1);
	#endif

	if( !m_param.m_strPostLogin.isEmpty() ) 
	{
		QByteArray temp = parseString(m_param.m_strPostLogin.toLatin1());
		m_pTelnet->write( (const char *)(temp), temp.length() );
	}
	m_bDoingLogin = false;
}

void Window::reconnectProcess()
{
	static int retry = 0;
	if( retry<m_param.m_nRetry || m_param.m_nRetry==-1 )
	{
		if( m_param.m_nReconnectInterval<=0)
			reconnect();
		else
			m_reconnectTimer->start(m_param.m_nReconnectInterval*1000);
		retry++;
	}
}
/* ------------------------------------------------------------------------ */
/*	                                                                        */
/* 	                         Events                                         */
/*                                                                          */
/* ------------------------------------------------------------------------ */

void Window::jobDone(int e)
{
	if( e == DAE_FINISH )
	{
		articleDialog article(this);
		const char * size = Global::instance()->fileCfg()->getItemValue("global","articledialog").toString().toLatin1().data();
		if(size!=NULL)
		{
			int x,y,cx,cy;
			sscanf(size,"%d %d %d %d",&x,&y,&cx,&cy);
			article.resize(QSize(cx,cy));
			article.move(QPoint(x,y));	
		}
		article.strArticle = m_pDAThread->strArticle;
		if(m_param.m_nBBSCode==0)
			article.strArticle = G2U(m_pDAThread->strArticle.toLatin1());
		else
			article.strArticle = B2U(m_pDAThread->strArticle.toLatin1());
		article.ui.textBrowser->setPlainText(article.strArticle);
		article.exec();
		QString strSize = QString("%1 %2 %3 %4").arg(article.x()).arg(article.y()).arg(article.width()).arg(article.height());
		Global::instance()->fileCfg()->setItemValue("global","articledialog",strSize);
		Global::instance()->fileCfg()->save();
	}
	else if(e == DAE_TIMEOUT)
	{
		QMessageBox::warning(this,"timeout","download article timeout, aborted");
	}	
	else if(e == PYE_ERROR)
	{
		QMessageBox::warning(this,"Python script error", m_strPythonError);
	}
	else if(e == PYE_FINISH)
	{
		QMessageBox::information(this,"Python script finished", "Python script file executed successfully");
	}

}

/* ------------------------------------------------------------------------ */
/*	                                                                        */
/* 	                         Aux Func                                       */
/*                                                                          */
/* ------------------------------------------------------------------------ */

QByteArray  Window::parseString(const QByteArray& cstr, int *len)
{
	QByteArray parsed="";

	if(len!=0)	
		*len=0;

	for( uint i=0; i<cstr.length(); i++ )
	{
		if(cstr.at(i)=='^')
		{
			i++;
			if(i<cstr.length())
			{
				parsed += CTRL(cstr.at(i));
				if(len!=0)	
					*len=*len+1;
			}
			
		}else
		if(cstr.at(i)=='\\')
		{
			i++;
			if(i<cstr.length())
			{
				if(cstr.at(i)=='n')
					parsed += CHAR_CR;
				else if(cstr.at(i)=='r')
					parsed += CHAR_LF;
				else if(cstr.at(i)=='t')
					parsed += CHAR_TAB;
				if(len!=0)	
					*len=*len+1;
			}
		}else	
		{	
			parsed += cstr.at(i);
			if(len!=0)	
				*len=*len+1;
		}
	}

	return parsed;
}

void Window::replyMessage()
{
	if(m_replyTimer->isActive())
		m_replyTimer->stop();
	
	QByteArray cstrTmp = m_param.m_strReplyKey.toLocal8Bit();
	QByteArray cstr = parseString(cstrTmp.isEmpty()?QByteArray("^Z"):cstrTmp);
	//cstr += m_param.m_strAutoReply.toLocal8Bit();
	if( m_param.m_nBBSCode==0 )
		cstr += U2G(m_param.m_strAutoReply);
	else
		cstr += U2B(m_param.m_strAutoReply);
	
	cstr += '\n';
	m_pTelnet->write( cstr, cstr.length() );
	m_pMessage->display("You have messages", PageViewMessage::Info, 0);
}

void Window::saveSetting()
{
	if(m_nAddrIndex == -1 || !m_bSetChanged)
		return;

	QMessageBox mb( "QTerm",
		"Setting changed do you want to save it?",
		QMessageBox::Warning,
		QMessageBox::Yes | QMessageBox::Default,
		QMessageBox::No  | QMessageBox::Escape ,
			0,this);
	if ( mb.exec() == QMessageBox::Yes )
	{
		Global::instance()->saveAddress(m_nAddrIndex, m_param);
	}
}

void Window::externInput( const QByteArray& cstrText )
{
	QByteArray cstrParsed = parseString( cstrText );
	m_pTelnet->write( cstrParsed, cstrParsed.length() );

}

QByteArray Window::unicode2bbs(const QString& text)
{
	QByteArray strTmp;

	if( Global::instance()->m_pref.nXIM == 0 )
	{
		strTmp = U2G(text);
		if( m_param.m_nBBSCode == 1)
		{
			char * str = m_converter.G2B( strTmp, strTmp.length() );
			strTmp = str;
			delete []str;
		}
	}
	else
	{
		strTmp = U2B(text);
		if( m_param.m_nBBSCode == 0 )
		{
			char * str = m_converter.B2G( strTmp, strTmp.length() );
			strTmp = str;
			delete []str;
		}
	}

	return strTmp;
}

QString Window::fromBBSCodec( const QByteArray& cstr )
{
	if(m_param.m_nBBSCode==0)
		return G2U(cstr);
	else
		return B2U(cstr);
}

QByteArray Window::stripWhitespace( const QByteArray& cstr )
{
	QString cstrText=QString::fromLatin1(cstr);
	
	#if (QT_VERSION>=300)
	int pos=cstrText.lastIndexOf(QRegExp("[\\S]"));
	#else
	int pos=cstrText.lastIndexOf(QRegExp("[^\\s]"));
	#endif

	if(pos==-1)
		cstrText="";
	else
		cstrText.truncate(pos+1);
	return cstrText.toLatin1();
}

void Window::sendParsedString( const char * str)
{
    int length;
	QByteArray cstr = parseString( str, &length );
	m_pTelnet->write( cstr, length );
}

void Window::setMouseMode(bool on)
{
	m_bMouseX11 = on;
}

/* 0-left 1-middle 2-right 3-relsase 4/5-wheel
 *
 */
//void Window::sendMouseState( int num, 
//				Qt::ButtonState btnstate, Qt::ButtonState keystate, const QPoint& pt )
void Window::sendMouseState( int num, Qt::KeyboardModifier btnstate, Qt::KeyboardModifier keystate, const QPoint& pt)
{
	/*
	if(!m_bMouseX11)
		return;
	
	QPoint ptc = m_pScreen->mapToChar(pt);

	if(btnstate&Qt::LeftButton)
		num = num==0?0:num;
	else if(btnstate&Qt::MidButton)
		num = num==0?1:num;
	else if(btnstate&Qt::RightButton)
		num = num==0?2:num;

	int state = 0;
	if(keystate&Qt::ShiftModifier)
		state |= 0x04;
	if(keystate&Qt::MetaModifier)
		state |= 0x08;
	if(keystate&Qt::ControlModifier)
		state |= 0x10;

	// normal buttons are passed as 0x20 + button,
	// mouse wheel (buttons 4,5) as 0x5c + button
	if(num>3)	num += 0x3c;

	char mouse[10];
	sprintf(mouse, "\033[M%c%c%c", 
			num+state+0x20,
			ptc.x()+1+0x20,
			ptc.y()+1+0x20);
	m_pTelnet->write( mouse, strlen(mouse) );
	*/
}

/* ------------------------------------------------------------------------ */
/*	                                                                        */
/* 	                         Python Func                                    */
/*                                                                          */
/* ------------------------------------------------------------------------ */


void Window::runScriptFile( const QString & cstr )
{
	char str[32];
	sprintf(str,"%ld",this);
	
	char *argv[2]={str,NULL};

#ifdef HAVE_PYTHON
    // get the global python thread lock
    PyEval_AcquireLock();

    // get a reference to the PyInterpreterState
    extern PyThreadState * mainThreadState;
    PyInterpreterState * mainInterpreterState = mainThreadState->interp;

    // create a thread state object for this thread
    PyThreadState * myThreadState = PyThreadState_New(mainInterpreterState);
    PyThreadState_Swap(myThreadState);


	PySys_SetArgv(1, argv);

	runPythonFile(cstr.toLocal8Bit());

	//Clean up this thread's python interpreter reference
    PyThreadState_Swap(NULL);
    PyThreadState_Clear(myThreadState);
    PyThreadState_Delete(myThreadState);
    PyEval_ReleaseLock(); 
#endif // HAVE_PYTHON
}

#ifdef HAVE_PYTHON
bool Window::pythonCallback(const QString & func, PyObject* pArgs)
{
	if(!m_bPythonScriptLoaded) {
		Py_DECREF(pArgs);
		return false;
	};
	
	bool done = false;
	// get the global lock
	 PyEval_AcquireLock();
	// get a reference to the PyInterpreterState
      
	//Python thread references
	extern PyThreadState * mainThreadState;
      
	PyInterpreterState * mainInterpreterState = mainThreadState->interp;
	// create a thread state object for this thread
	PyThreadState * myThreadState = PyThreadState_New(mainInterpreterState);
	PyThreadState_Swap(myThreadState);
    
	PyObject *pF = PyString_FromString(func.toLatin1());
	PyObject *pFunc = PyDict_GetItem(pDict, pF);
 	Py_DECREF(pF);

	if (pFunc && PyCallable_Check(pFunc)) 
	{
		PyObject *pValue = PyObject_CallObject(pFunc, pArgs);
		Py_DECREF(pArgs);
		if (pValue != NULL) 
		{
			done = true;
			Py_DECREF(pValue);
		}
		else 
		{
			QMessageBox::warning(this,"Python script error", getException());
		}
      }
	else 
	{
		PyErr_Print();
		qDebug("Cannot find python %s callback function", func.toLatin1());
	}
      
	// swap my thread state out of the interpreter
	PyThreadState_Swap(NULL);
	// clear out any cruft from thread state object
	PyThreadState_Clear(myThreadState);
   // delete my thread state object
	PyThreadState_Delete(myThreadState);
	// release the lock
	PyEval_ReleaseLock();

	if (func == "autoReply")
		m_pMessage->display("You have messages", PageViewMessage::Info, 0);

	return done;
}
#endif //HAVE_PYTHON
#if 0
int Window::runPythonFile( const char * filename )
{
#ifdef HAVE_PYTHON
    static char buffer[1024];
    const char *file = filename;

    char *p;

    /* Have to do it like this. PyRun_SimpleFile requires you to pass a
     * stdio file pointer, but Vim and the Python DLL are compiled with
     * different options under Windows, meaning that stdio pointers aren't
     * compatible between the two. Yuk.
     *
     * Put the string "execfile('file')" into buffer. But, we need to
     * escape any backslashes or single quotes in the file name, so that
     * Python won't mangle the file name.
	 * ---- kafa
     */
	strcpy(buffer, "def work_thread():\n"
					"\ttry:\n\t\texecfile('");
    p = buffer + 37; /* size of above  */

    while (*file && p < buffer + (1024 - 3))
    {
	if (*file == '\\' || *file == '\'')
	    *p++ = '\\';
	*p++ = *file++;
    }

    /* If we didn't finish the file name, we hit a buffer overflow */
    if (*file != '\0')
	return -1;

    /* Put in the terminating "')" and a null */
    *p++ = '\'';
	*p++ = ',';
	*p++ = '{';
	*p++ = '}';
	*p++ = ')';
    *p++ = '\n';
	*p++ = '\0';

	Q3CString cstr;
	
//	cstr.sprintf("\t\tqterm.formatError(%ld,'')\n",this);
//	strcat(buffer, cstr);
	
	cstr.sprintf("\texcept:\n"
				"\t\texc, val, tb = sys.exc_info()\n"
				"\t\tlines = traceback.format_exception(exc, val, tb)\n"
				"\t\terr = string.join(lines)\n"
				"\t\tprint err\n"
				"\t\tf=open('%s','w')\n"
				"\t\tf.write(err)\n"
				"\t\tf.close()\n"
				,getErrOutputFile(this).data());
	strcat(buffer, cstr);

	cstr.sprintf("\t\tqterm.formatError(%ld)\n",this);
	strcat(buffer, cstr);

	strcat(buffer, "\t\texit\n");

    /* Execute the file */
	PyRun_SimpleString("import thread,string,sys,traceback,qterm");
	PyRun_SimpleString(buffer);
	PyRun_SimpleString(	"thread.start_new_thread(work_thread,())\n");

#endif // HAVE_PYTHON

	return 0;
}
#endif

int Window::runPythonFile( const char * filename )
{
#ifdef HAVE_PYTHON
    static char buffer[1024];
    const char *file = filename;

    char *p;

    /* Have to do it like this. PyRun_SimpleFile requires you to pass a
     * stdio file pointer, but Vim and the Python DLL are compiled with
     * different options under Windows, meaning that stdio pointers aren't
     * compatible between the two. Yuk.
     *
     * Put the string "execfile('file')" into buffer. But, we need to
     * escape any backslashes or single quotes in the file name, so that
     * Python won't mangle the file name.
	 * ---- kafa
     */
	strcpy(buffer, "def work_thread():\n"
					"\ttry:\n\t\texecfile('");
    p = buffer + 37; /* size of above  */

    while (*file && p < buffer + (1024 - 3))
    {
	if (*file == '\\' || *file == '\'')
	    *p++ = '\\';
	*p++ = *file++;
    }

    /* If we didn't finish the file name, we hit a buffer overflow */
    if (*file != '\0')
	return -1;

    /* Put in the terminating "')" and a null */
    *p++ = '\'';
	*p++ = ',';
	*p++ = '{';
	*p++ = '}';
	*p++ = ')';
    *p++ = '\n';
	*p++ = '\0';

	QString str;
	
//	cstr.sprintf("\t\tqterm.formatError(%ld,'')\n",this);
//	strcat(buffer, cstr);
	
	str = QString("\texcept:\n"
				"\t\texc, val, tb = sys.exc_info()\n"
				"\t\tlines = traceback.format_exception(exc, val, tb)\n"
				"\t\terr = string.join(lines)\n"
				"\t\tprint err\n"
				"\t\tf=open('%1','w')\n"
				"\t\tf.write(err)\n"
			"\t\tf.close()\n").arg(getErrOutputFile(this).data());
	strcat(buffer, str.toLatin1());

	str = QString("\t\tqterm.formatError(%1)\n").arg(static_cast<int>(this));
	strcat(buffer, cstr);

	strcat(buffer, "\t\texit\n");

    /* Execute the file */
	PyRun_SimpleString("import string,sys,traceback,qterm");
	PyRun_SimpleString(buffer);
	PyRun_SimpleString("work_thread()\n");

#endif // HAVE_PYTHON

	return 0;
}


//void Window::pythonMouseEvent(int type, Qt::ButtonState btnstate, Qt::ButtonState keystate, 
//				const QPoint& pt, int delta  )
void Window::pythonMouseEvent(int type, Qt::KeyboardModifier, Qt::KeyboardModifier, const QPoint & pt, int delta)
{
	/*
	int state=0;

	if(btnstate&Qt::LeftButton)
		state |= 0x01;
	if(btnstate&Qt::RightButton)
		state |= 0x02;
	if(btnstate&Qt::MidButton)
		state |= 0x04;

	if(keystate&Qt::AltModifier)
		state |= 0x08;
	if(keystate&Qt::ControlModifier)
		state |= 0x10;
	if(keystate&Qt::ShiftModifier)
		state |= 0x20;

	QPoint ptc = m_pScreen->mapToChar(pt);

#ifdef HAVE_PYTHON
	pythonCallback("mouseEvent", 
					Py_BuildValue("liiiii", this, type, state, ptc.x(), ptc.y(),delta));
#endif
*/
}

void Window::inputHandle(QString * text)
{
	if (text->length() > 0) {
		QByteArray cstrTmp = unicode2bbs(*text);
		m_pTelnet->write( cstrTmp, cstrTmp.length() );
	}
}

/* ------------------------------------------------------------------------ */
/*	                                                                        */
/* 	                         HTTP Func                                      */
/*                                                                          */
/* ------------------------------------------------------------------------ */

void Window::openLink()
{
	QString strCmd=Global::instance()->m_pref.strHttp;
	if(strCmd.indexOf("%L")==-1) // no replace
	//QApplication::clipboard()->setText(strUrl);
		strCmd += " \"" + m_pBBS->getUrl() +"\"";
	else
		strCmd.replace(QRegExp("%L",Qt::CaseInsensitive), m_pBBS->getUrl());
	QProcess::startDetached(strCmd);
}

void Window::previewLink()
{	
	getHttpHelper(m_pBBS->getUrl(), true);
}

void Window::copyLink()
{
	QString strUrl;
	if( m_param.m_nBBSCode==0 )
		strUrl=G2U(m_pBBS->getUrl().toLatin1());
	else
		strUrl=B2U(m_pBBS->getUrl().toLatin1());
				
	QClipboard *clipboard = QApplication::clipboard();

	#if (QT_VERSION>=0x030100)
	clipboard->setText(strUrl, QClipboard::Selection);
	clipboard->setText(strUrl, QClipboard::Clipboard);
	#else
	clipboard->setText(strUrl);
	#endif
}

void Window::saveLink()
{
	getHttpHelper(m_pBBS->getUrl(), false);
}

void Window::getHttpHelper(const QString& strUrl, bool bPreview)
{
	Http *pHttp = new Http(this);
	connect(pHttp, SIGNAL(done(QObject*)), this, SLOT(httpDone(QObject*)));
	connect(pHttp, SIGNAL(message(const QString &)), m_pMessage, SLOT(showText(const QString &)));
	pHttp->getLink(strUrl, bPreview);
}

void Window::httpDone(QObject *pHttp)
{
	pHttp->deleteLater();
}
}
#include <qtermwindow.moc>
