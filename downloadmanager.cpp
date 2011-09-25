#include "downloadmanager.h"

#define MAX_CONCURRENT_DOWNLOADS_PER_NAM 5

DownloadManager::DownloadManager(QObject *parent) :
//    QThread(parent)
    QObject(parent)
{
/*
}

void DownloadManager::run() {
*/
    nams.clear();
    nams.append(new NetworkAccessManager(this));    // Add at least one AccessManager
    settings = new QSettings("settings.ini", QSettings::IniFormat);

    waitTimer = new QTimer();
    waitTimer->setSingleShot(true);
    waitTimer->setInterval(20000);

    requestList.clear();
    lastid = 0;
    currentRequests = 0;

    totalRequests = 0;
    finishedRequests = 0;

    statistic_downloadedFiles  = 0;
    statistic_downloadedKBytes = 0;

    loadSettings();
    serviceAvailable = true;

    connect(nams.at(0), SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));
    connect(waitTimer, SIGNAL(timeout()), this, SLOT(resumeDownloads()));

//    exec();
}

void DownloadManager::loadSettings() {
    settings->beginGroup("download_manager");
        maxRequests = settings->value("concurrent_downloads", 20).toInt();
        initialTimeout = settings->value("initial_timeout", 30).toInt()*1000;
        runningTimeout = settings->value("running_timeout", 20).toInt()*1000;
    settings->endGroup();

    settings->beginGroup("statistics");
        statistic_downloadedFiles = settings->value("downloaded_files", 0).toFloat();
        statistic_downloadedKBytes = settings->value("downloaded_kbytes", 0).toFloat();
    settings->endGroup();

    setupNetworkAccessManagers(qCeil(maxRequests/MAX_CONCURRENT_DOWNLOADS_PER_NAM));
}

void DownloadManager::replyFinished(QNetworkReply* reply) {
    QString redirect;
    qint64 uid;
    QList<QNetworkReply*> replies;
    DownloadRequest* dr;
    int repliesRemoved;

    // Search in requestList for this reply
    replies = activeReplies.values();
    uid = activeReplies.key(reply, -1);
    dr = requestList.value(uid,0);
    repliesRemoved = activeReplies.remove(uid);
    supervisors.value(uid)->deleteLater();
    supervisors.remove(uid);
    priorities.remove(priorities.key(uid), uid);

//    qDebug() << QTime::currentTime().toString("hh:mm:ss") << "Finished request" << uid << reply->url().toString() << "reply" << (qint64)reply;

    if (uid == 0) {
        qDebug() << "uid 0 finished; url=" << reply->url().toString();
    }
    if (uid != -1) {
        if (reply->bytesAvailable() < reply->header(QNetworkRequest::ContentLengthHeader).toLongLong()) {
            qDebug() << "Received less byte than expected - Possibly because the download timed out";
            reschedule(uid);
        }
        else {
            redirect = reply->header(QNetworkRequest::LocationHeader).toString();
            if (!redirect.isEmpty()) {
                if (dr != 0) {
                    dr->setUrl(QUrl(redirect));
                    dr->setProcessing(false);
                    priorities.insertMulti(-1, uid);
                }
            }
            else if (reply->error() != QNetworkReply::NoError) {
                handleError(uid, reply);
            }
            else {
                if (reply->isFinished()) {
                    dr->setResponse(reply->readAll());
                    dr->requestHandler()->requestFinished(uid);

                    emit finishedRequestsChanged(++finishedRequests);
                    if (reply->url().toString().indexOf(QRegExp("(\\.jpg|\\.gif|\\.jpeg|\\.png)", Qt::CaseInsensitive)) != -1) {
                        statistic_downloadedFiles++;
                        statistic_downloadedKBytes += ((reply->header(QNetworkRequest::ContentLengthHeader).toFloat())/1024);
                    }
                }
            }
        }
    }
    else {
        qDebug() << "I did not find the request to the reply!";
    }

    currentRequests--;
    reply->deleteLater();

    if (activeReplies.count() > (maxRequests+10)) {
        qDebug() << "There are more active requests than allowed. Setting service to unavailable";
        serviceAvailable = false;

        foreach (QNetworkReply* r, activeReplies) {
            r->abort();
        }

        waitTimer->start();
    }

    processRequests();
}

