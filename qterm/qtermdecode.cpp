/*******************************************************************************
FILENAME:      qtermdecode.cpp
REVISION:      2001.8.12 first created.
         
AUTHOR:        kingson fiasco
*******************************************************************************/
/*******************************************************************************
                                    NOTE
 This file may be used, distributed and modified without limitation.
 *******************************************************************************/
#include "qtermdecode.h"

#include "qterm.h"
#include "qtermbuffer.h"


#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <qcstring.h>

#define MODE_MouseX11	0

/************************************************************************/
// state for FSM
// please read ANSI decoding
StateOption QTermDecode::normalState[] =
{
    { CHAR_CR, 		&QTermDecode::cr,			normalState },
    { CHAR_LF, 		&QTermDecode::lf,			normalState },
    { CHAR_FF, 		&QTermDecode::ff,			normalState },
    { CHAR_TAB, 	&QTermDecode::tab,			normalState },
    { CHAR_BS,  	&QTermDecode::bs,			normalState },
    { CHAR_BELL,  	&QTermDecode::bell,			normalState },
    { CHAR_ESC, 	0,							escState    },
    { CHAR_NORMAL, 	&QTermDecode::normalInput, 	normalState }
};

// state after a ESC_CHAR
// only for BBS, so I reduce a lots
StateOption QTermDecode::escState[] =
{
    { '[', 		&QTermDecode::clearParam,  		bracketState },
	// VT52
	{ 'A', 		&QTermDecode::cursorUp,			normalState	},
    { 'B',		&QTermDecode::cursorDown,		normalState	},
    { 'C', 		&QTermDecode::cursorRight,		normalState },
    { 'D', 		&QTermDecode::cursorLeft,		normalState },
    { 'J', 		&QTermDecode::eraseScreen,		normalState },
    { 'K', 		&QTermDecode::eraseLine,		normalState },
	{ 'Z',		&QTermDecode::test,				normalState	},
	{ '>',		&QTermDecode::test,				normalState	},
	{ '<',		&QTermDecode::test,				normalState	},
    { CHAR_NORMAL, 	0,							normalState	}
};

// state after ESC [
StateOption QTermDecode::bracketState[] =
{
    { '0', 		&QTermDecode::paramDigit,		bracketState },
    { '1', 		&QTermDecode::paramDigit,		bracketState },
    { '2', 		&QTermDecode::paramDigit,		bracketState },
    { '3', 		&QTermDecode::paramDigit,		bracketState },
    { '4', 		&QTermDecode::paramDigit,		bracketState },
    { '5',		&QTermDecode::paramDigit,		bracketState },
    { '6', 		&QTermDecode::paramDigit,		bracketState },
    { '7', 		&QTermDecode::paramDigit,		bracketState },
    { '8', 		&QTermDecode::paramDigit,		bracketState },
    { '9', 		&QTermDecode::paramDigit,		bracketState },
    { ';', 		&QTermDecode::nextParam,		bracketState },

	{ '?',		&QTermDecode::clearParam,		privateState},
	
    { 'A', 		&QTermDecode::cursorUp,			normalState },
    { 'B', 		&QTermDecode::cursorDown,		normalState },
    { 'C', 		&QTermDecode::cursorRight,		normalState },
    { 'D', 		&QTermDecode::cursorLeft,		normalState },
    { 'H', 		&QTermDecode::cursorPosition,  	normalState },
    { 'J', 		&QTermDecode::eraseScreen,		normalState },
    { 'K', 		&QTermDecode::eraseLine,		normalState },
    { 'L', 		&QTermDecode::insertLine,		normalState },
    { 'M', 		&QTermDecode::deleteLine,		normalState },
    { 'P', 		&QTermDecode::deleteStr,		normalState },
    { 'X', 		&QTermDecode::eraseStr,			normalState },
    { 'f', 		&QTermDecode::cursorPosition,  	normalState },
    { 'h',		&QTermDecode::setMode,			normalState },
    { 'l',		&QTermDecode::resetMode,		normalState },
    { 'm', 		&QTermDecode::getAttr,			normalState },
    { 'r',		&QTermDecode::setMargins,		normalState },
    { 's', 		&QTermDecode::saveCursor,		normalState },
    { 'u', 		&QTermDecode::restoreCursor,	normalState },
    { '@', 		&QTermDecode::insertStr,		normalState },

    { CHAR_CR, 		&QTermDecode::cr,			bracketState },
    { CHAR_LF, 		&QTermDecode::lf,			bracketState },
    { CHAR_FF, 		&QTermDecode::ff,			bracketState },
    { CHAR_TAB,  	&QTermDecode::tab,			bracketState },
    { CHAR_BS, 	 	&QTermDecode::bs,			bracketState },
    { CHAR_BELL, 	&QTermDecode::bell,			bracketState },
    { CHAR_NORMAL, 	0,							normalState }
};

