#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "gamewidget.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle(QStringLiteral("阿尔托的冒险"));

    if (auto *mb = menuBar()) mb->hide();
    if (auto *sb = statusBar()) sb->hide();

    auto *game = new GameWidget(this);
    setCentralWidget(game);
    game->setFocus();

    resize(1024, 640);
}

MainWindow::~MainWindow()
{
    delete ui;
}
