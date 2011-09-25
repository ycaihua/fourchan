#include <QtGui/QApplication>
#include <QtDebug>
#include <QCleanlooksStyle>
#include "mainwindow.h"
#include "downloadmanager.h"
#include "thumbnailthread.h"
#include "foldershortcuts.h"
#include "pluginmanager.h"

#if QT_VERSION < 0x040000
 #error "Sorry mate, this application needs Qt4.x.x to run properly."
#endif

DownloadManager* downloadManager;
ThumbnailThread* tnt;
FolderShortcuts* folderShortcuts;
MainWindow* mainWindow;
PluginManager* pluginManager;

void checkEnvironment();

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    a.setStyle("plastique");

    checkEnvironment();

    downloadManager = new DownloadManager();
    tnt = new ThumbnailThread();
    tnt->start(QThread::NormalPriority);

    folderShortcuts = new FolderShortcuts();

    pluginManager = new PluginManager();
    pluginManager->loadPlugins();

    mainWindow = new MainWindow();
    mainWindow->show();
    mainWindow->restoreTabs();

    a.connect(&a, SIGNAL(lastWindowClosed()), mainWindow, SLOT(saveSettings()));

    return a.exec();
}

void checkEnvironment() {
    QDir dir;
    QDir updaterDir;
    QStringList neededFiles;
    QFile f;

    dir.setPath(QApplication::applicationDirPath());
    updaterDir.setPath(dir.path()+"/updater");

#ifdef Q_OS_WIN32
    neededFiles << "Qt4Core.dll" << "QtNetwork4.dll" << "mingwm10.dll" << "libgcc_s_dw2-1.dll" << "au.exe";
#endif

    // Check for updater folders
    if (!updaterDir.exists()) {
        qDebug() << "updater directory does not exists. Creating " << QString("%1/updater").arg(dir.absolutePath());
        dir.mkdir("updater");
    }

    // Check if all files are present
    foreach (QString filename, neededFiles) {
        if (!QFile::exists(QString("%1/%2").arg(updaterDir.absolutePath()).arg(filename))) {
            qDebug() << filename << "does not exists - copying from application dir";
            f.copy(QString("%1/%2").arg(dir.absolutePath()).arg(filename), QString("%1/%2").arg(updaterDir.absolutePath()).arg(filename));
        }
    }
}
