#include "gamewindow.h"
#include <QKeyEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QFont>
#include <QGraphicsProxyWidget>
#include <QMessageBox>
#include <QPen>
#include <QBrush>

GameWindow::GameWindow(QWidget *parent)
    : QGraphicsView(parent)
{
    setWindowTitle("越野滑雪大冒险");
    setFixedSize(1280, 720);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setRenderHint(QPainter::Antialiasing);

    m_scene = new QGraphicsScene(this);
    m_scene->setSceneRect(0, 0, width(), height());
    setScene(m_scene);

    m_gameTimer = new QTimer(this);
    m_gameTimer->setInterval(16); // ~60 FPS
    connect(m_gameTimer, &QTimer::timeout, this, &GameWindow::updateGame);

    m_spawnTimer = new QTimer(this);
    connect(m_spawnTimer, &QTimer::timeout, this, &GameWindow::spawnObstacle);

    m_player = nullptr;
    m_gameState = STATE_MENU;
    m_difficulty = DIFFICULTY_MEDIUM;
    m_score = 0;
    m_reviveCount = 2;
    m_gameSpeed = 5.0;

    initScene();
    initMenuUI();
    initGameUI();
    initGameOverUI();

    setGameState(STATE_MENU);
}

GameWindow::~GameWindow()
{
}

void GameWindow::initScene()
{
    m_scene->setBackgroundBrush(QBrush(QColor(240, 248, 255)));
    QGraphicsRectItem *ground = new QGraphicsRectItem(0, height() - 100, width(), 100);
    ground->setBrush(QBrush(QColor(220, 240, 255)));
    ground->setPen(Qt::NoPen);
    m_scene->addItem(ground);
}

void GameWindow::initMenuUI()
{
    m_menuWidget = new QWidget();
    m_menuWidget->setFixedSize(400, 500);
    m_menuWidget->setStyleSheet("background-color: rgba(255,255,255,200); border-radius: 10px;");

    QVBoxLayout *layout = new QVBoxLayout(m_menuWidget);
    layout->setContentsMargins(30, 30, 30, 30);
    layout->setSpacing(20);

    QLabel *titleLabel = new QLabel("越野滑雪大冒险");
    titleLabel->setAlignment(Qt::AlignCenter);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(28);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    layout->addWidget(titleLabel);

    QHBoxLayout *diffLayout = new QHBoxLayout();
    QLabel *diffLabel = new QLabel("难度选择：");
    diffLabel->setFont(QFont("Microsoft YaHei", 14));
    m_difficultyCombo = new QComboBox();
    m_difficultyCombo->addItem("简单", DIFFICULTY_EASY);
    m_difficultyCombo->addItem("中等", DIFFICULTY_MEDIUM);
    m_difficultyCombo->addItem("困难", DIFFICULTY_HARD);
    m_difficultyCombo->setCurrentIndex(1);
    m_difficultyCombo->setFont(QFont("Microsoft YaHei", 14));
    m_difficultyCombo->setFixedHeight(40);
    connect(m_difficultyCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &GameWindow::updateDifficulty);
    diffLayout->addWidget(diffLabel);
    diffLayout->addWidget(m_difficultyCombo);
    layout->addLayout(diffLayout);

    QLabel *tipLabel = new QLabel(
        "操作说明：\n"
        "空格：跳跃\n"
        "W：前空翻（空中生效）\n"
        "S：后空翻（空中生效）\n"
        "Shift：加速\n"
        "P：暂停/继续\n"
        "R：重新开始"
        );
    tipLabel->setFont(QFont("Microsoft YaHei", 12));
    tipLabel->setAlignment(Qt::AlignLeft);
    layout->addWidget(tipLabel);

    QPushButton *startBtn = new QPushButton("开始游戏");
    startBtn->setFont(QFont("Microsoft YaHei", 16, QFont::Bold));
    startBtn->setFixedHeight(50);
    startBtn->setStyleSheet("background-color: #4CAF50; color: white; border-radius: 8px;");
    connect(startBtn, &QPushButton::clicked, this, &GameWindow::startGame);
    layout->addWidget(startBtn);

    QPushButton *exitBtn = new QPushButton("退出游戏");
    exitBtn->setFont(QFont("Microsoft YaHei", 14));
    exitBtn->setFixedHeight(40);
    exitBtn->setStyleSheet("background-color: #f44336; color: white; border-radius: 8px;");
    connect(exitBtn, &QPushButton::clicked, this, &QWidget::close);
    layout->addWidget(exitBtn);

    layout->addStretch();

    QGraphicsProxyWidget *proxy = m_scene->addWidget(m_menuWidget);
    proxy->setPos((width() - m_menuWidget->width())/2, (height() - m_menuWidget->height())/2);
}

