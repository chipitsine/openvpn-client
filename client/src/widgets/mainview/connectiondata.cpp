#include "connectiondata.h"

#include <qdatetime.h>
#include <service/srvcli.h>
#include <dialogs/FrmGetUserData.h>
#include <database/crypt.h>
#include <database/database.h>
#include <message.h>
#include <frmmain.h>
#include <widgets/settings/client/settings.h>
#include <utils.h>

ConnectionData::ConnectionData(QObject *parent)
    : QObject(parent)
{
    ip = "xxx.xxx.xxx.xxx";
    state = ConnectionState::Disconnected;
    lastUsed = 0;
    lastConnected = 0;
    user = true;
}

ConnectionData::~ConnectionData()
{

}

void ConnectionData::SetName(const QString &name)
{
    this->name = name;
}

const QString ConnectionData::GetName()
{
    return this->name;
}

const QString ConnectionData::GetIP()
{
    return this->ip;
}

void ConnectionData::SetIP(const QString &ip)
{
    QString timeOut (this->getScript("TO"));
    int scriptDelay (0);
    // Ist ein Timeout da?
    if (timeOut.isEmpty()) {
        // Nein, Standard sind 2 Sekunden
        timeOut = QLatin1String ("2000");
    }
    //
    scriptDelay = timeOut.trimmed().toInt();

    QTimer::singleShot(scriptDelay, this, SLOT(startAfterConnectDelayed()));

    this->ip = ip;
}

void ConnectionData::startAfterConnectDelayed()
{
    // Diese Funktion wird von einem
    // Singleshot timer aufgerufen, um
    // sicherstellen, dass die IP-Adreese
    // zugewiesen ist.
    this->runScript("AC");
}


quint32 ConnectionData::GetId()
{
    return id;
}

void ConnectionData::SetId(quint32 id)
{
    this->id = id;
}

void ConnectionData::SetError(QString errorMsg)
{
    this->errorMsg = errorMsg;

    QString sql;
    sql = QString("UPDATE vpn SET %1 = '%2' WHERE \"vpn-id\" = %3")
        .arg("\"vpn-last-error\"")
        .arg(errorMsg)
        .arg(this->GetId());

    Database::instance()->execute(sql);
}

QString ConnectionData::GetError()
{
    return this->errorMsg;
}

quint32 ConnectionData::GetLastConnected()
{
    return this->lastConnected;
} 

void ConnectionData::runScript(const QString &type)
{
    if (!this->getScript(type).isEmpty()) {

        auto procScripts = new QProcess(this);
        QObject::connect(procScripts, SIGNAL(error(QProcess::ProcessError)), this, SLOT(showProcessScriptError(QProcess::ProcessError)));
        QObject::connect(procScripts, SIGNAL(finished(int,QProcess::ExitStatus)), procScripts, SLOT(deleteLater()));

        procScripts->start(this->getScript(type));
        qApp->processEvents() ;
    }
}

QString ConnectionData::getScript(const QString &type)
{
        QFile scrtiptFile (this->GetDir() + QLatin1String("/scripts.conf"));

        if (scrtiptFile.exists()) {
            // �ffnen und auslesen
            if (!scrtiptFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                Message::error(QObject::tr("Can't read scriptconfig file!"), QObject::tr("An error occured"), FrmMain::instance());
                return QLatin1String ("");
            }

            // Datei offen
            QTextStream sin (&scrtiptFile);
            while (!sin.atEnd()){
                QString line (sin.readLine());
                if (line.trimmed().left(3).toUpper() == type.toUpper() + QLatin1String(":")) {
                    scrtiptFile.close();
                    return line.right(line.size() - 3);
                }
            }
            scrtiptFile.close();
        }

        return QLatin1String("");
    }


quint32 ConnectionData::GetLastUsed()
{
    return this->lastUsed;
}

void ConnectionData::SetLastUsed(quint32 lastUsed, bool bDb)
{
    if(bDb)
    {
        QString sql;
        sql = QString("UPDATE vpn SET %1 = '%2' WHERE \"vpn-id\" = %3")
            .arg("\"vpn-last-used\"")
            .arg((int)lastUsed)
            .arg(this->GetId());

        Database::instance()->execute(sql);
    }
    this->lastUsed = lastUsed;
}

void ConnectionData::SetLastConnected(quint32 lastConnected, bool bDb)
{
    if(bDb)
    {
        QString sql;
        sql = QString("UPDATE vpn SET %1 = '%2' WHERE \"vpn-id\" = %3")
            .arg("\"vpn-last-connected\"")
            .arg((int)lastConnected)
            .arg(this->GetId());

        Database::instance()->execute(sql);
    }
    this->lastConnected = lastConnected;
}

