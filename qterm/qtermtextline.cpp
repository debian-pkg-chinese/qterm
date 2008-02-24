/*******************************************************************************
FILENAME:      qtermtextline.cpp
REVISION:      2001.8.12 first created.
         
AUTHOR:        kingson fiasco
*******************************************************************************/
/*******************************************************************************
                                    NOTE
 This file may be used, distributed and modified without limitation.
 *******************************************************************************/
#include "qterm.h"
#include "qtermtextline.h"

#include <qregexp.h>

#include <stdio.h>

QTermTextLine::QTermTextLine()
{
	m_bChanged=true;
	m_start = -1;
	m_end = -1;
	reset();
}

QTermTextLine::~QTermTextLine()
{

}


// insert cstr at position index of line, 
// if attrib == -1, use current attr,
// if index == -1, append line
void QTermTextLine::insertText( const QCString& str, short attribute, int index )
{
	// set attribute
	if ( attribute != -1 )	
	{
		m_curColor = GETCOLOR(attribute);
		if( m_curColor == '\0' )
			m_curColor = NO_COLOR;
		m_curAttr  = GETATTR (attribute);
		if( m_curAttr == '\0' )
			m_curAttr = NO_ATTR;
	}
	

	int len = str.length();

	QCString tmp;

	int start; 

	if ( index == -1 )	// append
	{
		m_text += str;
		
		tmp.fill( m_curColor, len );
		m_color += tmp;
		tmp.fill( m_curAttr, len );	
		m_attr  += tmp;

		start = m_length;
		m_length += len;
	}
	else			// insert
	{

		if(index>=m_length)
		{
			m_text.insert( index, str );
			m_length = m_text.length();		

			tmp.fill( NO_COLOR, index-m_length );
			m_color.append(tmp);
			tmp.fill( m_curColor, len );
			m_color.insert( index, tmp );

			tmp.fill( NO_ATTR, index-m_length );
			m_attr.append(tmp);
			tmp.fill( m_curAttr, len );	
			m_attr.insert( index, tmp );
		}
		else
		{
			m_text.insert( index, str );
			m_length = m_text.length();		
			tmp.fill( m_curColor, len );
			m_color.insert( index, tmp );
			tmp.fill( m_curAttr, len );	
			m_attr.insert( index, tmp );

		}
		start = index;
	}
	
	setChanged( start, m_length );
	
}

// replace the cstring with cstr, which start at index and have len chars,
// if attr == -1, use the current attr,
// if index == -1, reset line and insert str.
// if len == -1, replace str's length chars.
void QTermTextLine::replaceText( const QCString& str, short attribute, int index, int len )
{
	// set attribute
	if ( attribute != -1 )
	{
		m_curColor = GETCOLOR(attribute);
		if( m_curColor == '\0' )
			m_curColor = NO_COLOR;
		m_curAttr  = GETATTR (attribute);
		if( m_curAttr == '\0' )
			m_curAttr = NO_ATTR;
	}

	int newlen = str.length();

	QCString tmp;
	
	if ( index == -1 )	// replace whole line
	{
		m_text = str;

		m_color.fill( m_curColor, newlen );
		m_attr.fill( m_curAttr, newlen );
	
		setChanged( 0,QMAX(newlen, m_length) );

		m_length = newlen;
		
		return;
	}

	if ( len == -1 )	// replace with  str
		len = newlen;

	if( index>=m_length )
	{
			tmp.fill( ' ', index-m_length );
			m_text.replace( index, len, str );
		
			setChanged( index, QMAX(m_length, m_text.length()));

			m_length = m_text.length();
	
			
			tmp.fill( NO_COLOR, index-m_length );
			m_color.append(tmp);
			tmp.fill( m_curColor, newlen );
			m_color.replace( index, len, tmp );

			tmp.fill( NO_ATTR, index-m_length );
			m_attr.append(tmp);
			tmp.fill( m_curAttr, newlen );
			m_attr.replace( index, len, tmp );

	}
	else
	{
		m_text.replace( index, len, str );

		setChanged( index, QMAX(m_length, m_text.length()));

		m_length = m_text.length();

		tmp.fill( m_curColor, newlen );
		m_color.replace( index, len, tmp );
		tmp.fill( m_curAttr, newlen );
		m_attr.replace( index, len, tmp );
	}

}

