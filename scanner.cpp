#include "scanner.h"

const qint64 PRE = 20;

bool Scanner::checkStop() {
    return QThread::currentThread()->isInterruptionRequested();
}

QString Scanner::getPrefix(QString const& path, qint64 kol) {
    QFile file(path);
    if (file.open(QFile::ReadOnly)) {
        return file.read(kol);
    }
}

QByteArray Scanner::getHash (QString const& path) {
    QFile file(path);
    if (file.open(QFile::ReadOnly)) {
        QCryptographicHash hash(QCryptographicHash::Sha256);
        if (hash.addData(&file)) {
            return hash.result();
        }
    }
    return QByteArray();
}

void Scanner::progress(qint64 curSize, qint64 size, qint8 percent) {
    qint8 newPercent = static_cast<qint8>(curSize * 100 / (size));
    if (newPercent > percent) {
        percent = newPercent;
        emit updateProgress(percent);
    }
}

void Scanner::getDuplicates(QString const &dir) {
    QMap<qint64, QVector<QString>> map1;
    QDirIterator it(dir, QDir::Files, QDirIterator::Subdirectories);
    qint64 curSize = 0, size = 0;
    qint8 percent = 0;

    while (it.hasNext()) {
        QString path = it.next();
        QFileInfo fileInfo = QFileInfo(path);
        map1[fileInfo.size()].push_back(path);
        size += fileInfo.size();
        if (checkStop()) {
            emit finish();
            return;
        }
    }

    for (auto curSizeIt = map1.begin(); curSizeIt != map1.end(); ++curSizeIt) {
        if (checkStop()) {
            emit finish();
            return;
        }
        QMap <QString, QVector<QString>> map2;
        for (auto path : curSizeIt.value()) {
            if (checkStop()) {
                emit finish();
                return;
            }
            map2[getPrefix(path, std::min(PRE, curSizeIt.key()))].push_back(path);
            //curSize += curSizeIt.key();
            //progress(curSize, size, percent);
        }
        if (checkStop()) {
            emit finish();
            return;
        }

        for (auto curPrefixIt = map2.begin(); curPrefixIt != map2.end(); ++curPrefixIt) {
            if (checkStop()) {
                emit finish();
                return;
            }
            QMap <QString, QVector<QString>> map3;
            for (auto path : curPrefixIt.value()) {
                if (checkStop()) {
                    emit finish();
                    return;
                }
                map3[getHash(path)].push_back(path);
                curSize += curSizeIt.key();
                progress(curSize, size, percent);
            }
            if (checkStop()) {
                emit finish();
                return;
            }

            QVector<QVector<QString>> duplicates;
            for (auto curHashIt = map3.begin(); curHashIt != map3.end(); ++curHashIt) {
                if (checkStop()) {
                    emit finish();
                    return;
                }
                if (curHashIt.value().size() != 1) emit sendDuplicates(curHashIt.value());
            }
        }
    }
    emit finish();
}
