#ifndef QTERMSSHDES_H
#define QTERMSSHDES_H

#include "qtermsshcipher.h"
#include <openssl/des.h>
#include "types.h"

namespace QTerm
{
/*
class QTermSSHDES : public SSHCipher
{
private:
	u_char d_IV[8];
	DES_key_schedule * d_key;
public:
	QTermSSHDES();
	void setIV(const u_char * data);
	void setKey(const u_char * data);
	void encrypt(const u_char * source, u_char * dest, int len);
	void decrypt(const u_char * source, u_char * dest, int len);
};
*/
class SSH1DES3 : public SSHCipher
{
private:
	DES_cblock d_IV1;
	DES_cblock d_IV2;
	DES_cblock d_IV3;

	DES_key_schedule d_key1;
	DES_key_schedule d_key2;
	DES_key_schedule d_key3;
public:
	SSH1DES3();
	void setIV(const u_char * data);
	void setKey(const u_char * data);
	void encrypt(const u_char * source, u_char * dest, int len);
	void decrypt(const u_char * source, u_char * dest, int len);
};

} // namespace QTerm

#endif		//QTERMSSHDES_H
