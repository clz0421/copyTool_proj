#include "diskmanager.h"
#include "QDebug"
#include "dog_api_cpp.h"
#include "errorprinter.h"
#include "vendor_code.h"


DiskManager::DiskManager(QObject *parent)
    : QObject(parent)
{}

void DiskManager::setCurrentDriveUrl(const QString &url)
{
    QUrl u(url);
    QString targetFile = u.toLocalFile();             // 把 file:/// 转为 /home/... 或 C:/...

    setCurrentDrive(targetFile);
}

void DiskManager::setCurrentLocalFileUrl(const QString &url)
{
    QUrl u(url);
    QString local = u.toLocalFile();

    setCurrentLocalFile(local);
}

QString DiskManager::autoDetectMaster()
{
    const auto volumes = QStorageInfo::mountedVolumes();
    for (const QStorageInfo &vol : volumes) {
        if (!vol.isValid() || vol.isReadOnly() || !vol.isReady())
            continue;
        if (vol.isRoot()) {
            return vol.rootPath();
        }
    }
    return QString(); // 未找到
}

void DiskManager::copyFiles(const QString &localDir, const QString &targetDir)
{
    if (localDir.isEmpty() || targetDir.isEmpty())
    {
        this->setIsCopySuccessed(false);
        this->setMessage(QStringLiteral("U盘路径或本地文件路径不能为空"));
        return;
    }

    QDir src(localDir);
    if (!src.exists())
    {
        this->setIsCopySuccessed(false);
        this->setMessage(QStringLiteral("本地路径不存在"));
        return;
    }

    QDir dst(targetDir);
    if (!dst.exists() && !dst.mkpath(targetDir))
    {
        this->setIsCopySuccessed(false);
        this->setMessage(QStringLiteral("无法打开U盘目录"));
        return;
    }


    this->setMessage(QStringLiteral("正在复制文件。。。"));

    QStringList fileList;
    // 遍历所有文件（递归）
    QDirIterator it(localDir,
                    QDir::Files | QDir::NoSymLinks,
                    QDirIterator::Subdirectories);
    while (it.hasNext()) {
         fileList << it.next();
    }
    int total = fileList.size();
    if (total == 0) {
        this->setIsCopySuccessed(true);
        this->setMessage(QStringLiteral("所选本地路径为空目录"));
        return;
    }

    for(int i = 0; i < total; ++i)
    {
        QString fileSrcPath = fileList[i];                        // 完整源路径
        QString relativePath = src.relativeFilePath(fileSrcPath);
        QString fileDstPath  = dst.filePath(relativePath);

        // 创建目标子目录（若尚不存在）
        QFileInfo fi(fileDstPath);
        QDir().mkpath(fi.path());

        // U盘已经存在相同文件时，让用户选择覆盖还是跳过
        if (QFile::exists(fileDstPath)) {
             emit fileConflictDetected(fileSrcPath, relativePath);
            // 等待用户选择
            m_waitingForChoice = true;
            while (m_waitingForChoice) {
                QCoreApplication::processEvents();            // 保持事件循环响应
            }
//            QFile::remove(fileDstPath);
            if (m_userChoiceOverwrite) {
                qDebug()<<"fileDesPath:  "<<fileDstPath;
                if (!QFile::remove(fileDstPath)) {               // 删除旧文件以便覆盖
                    this->setIsCopySuccessed(false);
                    this->setMessage(QStringLiteral("无法删除已存在文件：%1").arg(relativePath));
                    return;
                }
            } else {
                // 用户选择“跳过”，继续下一文件
//                emit copyProgress(i + 1, total);
                continue;
            }
        }

        // 执行复制
        if (!QFile::copy(fileSrcPath, fileDstPath)) {
            this->setIsCopySuccessed(false);
            this->setMessage(QStringLiteral("复制失败：%1 → %2")
                             .arg(fileSrcPath, fileDstPath));
            return;
        }

        // 文件大小校验
        qint64 sizeSrc = QFileInfo(fileSrcPath).size();
        qint64 sizeDst = QFileInfo(fileDstPath).size();
        if (sizeSrc != sizeDst) {
            this->setIsCopySuccessed(false);
            this->setMessage(QStringLiteral("文件大小不一致：%1 (%2 字节) → %3 (%4 字节)")
                             .arg(relativePath)
                             .arg(sizeSrc)
                             .arg(relativePath)
                             .arg(sizeDst));
            return;
        }
//        emit copyProgress(i + 1, total);
    }

    this->setIsCopySuccessed(true);
    this->setMessage(QStringLiteral("复制完成"));

    return;
}