// delete cstr from position index of line, 
// it will delete len chars,  
// if index == -1, delete the whole line
// if len ==-1, delete the rest from index
void QTermTextLine::deleteText( int index, int len )
{
	if ( index == -1 )	// delete the line
	{	
		setChanged( 0, m_length );
		reset();
		return;
	}

	if ( len == -1 )	// only make len larger so it will delete
		len = m_length;	// the right from index
	
	m_text.remove( index, len );
	m_color.remove( index, len );
	m_attr.remove( index, len );

	setChanged( index, m_length );

	m_length = m_text.length();
	

}
	
// return str in text for show
// if index == -1, get the whole line
// if len == -1, get the rest from index
QCString QTermTextLine::getText( int index, int len )
{
	QCString str;

	if ( index == -1 )
		str = m_text;
	else if ( len == -1 )	
		str = m_text.mid( index, len );
	else
		str = m_text.mid( index, len );


	return str;
}

QCString QTermTextLine::getAttrText( int index, int len, const QCString& escape )
{
	QCString str;
	int startx;
	char tempcp, tempea;

	if ( index == -1 )
	{
		index =0;
		len = m_length;
	}
	else if ( len == -1 )
		len = m_length-index;


	if(index>=m_length)
		return (char *)NULL;


	for( int i=index; i<index+len && i<m_length; i++)
	{
		startx = i;
		tempcp = m_color.at(i);
		tempea = m_attr.at(i);
		// get str of the same attribute
		while ( tempcp == m_color.at(i) && 
			tempea == m_attr.at(i)  &&
			i < m_length )
			i++;
		
		int fg = GETFG(tempcp)+30;
		int bg = GETBG(tempcp)+40;
		QCString cstrAttr;
		cstrAttr.sprintf("%d;%d;", fg, bg );
		cstrAttr = escape + cstrAttr;

		if (GETBOLD(tempea))
			cstrAttr += "1;";
		if ( GETDIM(tempea ) )
			cstrAttr += "2;";	
		if ( GETUNDERLINE( tempea ) )
			cstrAttr += "4;";		
		if ( GETBLINK( tempea ) )
			cstrAttr += "5;";
		if ( GETRAPIDBLINK( tempea ) )
			cstrAttr += "6;";	
		if ( GETREVERSE( tempea ) )
			cstrAttr += "7;";	
		if ( GETINVISIBLE( tempea ) )
			cstrAttr += "8;";
		cstrAttr.remove(cstrAttr.length()-1,1);
		cstrAttr += "m";
		str += cstrAttr; // set attr
		// the text
		str += getText(startx, i-startx);	
		// reset attr
		cstrAttr = escape+"0m";
		str	+= cstrAttr;
		i--;
	}
	
	return str;
}

// reset line
inline void QTermTextLine::reset()
{
	m_length = 0;
	
	m_text = "";
	m_color = "";
	m_attr ="";

	m_curColor = NO_COLOR;
	m_curAttr = NO_ATTR;


}
bool QTermTextLine::hasBlink()
{
	bool blink = false;
	
	char tempea;
	for( int i=0; i<m_length; i++)
	{
		tempea = m_attr.at(i);
		if(GETBLINK(tempea))
		{
			blink = true;
			break;
		}
	}

	return blink;
}


bool QTermTextLine::isChanged(int &start, int &end)
{
	start=m_start;
	end=m_end;
	
	return m_bChanged;
}

void QTermTextLine::setChanged( int start, int end )
{
	if(start==-1 && end==-1) 
	{
		m_bChanged = true;
		m_start=start;
		m_end=end;
		return;
	}
	
	if(m_bChanged)
	{
		if(!(m_start==-1 && m_end==-1))
		{
			m_start=QMIN(start, m_start);
			m_end = QMAX(end, m_end);
		}
	}
	else
	{
		m_start = start;
		m_end = end;
	}

	m_bChanged = true;
}