QByteArray DownloadManager::getByteArray(qint64 uid) {
    QByteArray ret;
    DownloadRequest* dr;

    dr = requestList.value(uid,0);

    if (dr != 0) {
        ret = dr->response();
    }

    return ret;
}

void DownloadManager::freeRequest(qint64 uid) {
    DownloadRequest* dr;
    dr = requestList.value(uid, 0);
//    qDebug() << "freeRequest(" << uid << ")";

    if (dr != 0) {
        dr->deleteLater();
        requestList.remove(uid);
    }
//    qDebug() << "open requests" << requestList.keys();
//    qDebug() << "priorities" << priorities;
}

qint64 DownloadManager::requestDownload(RequestHandler* caller, QUrl url, int prio) {
    DownloadRequest* dr;
    qint64 uid;

    dr = new DownloadRequest();
    dr->setRequestHandler(caller);
    dr->setUrl(url);
    dr->setPriority(prio);

    uid = getUID();

//    addRequest(uid, dr);
    requestList.insert(uid, dr);
    priorities.insertMulti(prio, uid);

    processRequests();

    return uid;
}

/**
 * Add Request to internal list and start Thread/Download immediately if max. number of downloads is not met.
 */
void DownloadManager::addRequest(qint64 uid, DownloadRequest* dr) {
    requestList.insert(uid, dr);

    processRequests();
    emit totalRequestsChanged(++totalRequests);
}

void DownloadManager::downloadTimeout(qint64 uid) {
    QNetworkReply* r;

    r = activeReplies.value(uid);

    if (r != 0) {
//        qDebug() << uid << QString(r->readAll());
        r->abort();
    }
}

void DownloadManager::processRequests() {
    QList<qint64> uids;

    if (serviceAvailable) {

        //    uids = requestList.keys();
        uids = priorities.values();

        if (uids.count() > 0) {
            foreach (qint64 uid, uids) {
                //            if (currentRequests >= maxRequests) {
                if (activeReplies.count() >= maxRequests) {
                    break;
                }
                else {
                    if (requestList.count(uid)>0) {
                        if (!(requestList.value(uid)->finished()) && !(requestList.value(uid)->processing())) {
                            startRequest(uid);
//                            qDebug() << "addRequest (" << uid << ")";
                        }
                    }
                }
            }
        }
        else {
            // This should not happen but if there are no (more) priorities set for some downloads, reset them
            if (requestList.count() > 0) {
                uids = requestList.keys();
                foreach(qint64 uid, uids) {
                    priorities.insertMulti(uid, 0);
                }

                processRequests();
            }
        }
    }
}

void DownloadManager::startRequest(qint64 uid) {
    QNetworkRequest req;
    QNetworkReply* rep;
    NetworkAccessManager* nam;
    DownloadRequest* dr;
    SupervisedNetworkReply* sup;

    dr = requestList.value(uid, 0);

    if (dr != 0 && serviceAvailable) {
        dr->setProcessing(true);
        sup = new SupervisedNetworkReply();
        sup->setTimeouts(initialTimeout, runningTimeout);
        connect(sup, SIGNAL(timeout(qint64)), this, SLOT(downloadTimeout(qint64)));

        req = QNetworkRequest(dr->url());
        req.setAttribute(QNetworkRequest::CookieSaveControlAttribute, QNetworkRequest::Automatic);
//        req.setRawHeader("User-Agent", "Wget/1.12");
        req.setRawHeader("User-Agent", "Opera/9.80 (Windows NT 6.1; U; en) Presto/2.9.168 Version/11.50");
        currentRequests++;
        nam = getFreeNAM();
        rep = nam->get(req);

        sup->setNetworkReply(rep, uid);
        supervisors.insert(uid, sup);

//        qDebug() << "Requesting" << uid << ":" << dr->url();// << "with reply" << (qint64)rep;
        activeReplies.insert(uid, rep);
    }
    else {
        qDebug() << "Requested start of uid which is non-existent";
    }
}

