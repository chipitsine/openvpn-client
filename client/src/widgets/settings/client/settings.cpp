#include "settings.h"
#include <database/database.h>
#include <database/crypt.h>

Settings *Settings::mInst = 0;

Settings *Settings::instance()
{
    if (!mInst) {
        mInst =  new Settings;
    }

    return mInst;
}

Settings::Settings()
    : isOnRefresh (false),
      isUseInteract (true),
      isStartOnWindows (false),
      isBlockShutdown (true),
      isShowBallon (true),
      isAutoRestart(false),
      isAlwaysPopup (false),
      isShowSplash (true),
      isEnableUpdate(true),
      isUseSecurepoint(true),
      isManaged(false),
      _bShowLog(false),
      _startConfigPath(""),
      _startUser(""),
      _startPassword("")
{
}

void Settings::loadOrRefresh()
{
    //
    // Loading the data from the data provider
    //

    // Lock save
    this->isOnRefresh = true;

    QScopedPointer<QSqlQuery> query (Database::instance()->openQuery("SELECT [settings-name], [settings-value] FROM settings;"));
    //
    while (query->next()) {
        // Set value
        QString name (Crypt::decodeToPlaintext(query->value(0).toString()));
        QString valueRaw (Crypt::decodeToPlaintext(query->value(1).toString()));
        // Read meta object data
        const QMetaObject *object = this->metaObject();
        int index (object->indexOfProperty(name.toLatin1().constData()));
        //
        // Found a valid index?
        if (index > -1) {
            QMetaProperty propertyData = object->property(index);
            // If the property is a bool, it is necessary to convert it by hand
            QVariant value (valueRaw);
            if (propertyData.type() == QVariant::Bool) {
                // Convert QString to Bool - only 1 and true -> true
                value = QVariant(((valueRaw == QLatin1String("1") || valueRaw.toLower() == QLatin1String("true")) ? true : false));
            }
            //
            // Set the property
            if(!this->setProperty(name.toLatin1().constData(), value)) {
                // Error, no poperty available
                qDebug() << QLatin1String("Can't set property: ") << name << value;
            }
        }
    }

    // Unlock save
    this->isOnRefresh = false;
}

void Settings::setUseInteract(bool flag)
{
    // Change
    if (flag == this->isUseInteract) {
        return;
    }
    // Value have been changed
    this->isUseInteract = flag;
    //
    this->save(QLatin1String("useInteract"), (this->isUseInteract ? QLatin1String("1") : QLatin1String("0")));
}

bool Settings::useInteract() const
{
    return this->isUseInteract;
}

extern bool bGerman;

void Settings::setStartOnWindows(bool flag)
{
    // Change
    if (flag == this->isStartOnWindows) {
        return;
    }
    // Value have been changed
    this->isStartOnWindows = flag;

     QSettings regRun (QLatin1String("HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"), QSettings::NativeFormat);
    if (flag) {
        QString path;
        path = QCoreApplication::applicationDirPath() + QLatin1String("/SSLVpnClient.exe");

        if (!bGerman) {

            path += QLatin1String(" -german");
        }

        if (this->isManaged) {
            // Add manage param
            path += QLatin1String(" -manage");
        }

        path = path.replace("/", "\\");
        regRun.setValue(QLatin1String("SpSSLVPN"), path);
    } else {
        if (this->startOnWindows()) {
            regRun.remove(QLatin1String("SpSSLVPN"));
        }
    }

    //
    this->save(QLatin1String("startOnWindows"), (this->isStartOnWindows ? QLatin1String("1") : QLatin1String("0")));
}

bool Settings::startOnWindows() const
{
    QSettings regRun (QLatin1String("HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"), QSettings::NativeFormat);
    QString regVal = regRun.value(QLatin1String("SpSSLVPN"), QLatin1String("0")).toString();
    // Ist der Eintrag da und gef�llt
    if (regVal != QLatin1String("0")) {
        if(!isStartOnWindows)
        {
            QSettings regRun (QLatin1String("HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"), QSettings::NativeFormat);
            regRun.remove(QLatin1String("SpSSLVPN"));
        }
        else
        {
            QSettings regRun (QLatin1String("HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"), QSettings::NativeFormat);
            QString path;
            path = QCoreApplication::applicationDirPath() + QLatin1String("/SSLVpnClient.exe");

            /*if (!this->isLangGerman) {

            path += QLatin1String(" -useEnglish");
            }*/

            if (this->isManaged) {
                // Add manage param
                path += QLatin1String(" -manage");
            }

            path = path.replace("/", "\\");
            regRun.setValue(QLatin1String("SpSSLVPN"), path);
        } 
    }

    return this->isStartOnWindows;
}

bool Settings::blockShutdown() const
{
    return this->isBlockShutdown;
}

