#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "scanner.h"

#include <QCommonStyle>
#include <QDesktopServices>
#include <QDesktopWidget>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QThread>

main_window::main_window(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , scanner(nullptr)
{
    ui->setupUi(this);
    setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, size(), qApp->desktop()->availableGeometry()));

    ui->treeWidget->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->treeWidget->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->treeWidget->setUniformRowHeights(1);
    ui->treeWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);

    QCommonStyle style;
    ui->actionScan_Directory->setIcon(style.standardIcon(QCommonStyle::SP_DialogOpenButton));
    ui->actionExit->setIcon(style.standardIcon(QCommonStyle::SP_DialogCloseButton));
    ui->actionAbout->setIcon(style.standardIcon(QCommonStyle::SP_DialogHelpButton));

    ui->progressBar->setHidden(true);
    ui->progressBar->setRange(0, 100);
    ui->buttonStop->setHidden(true);
    ui->buttonDelete->setHidden(true);

    connect(ui->actionScan_Directory, &QAction::triggered, this, &main_window::selectDirectory);
    connect(ui->actionExit, &QAction::triggered, this, &QWidget::close);
    connect(ui->actionAbout, &QAction::triggered, this, &main_window::showAboutDialog);
    connect(ui->buttonStop, &QPushButton::clicked, this, &main_window::stopScanning);
    connect(ui->buttonDelete, &QPushButton::clicked, this, &main_window::deleteFiles);
}

main_window::~main_window() {
    thread.exit();
    thread.wait();
}

void main_window::selectDirectory()
{
    dir = QFileDialog::getExistingDirectory(this, "Select Directory for Scanning",
                                                    QString(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    QFile check_file(dir);

    ui->treeWidget->clear();
    setWindowTitle(QString("Duplicates in directory - %1").arg(dir));

    scanner = new Scanner();
    scanner->moveToThread(&thread);
    connect(&thread, &QThread::finished, scanner,  &QObject::deleteLater);

    qRegisterMetaType<QVector<QString>>("QVector<QString>");

    ui->progressBar->setHidden(false);
    ui->progressBar->reset();
    ui->progressBar->setValue(0);
    ui->actionScan_Directory->setDisabled(true);
    ui->buttonStop->setHidden(false);
    ui->buttonDelete->setHidden(true);

    connect(scanner, &Scanner::updateProgress, this, &main_window::setProgress);
    connect(scanner, &Scanner::sendDuplicates, this, &main_window::printDuplicates);
    connect(scanner, &Scanner::finish, this, &main_window::finishScanning);
    connect(this, &main_window::findDuplicates, scanner, &Scanner::getDuplicates);

    thread.start();
    emit findDuplicates(dir);
}

void main_window::printDuplicates(QVector<QString> const& duplicates) {
    QTreeWidgetItem* item = new QTreeWidgetItem(ui->treeWidget);

    QFileInfo fileInfo(duplicates[0]);
    item->setText(0, QString(QString::number(duplicates.size())) + " duplicated files found");
    item->setText(1, QString::number(fileInfo.size()));

    for (QString const &path : duplicates) {
        QTreeWidgetItem* childItem = new QTreeWidgetItem();
        QFileInfo file(path);
        childItem->setText(0, path.mid(dir.size(), path.size() - dir.size()));
        item->addChild(childItem);
    }
    ui->treeWidget->addTopLevelItem(item);
}

void main_window::finishScanning() {
    ui->buttonStop->setHidden(true);
    ui->buttonDelete->setHidden(false);
    ui->actionScan_Directory->setDisabled(false);
    thread.quit();
    thread.wait();
}

void main_window::stopScanning() {
    thread.requestInterruption();
}

void main_window::showAboutDialog()
{
    QMessageBox::aboutQt(this);
}

void main_window::setProgress(int percent) {
    ui->progressBar->setValue(percent);
}

void main_window::openFile(QTreeWidgetItem* item, int) {
    QFile file(item->text(0));
    if (file.exists()) {
        QDesktopServices::openUrl(item->text(0));
    }
}

void main_window::deleteFiles() {
    auto selectedItems = ui->treeWidget->selectedItems();
    QSet<QTreeWidgetItem*> deletingItems;

    for (auto const &item : selectedItems) {
        if (item->text(1).size() > 0) {
            auto children = item->takeChildren();
            for (auto const &child : children) {
                deletingItems.insert(child);
                item->addChild(child);
            }
        } else {
            deletingItems.insert(item);
        }
    }

    auto answer = QMessageBox::question(this, "Deleting",
                                        "Do you want to delete " + QString::number(deletingItems.size()) + " selected files?");
    if (answer == QMessageBox::No) {
        return;
    }

    QSet<QTreeWidgetItem*> changedParents;

    for (auto const& item: deletingItems) {
        QFile file(dir + item->text(0));
        if (file.remove()) {
            changedParents.insert(item->parent());
            item->parent()->removeChild(item);
        }
    }

    for (auto const& item: changedParents) {
        if (item->childCount() > 1) {
            item->setText(0, QString::number(item->childCount()) + "Duplicated files");
        } else {
            delete item;
        }
    }
}
