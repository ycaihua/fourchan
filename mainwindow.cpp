#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->tabWidget->removeTab(0);
    addTab();

    defaultDirectory = "";

    connect(ui->tabWidget, SIGNAL(tabCloseRequested(int)), this, SLOT(closeTab(int)));
}

int MainWindow::addTab() {
    int ci;
    UI4chan* w;

    w = new UI4chan(this);

    ci = ui->tabWidget->addTab(w, "no name");
    w->setDirectory(defaultDirectory);

    connect(w, SIGNAL(errorMessage(QString)), this, SLOT(displayError(QString)));
    connect(w, SIGNAL(tabTitleChanged(UI4chan*, QString)), this, SLOT(changeTabTitle(UI4chan*, QString)));
    connect(w, SIGNAL(directoryChanged(QString)), this, SLOT(setDefaultDirectory(QString)));

    ui->tabWidget->setCurrentIndex(ci);

    changeTabTitle(w, "idling");

    return ci;
}

void MainWindow::closeTab(int i) {
    UI4chan* w;

    if (ui->tabWidget->count()>1) {
        ui->tabWidget->setCurrentIndex(i);
        w = (UI4chan*)ui->tabWidget->widget(i);
        w->close();
        ui->tabWidget->removeTab(i);
    }
}

void MainWindow::displayError(QString s) {
    ui->statusBar->showMessage(s, 3000);
}

void MainWindow::showInfo(void) {
    UIInfo* uiInfo;

    uiInfo = new UIInfo(this);
    uiInfo->show();
}

void MainWindow::setDefaultDirectory(QString d) {
    defaultDirectory = d;
}

void MainWindow::changeTabTitle(UI4chan* w, QString s) {
    int i;

    i = ui->tabWidget->indexOf((QWidget*)w);
    ui->tabWidget->setTabText(i, s);
}


MainWindow::~MainWindow()
{
    delete ui;
}