ConnectionState ConnectionData::GetState()
{
    return state;
}

void ConnectionData::SetState(ConnectionState state, bool bDb)
{
    if(bDb)
    {
        QString sql;
        sql = QString("UPDATE vpn SET %1 = '%2' WHERE \"vpn-id\" = %3")
            .arg("\"vpn-state\"")
            .arg((int)state)
            .arg(this->GetId());

        Database::instance()->execute(sql);
    }
    this->state = state;

    if(state == ConnectionState::Disconnected)
    {
        runScript("AD");
    } 
    else if(state == ConnectionState::Error)
    {
        runScript("EC");
    }

    if(FrmMain::instanceCheck() && FrmMain::instance())
        FrmMain::instance()->setIcon();
}

void ConnectionData::SetAutoStart(bool bAutoStart)
{
    {
        QString sql;
        sql = QString("UPDATE vpn SET %1 = '%2' WHERE \"vpn-id\" = %3")
            .arg("\"vpn-autostart\"")
            .arg(bAutoStart ? "1" : "0")
            .arg(this->GetId());

        Database::instance()->execute(sql);
    }
    this->autostart = bAutoStart;
}

bool ConnectionData::IsAutoStart()
{
    return this->autostart;
}

void ConnectionData::SetUserConfig(bool user)
{
    this->user = user;
}

bool ConnectionData::IsUserConfig()
{
    return this->user;
}

void ConnectionData::SetConfigPath(const QString& path)
{
    this->configPath = path;
}

const QString& ConnectionData::GetConfigPath()
{
    return this->configPath;
}


const QString ConnectionData::GetDir()
{
    return this->configPath.left(this->configPath.lastIndexOf("/"));
}

bool ConnectionData::Connect()
{
    this->SetState(ConnectionState::Connecting);
    // Verbinden senden

    this->runScript("BC");

    auto makeProxyString = []() -> QStringList
    {
        QStringList retList;
        QFile pINI (Utils::dataDirectory() + QLatin1String("/proxy.ini"));
        if (pINI.exists()) {
            QSettings proxIni (Utils::dataDirectory() + QLatin1String("/proxy.ini"), QSettings::IniFormat);
            if (proxIni.value(QLatin1String("proxy-use"), QLatin1String("")).toString() == QLatin1String("CONFIG")) {
                // Nothing
            } else if (proxIni.value(QLatin1String("proxy-use"), QLatin1String("")).toString() == QLatin1String("IE")) {
                // IE
#ifdef Q_OS_WIN32
                QSettings regIE (QLatin1String("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"), QSettings::NativeFormat);
                QString regVal = regIE.value(QLatin1String("ProxyServer"), QLatin1String("")).toString();
                QStringList regData = regVal.split(":");
                if (regData.size() == 2) {
                    retList << QLatin1String ("--http-proxy")
                        << regData[0]
                    << regData[1];
                }
#endif
            } else {
                // MANUAL
                if (proxIni.value(QLatin1String("proxy-port"), QLatin1String("")).toString() != "" && proxIni.value(QLatin1String("proxy-ip"), QLatin1String("")).toString() != "") {
                    if (proxIni.value(QLatin1String("proxy-type"), QLatin1String("")).toString() == QLatin1String("HTTP")) {
                        retList << QLatin1String ("--http-proxy")
                            << proxIni.value(QLatin1String("proxy-ip"), QLatin1String("")).toString()
                            << proxIni.value(QLatin1String("proxy-port"), QLatin1String("")).toString();
                    } else {
                        retList << QLatin1String ("--socks-proxy")
                            << proxIni.value(QLatin1String("proxy-ip"), QLatin1String("")).toString()
                            << proxIni.value(QLatin1String("proxy-port"), QLatin1String("")).toString();
                    }
                }
            }
        }

        return retList;
    };

    return SrvCLI::instance()->send(QLatin1String("Open"), QString::number(this->GetId())
                                                    + QLatin1String(";")
                                                    + this->GetConfigPath()
                                                    + QLatin1String (";")
                                                    + (Settings::instance()->useInteract() ? QLatin1String("0") : QLatin1String("1"))
                                                    + QLatin1String (";")
                                                    + makeProxyString().join(" "));
    return true;
}

