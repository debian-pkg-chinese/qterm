#ifndef QTERMSSHMD5_H
#define QTERMSSHMD5_H

#include "types.h"
#include <openssl/md5.h>
#include "qtermsshhash.h"

class QTermSSHMD5 : public QTermSSHHash
{
protected:
	MD5_CTX d_md5;
public:
	QTermSSHMD5()
		:QTermSSHHash()
	{
		d_digestLength = 16;
		MD5_Init(&d_md5);
	}
	void update(u_char * data, int len);
	void final(u_char * digest);
};

#endif		//QTERMSSHMD5_H
