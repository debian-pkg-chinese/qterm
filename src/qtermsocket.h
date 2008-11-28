#ifndef QTERMSOCKET_H
#define QTERMSOCKET_H

// _OS_X_ not defined if i dont include it
#include <QtGlobal>
#include <QtCore/QObject>
#include <QtNetwork/QTcpSocket>
// different 
#if defined(Q_OS_WIN32) || defined(_OS_WIN32_)
	#include <winsock2.h>
#elif defined(Q_OS_BSD4) || defined(_OS_FREEBSD_) \
	|| defined(Q_OS_MACX) || defined(Q_OS_DARWIN)
	#include <netdb.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
#else
	#include <netdb.h>
	#include <sys/socket.h>
	#include <sys/types.h>
	#include <arpa/inet.h>
#endif

namespace QTerm
{
class HostInfo;
class SocketPrivate : public QObject
{
	Q_OBJECT
public:
	SocketPrivate(QObject * parent = 0);
	~SocketPrivate();

	void flush();
	void setProxy(int nProxyType, bool bAuth,
			const QString& strProxyHost, quint16 uProxyPort,
			const QString& strProxyUsr, const QString& strProxyPwd);
	void connectToHost(HostInfo * hostInfo);
	void close();
	QByteArray readBlock(unsigned long maxlen);
	long writeBlock(const QByteArray & data);
	unsigned long bytesAvailable();
	QAbstractSocket::SocketState state();
	HostInfo * hostInfo();

signals:
	void connected();
	void hostFound();
	void connectionClosed();
	void delayedCloseFinished();
	void readyRead();
	void error(QAbstractSocket::SocketError);
	void SocketState(int);

protected slots:
	void socketConnected();
	void socketReadyRead();	
	
protected:
	// socks5 function
	void socks5_connect();
	void socks5_auth();
	void socks5_reply( const QByteArray&, int );

private:
	// proxy 
	int 		proxy_type;
	QString 	proxy_host;
	QString 	proxy_usr;
	quint16  	proxy_port;
	QString 	proxy_pwd;
	QString 	host;
	quint16 	port;
	int			proxy_state;
	bool		bauth;

	struct sockaddr_in  addr_host;

	HostInfo * m_hostInfo;
	QTcpSocket *m_socket;
};

// virtual base class for TelnetSocket and SSHSocket
class Socket : public QObject
{
	Q_OBJECT
public:
	Socket(QObject * parent = 0)
		:QObject(parent)
	{
	}
	virtual ~Socket()
	{
	}
	virtual void flush() = 0;
	virtual void setProxy(int nProxyType, bool bAuth,
			const QString& strProxyHost, quint16 uProxyPort,
			const QString& strProxyUsr, const QString& strProxyPwd) = 0;
	virtual void connectToHost(HostInfo * hostInfo) = 0;
	virtual void close() = 0;
	virtual QByteArray readBlock(unsigned long maxlen) = 0;
	virtual long writeBlock(const QByteArray & data) = 0;
	virtual unsigned long bytesAvailable() = 0;
signals:
	void connected();
	void hostFound();
	void connectionClosed();
	void delayedCloseFinished();
	void readyRead();
	void error(QAbstractSocket::SocketError);
	void SocketState(int);
};

class TelnetSocket : public Socket
{
private:
	SocketPrivate * d_socket;
public:
	TelnetSocket();
	~TelnetSocket();
	void flush();
	void setProxy(int nProxyType, bool bAuth,
			const QString& strProxyHost, quint16 uProxyPort,
			const QString& strProxyUsr, const QString& strProxyPwd);
	void connectToHost(HostInfo * hostInfo);
	void close();
	QByteArray readBlock(unsigned long maxlen);
	long writeBlock(const QByteArray & data);
	unsigned long bytesAvailable();
};

} // namespace QTerm

#endif		//QTERMSOCKET_H