void ConnectionData::saveUserData(int id, int type, QString value, bool save)
{
    Q_UNUSED(id)

    // Wenn save false ist den Wert �berschreiben
    if (!save) {
        return;
    }
    // Welcher type wurde �bergeben
    // 0 - Username
    // 1 - Pwd
    // 2 - OTP
    // 3 - PKCS12
    // 4 - Private Key f�r Crypted User Data
    QString field;
    if (type == 0) {
        field = QLatin1String("\"vpn-user\"");
    } else if (type == 1) {
        field = QLatin1String("\"vpn-password\"");
    } else if (type == 3) {
        field = QLatin1String("\"vpn-pkcs12\"");
    } else if (type == 5) {
        field = QLatin1String("\"vpn-http-user\"");
    } else if (type == 6) {
        field = QLatin1String("\"vpn-http-password\"");
    }


    if (!value.isEmpty()) {
        value = Crypt::encodePlaintext(value.toLatin1());
    }

    QString sql;
    sql = QString("UPDATE vpn SET %1 = '%2' WHERE \"vpn-id\" = %3")
            .arg(field)
            .arg(value)
            .arg(this->GetId());

    Database::instance()->execute(sql);

    if (!value.isEmpty() && save) {
    }
}

bool ConnectionData::HasCrediantials(int type)
{
    // Allgemeine Frage oder nach Typ
    if (type == -1) {
        // Allgemein
        if (this->HasCrediantials(0)) {
            return true;
        } else if (this->HasCrediantials(1)) {
            return true;
        } else if (this->HasCrediantials(3)) {
            return true;
        } else if (this->HasCrediantials(5)) {
            return true;
        } else if (this->HasCrediantials(6)) {
            return true;
        }
    }

    // Es wurde ein Typ �bergeben
    // 0 - Username
    // 1 - Pwd
    // 2 - OTP
    // 3 - PKCS12
    // 4 - Private Key f�r Crypted User Data
    QString field;
    if (type == 0) {
        field = QLatin1String("\"vpn-user\"");
    } else if (type == 1) {
        field = QLatin1String("\"vpn-password\"");
    } else if (type == 3) {
        field = QLatin1String("\"vpn-pkcs12\"");
    } else if (type == 5) {
        field = QLatin1String("\"vpn-http-user\"");
    } else if (type == 6) {
        field = QLatin1String("\"vpn-http-password\"");
    }

    if (field.isEmpty()) {
        // F�r den typ kann man keine Daten speichern
        return false;
    }

    QString sql;
    sql = QLatin1String("SELECT ") + field + QLatin1String(" FROM vpn WHERE \"vpn-id\" = ") + QString::number(this->GetId());

    

    // Sind daten f�r die Verbindung hinterlegt
    QScopedPointer<QSqlQuery> query (Database::instance()->openQuery(sql));
    if (query->first() && !query->value(0).toString().isEmpty()) {
        return true;
    }

    // Kein Daten da
    return false;
}


void ConnectionData::writeUserData(QString data)
{
    /*if (this->proc.isWritable()) {
        QByteArray ba;
        ba.append(data + "\n");
        this->proc.write(ba);
    }*/
}

QString ConnectionData::getSavedUserData(int type)
{
    // 0 - Username
    // 1 - Pwd
    // 2 - OTP
    // 3 - PKCS12
    // 4 - Private Key f�r Crypted User Data
    QString field;
    if (type == 0) {
        field = QLatin1String("\"vpn-user\"");
    } else if (type == 1) {
        field = QLatin1String("\"vpn-password\"");
    } else if (type == 3) {
        field = QLatin1String("\"vpn-pkcs12\"");
    } else if (type == 5) {
        field = QLatin1String("\"vpn-http-user\"");
    } else if (type == 6) {
        field = QLatin1String("\"vpn-http-password\"");
    }

    if (field.isEmpty()) {
        return QString ();
    }

    QString sql;
    sql = QString ("SELECT %1 FROM vpn WHERE \"vpn-id\" = %2")
            .arg(field)
            .arg(this->GetId());

    QScopedPointer<QSqlQuery> query (Database::instance()->openQuery(sql));
    
    QString value;
    if (query->first()) {
        value = query->value(0).toString();
        value = value.trimmed();
    }

    if (!Crypt::secretKey.isEmpty()) {
        value = Crypt::decodeToPlaintext(value.toLatin1());
    }

    return value;
}


bool ConnectionData::Disconnect()
{
    runScript("BD");
    return SrvCLI::instance()->send(QLatin1String("Close"), QString::number(GetId()));
}

void ConnectionData::Delete()
{
    QDir dir(this->configPath.left(this->configPath.lastIndexOf("/")));
    dir.removeRecursively();
}