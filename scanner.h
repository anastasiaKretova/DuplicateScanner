#ifndef SCANNER_H
#define SCANNER_H

#include <QObject>
#include <QCryptographicHash>
#include <QDirIterator>
#include <QFile>
#include <QThread>
#include <QVector>
#include <QMap>

class Scanner : public QObject {
    Q_OBJECT

public :
    ~Scanner() = default;

public slots:
    void getDuplicates(QString const &dir);
    signals:
    void sendDuplicates(QVector<QString> const&);
    void updateProgress(int percent);
    void finish();

private:
    bool checkStop();
    void progress(qint64 curSize, qint64 size, qint8 percent);
    QByteArray getHash(QString const&);
    QString getPrefix(QString const& filepath, qint64 number);
};

#endif // SCANNER_H