void GameWindow::initGameUI()
{
    m_scoreLabel = new QLabel("分数：0");
    m_scoreLabel->setFont(QFont("Microsoft YaHei", 16, QFont::Bold));
    m_scoreLabel->setStyleSheet("color: #2c3e50; background-color: rgba(255,255,255,150); padding: 5px; border-radius: 5px;");
    QGraphicsProxyWidget *scoreProxy = m_scene->addWidget(m_scoreLabel);
    scoreProxy->setPos(20, 20);

    m_hpLabel = new QLabel("生命值：3");
    m_hpLabel->setFont(QFont("Microsoft YaHei", 16, QFont::Bold));
    m_hpLabel->setStyleSheet("color: #e74c3c; background-color: rgba(255,255,255,150); padding: 5px; border-radius: 5px;");
    QGraphicsProxyWidget *hpProxy = m_scene->addWidget(m_hpLabel);
    hpProxy->setPos(20, 60);

    QLabel *energyLabel = new QLabel("加速能量：");
    energyLabel->setFont(QFont("Microsoft YaHei", 12));
    energyLabel->setStyleSheet("color: #2980b9; background-color: rgba(255,255,255,150); padding: 3px; border-radius: 3px;");
    QGraphicsProxyWidget *energyLabelProxy = m_scene->addWidget(energyLabel);
    energyLabelProxy->setPos(20, 100);

    m_energyBar = new QProgressBar();
    m_energyBar->setRange(0, 100);
    m_energyBar->setValue(100);
    m_energyBar->setFixedSize(200, 20);
    m_energyBar->setStyleSheet("QProgressBar { border: 1px solid #2980b9; border-radius: 3px; background-color: white; } QProgressBar::chunk { background-color: #3498db; }");
    QGraphicsProxyWidget *energyProxy = m_scene->addWidget(m_energyBar);
    energyProxy->setPos(120, 100);
}

void GameWindow::initGameOverUI()
{
    m_gameOverWidget = new QWidget();
    m_gameOverWidget->setFixedSize(400, 400);
    m_gameOverWidget->setStyleSheet("background-color: rgba(255,255,255,220); border-radius: 10px;");

    QVBoxLayout *layout = new QVBoxLayout(m_gameOverWidget);
    layout->setContentsMargins(30, 30, 30, 30);
    layout->setSpacing(20);

    QLabel *titleLabel = new QLabel("游戏结束");
    titleLabel->setAlignment(Qt::AlignCenter);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(28);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setStyleSheet("color: #e74c3c;");
    layout->addWidget(titleLabel);

    QLabel *finalScoreLabel = new QLabel("最终分数：0");
    finalScoreLabel->setAlignment(Qt::AlignCenter);
    finalScoreLabel->setFont(QFont("Microsoft YaHei", 20, QFont::Bold));
    finalScoreLabel->setObjectName("finalScoreLabel");
    layout->addWidget(finalScoreLabel);

    QPushButton *reviveBtn = new QPushButton("复活（剩余2次）");
    reviveBtn->setObjectName("reviveBtn");
    reviveBtn->setFont(QFont("Microsoft YaHei", 14, QFont::Bold));
    reviveBtn->setFixedHeight(50);
    reviveBtn->setStyleSheet("background-color: #ff9800; color: white; border-radius: 8px;");
    connect(reviveBtn, &QPushButton::clicked, this, &GameWindow::revivePlayer);
    layout->addWidget(reviveBtn);

    QPushButton *restartBtn = new QPushButton("重新开始");
    restartBtn->setFont(QFont("Microsoft YaHei", 14));
    restartBtn->setFixedHeight(40);
    restartBtn->setStyleSheet("background-color: #4CAF50; color: white; border-radius: 8px;");
    connect(restartBtn, &QPushButton::clicked, this, &GameWindow::restartGame);
    layout->addWidget(restartBtn);

    QPushButton *menuBtn = new QPushButton("返回主菜单");
    menuBtn->setFont(QFont("Microsoft YaHei", 14));
    menuBtn->setFixedHeight(40);
    menuBtn->setStyleSheet("background-color: #2196F3; color: white; border-radius: 8px;");
    connect(menuBtn, &QPushButton::clicked, this, [=](){
        setGameState(STATE_MENU);
    });
    layout->addWidget(menuBtn);

    layout->addStretch();

    QGraphicsProxyWidget *proxy = m_scene->addWidget(m_gameOverWidget);
    proxy->setPos((width() - m_gameOverWidget->width())/2, (height() - m_gameOverWidget->height())/2);
    m_gameOverWidget->hide();
}

