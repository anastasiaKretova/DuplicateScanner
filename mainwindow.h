#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "scanner.h"

#include <QMainWindow>
#include <QTreeWidget>
#include <memory>

namespace Ui {
    class MainWindow;
}

class main_window : public QMainWindow
{
    Q_OBJECT
    QThread thread;

public:
    explicit main_window(QWidget *parent = nullptr);
    ~main_window();

private slots:
    void selectDirectory();
    void showAboutDialog();
    void setProgress(int percent);
    void deleteFiles();
    void printDuplicates(QVector<QString> const& duplicates);
    void openFile(QTreeWidgetItem* item, int);
    void stopScanning();
    void finishScanning();
signals:
    void findDuplicates(QString const& dir);

private:
    std::unique_ptr<Ui::MainWindow> ui;
    Scanner *scanner = nullptr;
    QString dir;
};

#endif // MAINWINDOW_H
