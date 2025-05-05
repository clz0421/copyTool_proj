#ifndef DISKMANAGER_H
#define DISKMANAGER_H

#include <QObject>
#include <QString>
#include <QStorageInfo>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QCryptographicHash>
#include <QUrl>
#include <QCoreApplication>
#ifdef Q_OS_WIN
  #include <windows.h>
  #include <cfgmgr32.h>
#endif


class DiskManager : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString message READ message WRITE setMessage NOTIFY messageChanged)
    Q_PROPERTY(bool isCopySuccessed READ isCopySuccessed WRITE setIsCopySuccessed NOTIFY isCopySuccessedChanged)

    Q_PROPERTY(QStringList drives READ drives WRITE setDrives NOTIFY drivesChanged)
    Q_PROPERTY(QString currentDrive READ currentDrive WRITE setCurrentDrive NOTIFY currentDriveChanged)
    Q_PROPERTY(QString currentLocalFile READ currentLocalFile WRITE setCurrentLocalFile NOTIFY currentLocalFileChanged)



public:
    explicit DiskManager(QObject *parent = nullptr);

    // 设定U盘路径
    Q_INVOKABLE void setCurrentDriveUrl(const QString &url);
    // 设定本地文件路径
    Q_INVOKABLE void setCurrentLocalFileUrl(const QString &url);

    Q_INVOKABLE QString autoDetectMaster();
    // 返回空字符串表示成功，否则返回错误描述
    Q_INVOKABLE void copyFiles(const QString &localDir, const QString &targetDir);
    Q_INVOKABLE void setUserConflictChoice(bool overwrite);

    // 扫描所有可移动设备
    Q_INVOKABLE void scanDrives();
    // 弹出U盘设备
    Q_INVOKABLE void ejectDrive(const QString &path);

    // 逐文件哈希校验
    Q_INVOKABLE void verifyCopy(const QString &localDir, const QString &targetDir);

    Q_INVOKABLE void setCurrentDriveIndex(int index);

    Q_INVOKABLE void checkDog();

//    Q_INVOKABLE QStringList checkConflicts(const QString &sourceDir, const QString &destDir);

    const QString &message() const;
    void setMessage(const QString &newMessage);


    bool isCopySuccessed() const;
    void setIsCopySuccessed(bool newIsCopySuccessed);

    const QString &currentDrive() const;
    void setCurrentDrive(const QString &newCurrentDrive);

    const QStringList &drives() const;
    void setDrives(const QStringList &newDrives);

    const QString &currentLocalFile() const;
    void setCurrentLocalFile(const QString &newCurrentLocalFile);

signals:
    // 复制完成信号
    void copyFinished(bool success, const QString &message);
    void fileConflictDetected(const QString &srcPath, const QString &relativePath);
    void copyProgress(int current, int total);
    void messageChanged();
    void isCopySuccessedChanged();
    void currentDriveChanged();
    void drivesChanged();
    void currentLocalFileChanged();

private:
    bool m_waitingForChoice = false;
    bool m_userChoiceOverwrite = false;
    QString m_message = "";
    bool m_isCopySuccessed = false;
    QString m_currentDrive = "";
    QStringList m_drives;
    QString m_currentLocalFile = "";
};

#endif // DISKMANAGER_H