void GameWindow::setGameState(GameState state)
{
    m_gameState = state;

    switch (state) {
    case STATE_MENU:
        m_menuWidget->show();
        m_gameOverWidget->hide();
        m_gameTimer->stop();
        m_spawnTimer->stop();
        clearScene();
        break;
    case STATE_PLAYING:
        m_menuWidget->hide();
        m_gameOverWidget->hide();
        m_gameTimer->start();
        m_spawnTimer->start();
        break;
    case STATE_PAUSED:
        m_gameTimer->stop();
        m_spawnTimer->stop();
        QMessageBox::information(this, "暂停", "游戏已暂停，点击确定继续");
        resumeGame();
        break;
    case STATE_GAMEOVER:
        m_gameTimer->stop();
        m_spawnTimer->stop();
        m_gameOverWidget->show();
        break;
    }
}

void GameWindow::clearScene()
{
    QList<QGraphicsItem*> items = m_scene->items();
    for (auto item : items) {
        if (dynamic_cast<Obstacle*>(item) || dynamic_cast<Player*>(item)) {
            m_scene->removeItem(item);
            delete item;
        }
    }
    m_player = nullptr;
}

void GameWindow::startGame()
{
    clearScene();

    m_player = new Player();
    m_player->setPos(150, height() - 100 - m_player->boundingRect().height());
    m_scene->addItem(m_player);

    m_score = 0;
    m_gameSpeed = 5.0;
    switch (m_difficulty) {
    case DIFFICULTY_EASY:
        m_reviveCount = 3;
        m_player->setHp(3);
        m_spawnTimer->setInterval(1500);
        break;
    case DIFFICULTY_MEDIUM:
        m_reviveCount = 2;
        m_player->setHp(2);
        m_spawnTimer->setInterval(1000);
        break;
    case DIFFICULTY_HARD:
        m_reviveCount = 1;
        m_player->setHp(1);
        m_spawnTimer->setInterval(600);
        break;
    }

    updateUI();
    setGameState(STATE_PLAYING);
}

void GameWindow::pauseGame()
{
    if (m_gameState == STATE_PLAYING) {
        setGameState(STATE_PAUSED);
    }
}

void GameWindow::resumeGame()
{
    if (m_gameState == STATE_PAUSED) {
        setGameState(STATE_PLAYING);
    }
}

void GameWindow::gameOver()
{
    setGameState(STATE_GAMEOVER);
    QLabel *finalScoreLabel = m_gameOverWidget->findChild<QLabel*>("finalScoreLabel");
    if (finalScoreLabel) {
        finalScoreLabel->setText(QString("最终分数：%1").arg(m_score));
    }
    QPushButton *reviveBtn = m_gameOverWidget->findChild<QPushButton*>("reviveBtn");
    if (reviveBtn) {
        reviveBtn->setText(QString("复活（剩余%1次）").arg(m_reviveCount));
        reviveBtn->setEnabled(m_reviveCount > 0);
    }
}