void DiskManager::setUserConflictChoice(bool overwrite)
{
    m_userChoiceOverwrite = overwrite;
    m_waitingForChoice = false;
}

void DiskManager::scanDrives()
{
    QStringList list;
    const auto vols = QStorageInfo::mountedVolumes();
    for (const QStorageInfo &vol : vols) {
        if (!vol.isValid() || !vol.isReady() || vol.isReadOnly())
            continue;

        QString root = vol.rootPath();
#ifdef Q_OS_WIN
        UINT type = GetDriveTypeW(reinterpret_cast<LPCWSTR>(root.utf16()));
        if (type == DRIVE_REMOVABLE) {
            list << root;
        }
#elif defined(Q_OS_UNIX)
        QString fs = vol.fileSystemType().toLower();
        if ((root.startsWith("/media") || root.startsWith("/run/media"))
             && (fs == "vfat" || fs == "exfat" || fs == "ntfs")) {
            list << root;
        }
#else
        // 通用 Qt5+ 平台
        if (vol.isRemovable()) {
            list << root;
        }
#endif
    }

    if (!list.isEmpty()) {
        setDrives(list);
        // 如果只有一个设备，自动选中
        if (m_drives.size() == 1) {
            setCurrentDrive(m_drives.first());
        }
    }
}

void DiskManager::ejectDrive(const QString &path)
{
    if (path.isEmpty()) {
        this->setMessage(QStringLiteral("U盘路径不能为空"));
        return;
    }

    // 根据挂载点获取设备节点（如 "/dev/sdb1"）
    QStorageInfo info(path);
    QString device = info.device();

#ifdef Q_OS_WIN
    // Windows 平台调用 CM_Request_Device_Eject 安全移除
    QString root = path;
    if (root.endsWith('/') || root.endsWith('\\'))
        root.chop(1);
    QString devicePath = QStringLiteral("\\\\.\\") + root;

    // 打开设备
    HANDLE hDev = CreateFileW(
        reinterpret_cast<LPCWSTR>(devicePath.utf16()),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        0,
        nullptr
    );
    if (hDev == INVALID_HANDLE_VALUE) {
        this->setMessage(QStringLiteral("打开设备失败"));
        return;
    }

    // 弹出 U 盘
    DWORD bytesReturned = 0;
    BOOL ok = DeviceIoControl(
        hDev,
        IOCTL_STORAGE_EJECT_MEDIA,
        nullptr, 0,
        nullptr, 0,
        &bytesReturned,
        nullptr
    );
    CloseHandle(hDev);

    if (ok) {
        this->setMessage(QStringLiteral("弹出成功"));
    } else {
        this->setMessage(QStringLiteral("弹出失败：失败代码 %1").arg(GetLastError()));
    }

    /*// 1. 从 path（如 "E:/" 或 "E:\\"）提取盘符 "E:"
    QString root = path;
    if (root.endsWith('/') || root.endsWith('\\'))
        root.chop(1);
    root = root.left(2);

    // 2. 调用 mountvol 卸载并断电:
    //    mountvol E: /p  （永久卸载并断电）
    //    mountvol E: /d  （只卸载挂载点）
    int unmount = QProcess::execute("mountvol", QStringList() << root << "/d");
//    int remount = QProcess::execute("mountvol", QStringList() << root << "/p");

    if (unmount == 0) {
        this->setMessage(QStringLiteral("弹出成功"));
    } else {
        this->setMessage(QStringLiteral("弹出失败：失败代码 %1").arg(unmount));
    }
    */

    /*
//    WCHAR driveLetter = device.toWCharArray()[4]; // 假设 device 是 "\\\\.\\E:"
    DEVINST devInst = 0;
//    WCHAR rootPath[] = { driveLetter, L':', L'\\', 0 };

    QString root = path;
    if (root.endsWith('/') || root.endsWith('\\'))
        root.chop(1);
    QString driveId = root.left(2);         // e.g. "E:"

    // 转成 UTF-16 宽字符，直接用 utf16() 方法
    LPCWSTR deviceIdW = reinterpret_cast<LPCWSTR>(driveId.utf16());

    // 获取设备实例句柄
    if (CM_Get_Parent(&devInst, CM_Locate_DevNodeW(&devInst, deviceIdW, CM_LOCATE_DEVNODE_NORMAL), 0) != CR_SUCCESS) {
        this->setMessage("找不到设备实例");
        return;
    }
    CONFIGRET status = CM_Request_Device_Eject(devInst, nullptr, nullptr, 0, 0);
    if (status == CR_SUCCESS)
        this->setMessage("弹出成功");
    else
        this->setMessage(tr("弹出失败：%1").arg(status));
        */
#elif defined(Q_OS_UNIX)
    // Linux 平台：先卸载，再断电
    int r1 = QProcess::execute("umount", QStringList() << path);
    int r2 = QProcess::execute("udisksctl", QStringList() << "power-off"
                                                     << "-b" << device);
    if (r1 == 0 && r2 == 0)
        this->setMessage(QStringLiteral("弹出成功"));
    else
        this->setMessage(QStringLiteral("弹出失败"));
#else
    Q_UNUSED(path)
#endif
}