// state after ESC [ ?

StateOption QTermDecode::privateState[] =
{
	{ '0', 		&QTermDecode::paramDigit,			privateState },
    { '1', 		&QTermDecode::paramDigit,			privateState },
    { '2', 		&QTermDecode::paramDigit,			privateState },
    { '3', 		&QTermDecode::paramDigit,			privateState },
    { '4', 		&QTermDecode::paramDigit,			privateState },
    { '5',		&QTermDecode::paramDigit,			privateState },
    { '6', 		&QTermDecode::paramDigit,			privateState },
    { '7', 		&QTermDecode::paramDigit,			privateState },
    { '8', 		&QTermDecode::paramDigit,			privateState },
    { '9', 		&QTermDecode::paramDigit,			privateState },
    { ';', 		&QTermDecode::nextParam,			privateState },

	{ 'h',		&QTermDecode::setMode,				normalState	 },
	{ 'l',		&QTermDecode::resetMode,			normalState	 },
	{ 's',		&QTermDecode::saveMode,				normalState	 },
	{ 'r',		&QTermDecode::restoreMode,			normalState	 },

	{ CHAR_NORMAL, 	0,								normalState  }
};

QTermDecode::QTermDecode( QTermBuffer * buffer )
{
	m_pBuffer = buffer;
	
	currentState = /*QTermDecode::*/normalState;
	
	m_defAttr = SETCOLOR( /*0x4b*/NO_COLOR ) | SETATTR( NO_ATTR );

	m_curAttr = m_defAttr;

	m_pBuffer->setCurAttr( m_curAttr );

	bCurMode[MODE_MouseX11]=bSaveMode[MODE_MouseX11]=false;
}

QTermDecode::~QTermDecode()
{
}

// precess input string from telnet socket
//void QTermDecode::ansiDecode( const QCString &cstr, int length )
void QTermDecode::decode( const char *cstr, int length )
{	
	inputData = cstr;
	inputLength = length;//inputData.length();

	dataIndex = 0;
	m_bBell = false;
	
	int i;
	StateOption *lastState;

	m_pBuffer->startDecode();

	// here we use FSM to ANSI decoding
	// use switch case is ok too
	// but i think use function pointer array can make this clear
	// you can see the defination at the beginning
	while ( dataIndex < inputLength )	
	{
		// current state always be initialized to point to the deginning of three structures
		// ( normalState, escState, bracketState )
		i = 0;
		while ( currentState[i].byte != CHAR_NORMAL 
			&& currentState[i].byte != inputData[dataIndex] )
			   i++;

		// action must be allowed to redirect state change
		// get current state with input character i ( hehe, current now become last one )
		lastState = currentState + i;	// good !!

		if ( lastState->action != 0 )
			( this->*( lastState->action ) )();
		
		// reinit current state	
		currentState = lastState->nextState;

		dataIndex++;
	}
	m_pBuffer->endDecode();
}
	