void GameWindow::revivePlayer()
{
    if (m_reviveCount <= 0) return;

    m_reviveCount--;
    if (m_player) {
        m_player->setHp(m_difficulty == DIFFICULTY_HARD ? 1 : 2);
        m_player->setInvincible(true);
        QTimer::singleShot(3000, [this]() {
            if (m_player) m_player->setInvincible(false);
        });
    }

    QList<QGraphicsItem*> items = m_scene->items();
    for (auto item : items) {
        if (dynamic_cast<Obstacle*>(item)) {
            m_scene->removeItem(item);
            delete item;
        }
    }

    setGameState(STATE_PLAYING);
}

void GameWindow::restartGame()
{
    startGame();
}

void GameWindow::updateDifficulty(int index)
{
    m_difficulty = static_cast<Difficulty>(m_difficultyCombo->itemData(index).toInt());
}

void GameWindow::updateUI()
{
    if (m_gameState != STATE_PLAYING || !m_player) return;

    m_scoreLabel->setText(QString("分数：%1").arg(m_score));
    m_hpLabel->setText(QString("生命值：%1").arg(m_player->getHp()));
    m_energyBar->setValue(m_player->getEnergy());
}

void GameWindow::updateGame()
{
    if (!m_player || m_gameState != STATE_PLAYING) return;

    m_player->updatePlayer(height() - 100);

    m_gameSpeed += 0.001;
    m_score += static_cast<int>(m_gameSpeed * (m_difficulty + 1));

    QList<QGraphicsItem*> items = m_scene->items();
    for (auto item : items) {
        Obstacle *obstacle = dynamic_cast<Obstacle*>(item);
        if (obstacle) {
            obstacle->updateObstacle(m_gameSpeed * m_player->getSpeedMultiplier());

            if (obstacle->x() + obstacle->boundingRect().width() < 0) {
                m_scene->removeItem(obstacle);
                delete obstacle;
            }

            if (m_player->collidesWithItem(obstacle) && !m_player->isInvincible()) {
                if (obstacle->getType() == OBSTACLE_ROCK) {
                    m_player->takeDamage(1);
                    m_player->fall();
                } else if (obstacle->getType() == OBSTACLE_SNOWDRIFT) {
                    m_player->slowDown();
                }

                if (m_player->getHp() <= 0) {
                    gameOver();
                    return;
                }
            }
        }
    }

    updateUI();
    m_scene->update();
}

void GameWindow::spawnObstacle()
{
    if (m_gameState != STATE_PLAYING || !m_player) return;

    int randType = QRandomGenerator::global()->bounded(2);
    ObstacleType type = static_cast<ObstacleType>(randType);

    Obstacle *obstacle = new Obstacle(type);
    obstacle->setPos(width(), height() - 100 - obstacle->boundingRect().height());
    m_scene->addItem(obstacle);
}

void GameWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->isAutoRepeat()) return;

    switch (event->key()) {
    case Qt::Key_Space:
        if (m_gameState == STATE_PLAYING && m_player) {
            m_player->jump();
        }
        break;
    case Qt::Key_W:
        if (m_gameState == STATE_PLAYING && m_player) {
            m_player->frontFlip();
        }
        break;
    case Qt::Key_S:
        if (m_gameState == STATE_PLAYING && m_player) {
            m_player->backFlip();
        }
        break;
    case Qt::Key_Shift:
        if (m_gameState == STATE_PLAYING && m_player) {
            m_player->startAccelerate();
        }
        break;
    case Qt::Key_P:
        if (m_gameState == STATE_PLAYING) {
            pauseGame();
        }
        break;
    case Qt::Key_R:
        restartGame();
        break;
    default:
        QGraphicsView::keyPressEvent(event);
        break;
    }
}

void GameWindow::keyReleaseEvent(QKeyEvent *event)
{
    if (event->isAutoRepeat()) return;

    if (event->key() == Qt::Key_Shift && m_player) {
        m_player->stopAccelerate();
    } else {
        QGraphicsView::keyReleaseEvent(event);
    }
}

void GameWindow::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);
    m_scene->setSceneRect(0, 0, width(), height());
}