void DiskManager::verifyCopy(const QString &localDir, const QString &targetDir)
{
    if (localDir.isEmpty() || targetDir.isEmpty()) {
        this->setMessage(QStringLiteral("U盘路径或本地文件路径不能为空"));
        return;
    }

    QDirIterator it(localDir,
                    QDir::Files | QDir::NoSymLinks,
                    QDirIterator::Subdirectories);                         // 递归遍历所有文件
    while (it.hasNext()) {
        QString srcPath = it.next();                                   // 源文件完整路径
        QString relPath = QDir(localDir).relativeFilePath(srcPath);
        QString dstPath = QDir(targetDir).filePath(relPath);

        QFile srcFile(srcPath);
        if (!srcFile.open(QIODevice::ReadOnly)) {
            this->setMessage(QStringLiteral("无法打开本地文件：%1").arg(relPath));
            return;
        }

        // 1. 计算源文件 MD5
        QCryptographicHash hash(QCryptographicHash::Md5);
        if (!hash.addData(&srcFile)) {                                 // 读取整个文件并哈希
            this->setMessage(QStringLiteral("本地文件哈希失败：%1").arg(relPath));
            return;
        }
        QByteArray srcDigest = hash.result();                          // 获取哈希值

        // 2. 计算目标文件 MD5
        QFile dstFile(dstPath);
        if (!dstFile.open(QIODevice::ReadOnly)) {
            this->setMessage(QStringLiteral("无法打开U盘文件：%1").arg(relPath));
            return;
        }
        hash.reset();                                                  // 重置哈希对象
        if (!hash.addData(&dstFile)) {
            this->setMessage(QStringLiteral("U盘文件哈希失败：%1").arg(relPath));
            return;
        }
        QByteArray dstDigest = hash.result();

        // 3. 对比
        if (srcDigest != dstDigest) {
            this->setMessage(QStringLiteral("文件校验不一致：%1").arg(relPath));
            return;
        }
    }

    // 全部通过
    this->setMessage(QStringLiteral("所有文件校验通过"));
}

void DiskManager::setCurrentDriveIndex(int index)
{
    setCurrentDrive(m_drives[index]);
}

void DiskManager::checkDog()
{
    CDog dog1(CDogFeature::defaultFeature());
    dogStatus status = dog1.login(vendorCode);

    ErrorPrinter errorPrinter;
    if (status == DOG_STATUS_OK)
    {
        this->setMessage(QStringLiteral("加密狗已接入"));
        return ;
    }
    else
    {
        errorPrinter.getError(status);
        this->setMessage(QStringLiteral("请检查加密狗是否接入"));
        return ;
    }
}

const QString &DiskManager::message() const
{
    return m_message;
}

void DiskManager::setMessage(const QString &newMessage)
{
    if (m_message == newMessage)
        return;
    m_message = newMessage;
    emit messageChanged();
}

bool DiskManager::isCopySuccessed() const
{
    return m_isCopySuccessed;
}

void DiskManager::setIsCopySuccessed(bool newIsCopySuccessed)
{
    if (m_isCopySuccessed == newIsCopySuccessed)
        return;
    m_isCopySuccessed = newIsCopySuccessed;
    emit isCopySuccessedChanged();
}

const QString &DiskManager::currentDrive() const
{
    return m_currentDrive;
}

void DiskManager::setCurrentDrive(const QString &newCurrentDrive)
{
    if (m_currentDrive == newCurrentDrive)
        return;
    m_currentDrive = newCurrentDrive;
    emit currentDriveChanged();
}

const QStringList &DiskManager::drives() const
{
    return m_drives;
}

void DiskManager::setDrives(const QStringList &newDrives)
{
    if (m_drives == newDrives)
        return;
    m_drives = newDrives;
    emit drivesChanged();
}

const QString &DiskManager::currentLocalFile() const
{
    return m_currentLocalFile;
}

void DiskManager::setCurrentLocalFile(const QString &newCurrentLocalFile)
{
    if (m_currentLocalFile == newCurrentLocalFile)
        return;
    m_currentLocalFile = newCurrentLocalFile;
    emit currentLocalFileChanged();
}