// fill letters into char buffer
void QTermDecode::normalInput()
{
	if ( inputData[dataIndex] < 0x20 && inputData[dataIndex] >= 0x00 )	// not print char
		return;
	
	int n = 0;
	while ( ( inputData[dataIndex + n] >= 0x20 || inputData[dataIndex + n] < 0x00 ) 
		&& ( dataIndex + n ) < inputLength )
		n++;

	QCString cstr( inputData + dataIndex, n + 1 );

	m_pBuffer->setBuffer( cstr, n ); 
	
	n--;
	dataIndex += n;

}

// non-printing characters functions
void QTermDecode::cr()
{
	// FIXME: dirty
	m_pBuffer->cr();
}

void QTermDecode::lf()
{
	m_pBuffer->newLine();
}

void QTermDecode::ff()
{
	m_pBuffer->eraseEntireScreen();
	
	m_pBuffer->moveCursor( 0, 0 );
}

void QTermDecode::tab()
{
	m_pBuffer->tab();
}

void QTermDecode::bs()
{
	m_pBuffer->moveCursorOffset( - 1, 0 );
}

void QTermDecode::bell()
{
	m_bBell = true;
}

void QTermDecode::setMargins()
{
	m_pBuffer->setMargins( param[0], param[1] );
}

void QTermDecode::nextLine()
{
	cr();
	lf();
}

// parameters functions
void QTermDecode::clearParam()
{
	nParam = 0;
	memset( param, 0, sizeof(param) );
	bParam = false;
}

// for performance, this grabs all digits
void QTermDecode::paramDigit()
{
	bParam = true;
	
	// make stream into number
	// ( e.g. this input character is '1' and this param is 4
	// after the following sentence this param is changed to 41
	param[nParam] = param[nParam]*10 + inputData[dataIndex] - '0';
}

void QTermDecode::nextParam()
{
	nParam++;
}

void QTermDecode::saveCursor()
{
	m_pBuffer->saveCursor();
}

void QTermDecode::restoreCursor()
{
	m_pBuffer->restoreCursor();
}

void QTermDecode::cursorLeft()
{
	int n = param[0];

	if ( n < 1 )
		n = 1;

	m_pBuffer->moveCursorOffset( -n, 0 );
}

void QTermDecode::cursorRight()
{
	int n = param[0];

	if ( n < 1 )
		n = 1;

	m_pBuffer->moveCursorOffset( n, 0 );
}

void QTermDecode::cursorUp()
{
	int n = param[0];

	if ( n < 1 )
		n = 1;

	m_pBuffer->moveCursorOffset( 0, -n );
}

void QTermDecode::cursorDown()
{
	int n = param[0];

	if ( n < 1)
		n = 1;

	m_pBuffer->moveCursorOffset( 0, n );
}

void QTermDecode::cursorPosition()
{
	int x = param[1];	
	int y = param[0];	
	
	if(x == 0) x = 1;
	if(y == 0) y = 1;

	m_pBuffer->moveCursor( x - 1, y - 1 );
}

// erase functions
void QTermDecode::eraseStr()
{
	int n = param[0];

	if ( n < 1 )
		n = 1;
		
	m_pBuffer->eraseStr( n );
}

// insert functions
void QTermDecode::insertStr()
{
	int n = param[0];

	if ( n < 1 )
		n = 1;

	m_pBuffer->insertStr( n );
}
// delete functions
void QTermDecode::deleteStr()
{
	int n = param[0];

	if ( n < 1 )
		n = 1;

	m_pBuffer->deleteStr( n );
}


void QTermDecode::eraseLine()
{
	switch ( param[0] )
	{
	case 0:
		m_pBuffer->eraseToEndLine();
		break;
	case 1:
		m_pBuffer->eraseToBeginLine();
		break;
	case 2:
		m_pBuffer->eraseEntireLine();
		break;
	default:
		break;
	}
}

void QTermDecode::insertLine()
{

	int n = param[0];

	if ( n < 1 )
		n = 1;
		
	m_pBuffer->insertLine( n );
}