void Settings::setBlockShutdown(bool flag)
{
    // Change
    if (flag == this->isBlockShutdown) {
        return;
    }
    // Value have been changed
    this->isBlockShutdown = flag;
    //
    this->save(QLatin1String("blockShutdown"), (this->isBlockShutdown ? QLatin1String("1") : QLatin1String("0")));
}

bool Settings::showBallon() const
{
    return this->isShowBallon;
}

void Settings::setShowBallon(bool flag)
{
    // Change
    if (flag == this->isShowBallon) {
        return;
    }
    // Value have been changed
    this->isShowBallon = flag;
    //
    this->save(QLatin1String("showBallon"), (this->isShowBallon ? QLatin1String("1") : QLatin1String("0")));
}

void Settings::setAutoRestart(bool flag)
{
    // Change
    if (flag == this->isAutoRestart) {
        return;
    }
    // Value have been changed
    this->isAutoRestart = flag;
    //
    this->save(QLatin1String("autoRestart"), (this->isAutoRestart ? QLatin1String("1") : QLatin1String("0")));
}

bool Settings::autoRestart() const
{
    return this->isAutoRestart;
}

void Settings::setAlwaysPopup(bool flag)
{
    // Change
    if (flag == this->isAlwaysPopup) {
        return;
    }
    // Value have been changed
    this->isAlwaysPopup = flag;
    //
    this->save(QLatin1String("alwaysPopup"), (this->isAlwaysPopup ? QLatin1String("1") : QLatin1String("0")));
}

bool Settings::alwaysPopup() const
{
    return this->isAlwaysPopup;
}

void Settings::setShowSplash(bool flag)
{
    // Change
    if (flag == this->isShowSplash) {
        return;
    }
    // Value have been changed
    this->isShowSplash = flag;
    //
    this->save(QLatin1String("showSplash"), (this->isShowSplash ? QLatin1String("1") : QLatin1String("0")));
}

bool Settings::showSplash() const
{
    return this->isShowSplash;
}

void Settings::setEnableUpdate(bool flag)
{
    // Change
    if (flag == this->isEnableUpdate) {
        return;
    }

    // Value have been changed
    this->isEnableUpdate = flag;
    //
    this->save(QLatin1String("enableUpdate"), (this->isEnableUpdate ? QLatin1String("1") : QLatin1String("0")));
}

bool Settings::enableUpdate() const
{
    return this->isEnableUpdate;
}

void Settings::setUseSecurepoint(bool flag)
{
    // Change
    if (flag == this->isUseSecurepoint) {
        return;
    }
    // Value have been changed
    this->isUseSecurepoint = flag;
    //
    this->save(QLatin1String("useSecurepoint"), (this->isUseSecurepoint ? QLatin1String("1") : QLatin1String("0")));
}

void Settings::setManaged(bool flag)
{
    this->isManaged = flag;
}


bool Settings::managed() const
{
    return this->isManaged;
}

bool Settings::useSecurepoint() const
{
    return this->isUseSecurepoint;
}

void Settings::setUpdateUrl(const QString &value)
{
    // Change
    if (value == this->updateUrlValue) {
        return;
    }
    // Value have been changed
    this->updateUrlValue = value;
    //
    this->save(QLatin1String("updateUrl"), this->updateUrlValue);
}

QString Settings::updateUrl() const
{
    return this->updateUrlValue;
}

void Settings::setMaschineConfigDirectory(const QString &value)
{
    // Change
    if (value == this->maschineDirectoryValue) {
        return;
    }
    // Value have been changed
    this->maschineDirectoryValue = value;
    //
    this->save(QLatin1String("maschineConfigDirectory"), this->maschineDirectoryValue);
}

QString Settings::maschineConfigDirectory() const
{
    return this->maschineDirectoryValue;
}

void Settings::save(const QString &name, const QString &value)
{
    //
    // Saved the given value
    //
    if (!this->isOnRefresh){
        // Write to disk
        QString settingsName (Crypt::encodePlaintext(name));
        QString settingsValue (Crypt::encodePlaintext(value));
        //
        QString sql (QString("UPDATE settings SET [settings-value] = '%1' WHERE [settings-name] = '%2';")
                     .arg(settingsValue)
                     .arg(settingsName));
        // Save
        Database::instance()->execute(sql);
    }
}


    QString Settings::startConfig() const
    {
        return this->_startConfigPath;
    }
    
    void Settings::setStartConfig(const QString &value)
    {
        this->_startConfigPath = value;
    }

    QString Settings::startUser() const
    {
        return this->_startUser;
    }

    void Settings::setStartUser(const QString &value)
    {
        this->_startUser = value;
    }

    QString Settings::startPassword() const 
    {
        return this->_startPassword;
    }

    void Settings::setStartPassword(const QString &value)
    {
        this->_startPassword = value;
    }


    void Settings::setVpnLog(bool bShow)
    {
        this->_bShowLog = bShow;
    }

    bool Settings::VpnLog()
    {
        return this->_bShowLog;
    }