#include "qtermsshauth.h"
#include "qtermsshpacket.h"
#include "qtermsshconst.h"
#include "qtermsshlogin.h"
#include <qstring.h>
namespace QTerm
{
//==============================================================================
//SSH1PasswdAuth
//==============================================================================

SSH1PasswdAuth::SSH1PasswdAuth(const char * sshuser, const char * sshpasswd)
	: SSHPasswdAuth(sshuser, sshpasswd)
{
	d_isTried = false;
	d_state = SSH1PasswdAuth::BEFORE_AUTH;
}

void SSH1PasswdAuth::initAuth(SSHPacketReceiver * packet, SSHPacketSender * output)
{
	d_incomingPacket = packet;
	d_outcomingPacket = output;
	d_incomingPacket->disconnect(this);
	connect(d_incomingPacket, SIGNAL(packetAvaliable(int)), this, SLOT(handlePacket(int)));
	d_outcomingPacket->startPacket(SSH1_CMSG_USER);
	while (d_user.isEmpty() || d_passwd.isEmpty()) {
		SSHLogin login(&d_user, &d_passwd);
		if (login.exec() == QDialog::Rejected) {
			emit authError("UserCancel");
			return;
		}
	}
		
	d_outcomingPacket->putString(d_user.toLatin1());
	d_outcomingPacket->write();
	d_state = USER_SENT;
	d_isTried = false;
}

void SSH1PasswdAuth::handlePacket(int type)
{
	switch (d_state) {
	case BEFORE_AUTH:
		qDebug("Auth: We should not be here");
		break;
	case USER_SENT:
		if (type == SSH1_SMSG_SUCCESS) {
			d_state = AUTH_OK;
			emit authOK();
			break;
		}
		if (type != SSH1_SMSG_FAILURE) {
			emit authError("Strange response from server");
			break;
		}
		if (d_isTried) {
			SSHLogin login(&d_user, &d_passwd);
			if (login.exec() == QDialog::Rejected) {
				emit authError("User canceled");
				break;
			}
			d_isTried = false;
		}
		d_outcomingPacket->startPacket(SSH1_CMSG_AUTH_PASSWORD);
		d_outcomingPacket->putString(d_passwd.toLatin1());
		d_outcomingPacket->write();
		d_isTried = true;
		break;
	case AUTH_OK:
		break;
	default:
		return;
	}
}

} // namespace QTerm

#include <qtermsshauth.moc>