void QTermDecode::deleteLine()
{
	int n = param[0];

	if ( n < 1 )
		n = 1;
		
	m_pBuffer->deleteLine( n );
}

void QTermDecode::eraseScreen()
{
	switch ( param[0] )
	{
	case 0:
		m_pBuffer->eraseToEndScreen();
		break;
	case 1:
		m_pBuffer->eraseToBeginScreen();
		break;
	case 2:
		m_pBuffer->eraseEntireScreen();
		break;
	case 3:
		break;
	}
}


void QTermDecode::getAttr()
{
	// get all attributes of character
	
	if ( !nParam && param[0] == 0 )
	{
		m_curAttr = m_defAttr ;
		m_pBuffer->setCurAttr(m_curAttr);
		return;
	}

	char cp = GETCOLOR( m_curAttr );
	char ea = GETATTR( m_curAttr );
	for ( int n = 0; n <= nParam; n++ )
	{
		if ( param[n]/10 == 4 )	// background color
		{
			cp = cp & ~BGMASK;
			cp += SETBG( param[n]%10 );
		}
		else if ( param[n]/10 == 3 )	// front color
		{
			cp = cp & ~FGMASK;
			cp += SETFG( param[n]%10 );
		}
		else
		{
			switch ( param[n] )
		 	{
			case 0:	// attr off
				cp = GETCOLOR( m_defAttr );//NO_COLOR;
				ea = GETATTR( m_defAttr );//NO_ATTR;
				break;
			case 1:	// bold
				ea = SETBOLD( ea );
				break;
			case 2:	// dim
				ea = SETDIM( ea );
				break;
			case 4:	// underline
				ea = SETUNDERLINE( ea );
				break;
			case 5:	// blink
				ea = SETBLINK( ea );
				break;
			case 7:	// reverse
				ea = SETREVERSE( ea );
				break;
			case 8:	// invisible
				ea = SETINVISIBLE( ea );
				break;
			default:
				break;
			}
		}
	}

	m_curAttr = SETCOLOR( cp ) | SETATTR( ea );

	m_pBuffer->setCurAttr( m_curAttr );
}

void QTermDecode::setMode()
{
	for( int i=0; i<=nParam; i++)
	{
		int n = param[i];
		switch(n)
		{
			case 4:
				m_pBuffer->setMode( INSERT_MODE );
				break;
			case 20:
				m_pBuffer->setMode( INSERT_MODE );
				break;
			case 1000:
			case 1001:
				emit mouseMode(true);
				bCurMode[MODE_MouseX11]=true;
				break;
			default:
				break;
		}
	}
}

void QTermDecode::resetMode()
{
	for( int i=0; i<=nParam; i++)
	{
		int n = param[i];
		switch(n)
		{
			case 4:
				m_pBuffer->resetMode( INSERT_MODE );
				break;
			case 20:
				m_pBuffer->resetMode( NEWLINE_MODE );
				break;
			case 1000:
			case 1001:
				bCurMode[MODE_MouseX11]=false;
				emit mouseMode(false);
				break;
			default:
			break;
		}
	}
}

void QTermDecode::saveMode()
{
	for( int i=0; i<=nParam; i++)
	{
		int n = param[i];
		switch(n)
		{
			case 1000:
			case 1001:
				bSaveMode[MODE_MouseX11]=bCurMode[MODE_MouseX11];
				break;
			default:
			break;
		}
	}
}

void QTermDecode::restoreMode()
{
	for( int i=0; i<=nParam; i++)
	{
		int n = param[i];
		switch(n)
		{
			case 1000:
			case 1001:
				bCurMode[MODE_MouseX11]=bSaveMode[MODE_MouseX11];
				emit mouseMode( bCurMode[MODE_MouseX11] );
				break;
			default:
			break;
		}
	}
}


void QTermDecode::test()
{
}
#ifdef HAVE_CONFIG_H
#include "qtermdecode.moc"
#endif