void DownloadManager::handleError(qint64 uid, QNetworkReply* r) {
    DownloadRequest* dr;
    dr = requestList.value(uid);

    qDebug() << uid << "received error" << r->error() << ":" << r->errorString();

    switch (r->error()) {
    case 404:
    case 203:
        dr->requestHandler()->error(uid, 404);

        currentRequests--;
        processRequests();

        break;

    case 301:   // Service unavailable
        qDebug() << "Service unavailable";
        emit error("Service unavailable");
        // Pause downloading to let the server relax
        serviceAvailable = false;
        // Abort all downloads
        foreach (QNetworkReply* r, activeReplies) {
            r->abort();
        }

        waitTimer->start();
        break;
    case 205:
    case 99:
    case 299:
    case 5:         // aborted
    case 2:         // Connection closed
        //        qDebug() << "response:" << QString(r->readAll());
        //        qDebug() << "finished:" << r->isFinished();
        reschedule(uid);

//        emit message(QString("Rescheduled %1").arg(r->url().toString()));
        break;

    case 202:
        dr->requestHandler()->error(uid, 202);

        currentRequests--;
        processRequests();

    case 3:
        qDebug() << "Host not found error for URL" << r->url().toString();
        currentRequests--;
        reschedule(uid);    // Try harder
        processRequests();
        break;

    default:
        qDebug() << "Unhandled error " << r->error();
//        r.caller->error(r.uid, 404);
        reschedule(uid);        // Since we don't know what happened, try harder
        currentRequests--;
        processRequests();

        break;
    }
}

void DownloadManager::reschedule(qint64 uid) {
    DownloadRequest* dr;
    int prio;

//    qDebug() << "rescheduling" << uid;
    dr = requestList.value(uid,0);
    if (dr != 0) {
        prio = dr->priority();
        dr->setProcessing(false);

        // "decrease priority for rescheduled downloads"
        priorities.remove(prio, uid);
        prio++;
        qDebug() << uid << ":" << "setting new priority" << prio;
        dr->setPriority(prio);
        priorities.insertMulti(prio,uid);

        dr->reset();
        currentRequests--;
        processRequests();
    }
}

void DownloadManager::resumeDownloads() {
    QList<qint64> uids;

    qDebug() << "rechecking service status";
    // Start only one request to check if service is available again
    serviceAvailable = true;
    uids = priorities.values();

    foreach (qint64 uid, uids) {
        if (currentRequests >= maxRequests) {
            break;
        }
        else {
            if (!(requestList.value(uid)->finished()) && !(requestList.value(uid)->processing())) {
                startRequest(uid);
                break;
            }
        }
    }
}

void DownloadManager::removeRequest(qint64 uid) {
    int prio;

    if (requestList.count(uid)>0) {
        prio = requestList.value(uid)->priority();
        priorities.remove(prio, uid);
        requestList.remove(uid);

        // See if this id is currently downloading
        if (activeReplies.count(uid) > 0) {
            supervisors.remove(uid);
            activeReplies.value(uid)->abort();
        }
    }
}

void DownloadManager::getStatistics(int *files, float *kbytes) {
    *files = statistic_downloadedFiles;
    *kbytes = statistic_downloadedKBytes;
}

NetworkAccessManager* DownloadManager::getFreeNAM() {
    NetworkAccessManager* ret;
    QMap<int, NetworkAccessManager*> load;

    for (int i=0; i<nams.count(); i++) {
        load.insertMulti(nams.at(i)->activeRequests(), nams.at(i));
    }

    ret = load.begin().value(); // Returns NetworkAccessManager with the least active requests

    return ret;
}

void DownloadManager::setupNetworkAccessManagers(int count) {
    for (int i=1; i<=count; i++) {
        nams.append(new NetworkAccessManager(this));
        connect(nams.last(), SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));
    }
}
