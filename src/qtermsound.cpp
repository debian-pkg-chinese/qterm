#include "qtermsound.h"
#include <QtCore/QFile>
#include <QtCore/QProcess>
#include <QtGui/QMessageBox>

#ifdef PHONON_ENABLED
#ifdef OLD_PHONON
#include <phonon>
#else
#include <phonon/MediaObject>
#endif
#endif // PHONON_ENABLED

namespace QTerm
{

Sound::~Sound()
{
}

#ifdef PHONON_ENABLED
void
PhononSound::play()
{
    if (QFile::exists(_soundfile)) {
        Phonon::MediaObject * player = Phonon::createPlayer(Phonon::NotificationCategory, Phonon::MediaSource(_soundfile));
        player->play();
    }
}
#endif // PHONON_ENABLED

void
ExternalSound::setPlayer(const QString & playername)
{
    _player = playername;
}

void
ExternalSound::play()
{
    if (QFile::exists(_soundfile)) {
        QString command = _player + ' ' + _soundfile;
        QProcess::startDetached(command);
    }
}

} // namespace QTerm
