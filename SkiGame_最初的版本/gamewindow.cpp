#include "gamewindow.h"
#include <QApplication>
#include <QPainterPath>
#include <QDebug>
#include <cmath>

GameWindow::GameWindow(QWidget *parent)
    : QWidget(parent),
      GAME_WIDTH(800),
      GAME_HEIGHT(600),
      PLAYER_WIDTH(60),
      PLAYER_HEIGHT(80)
{
    // 设置窗口属性
    setWindowTitle("滑雪大冒险");
    setFixedSize(GAME_WIDTH, GAME_HEIGHT);
    setFocusPolicy(Qt::StrongFocus);

    // 初始化游戏状态
    m_gameState = Menu;
    m_difficulty = Normal;
    m_scene = Snow;
    m_weather = Sunny;
    m_screenEffect = None;
    m_isGameRunning = false;
    m_score = 0;
    m_level = 1;
    m_highScore = 0;
    m_combo = 0;
    m_scoreToNextLevel = 200;
    m_difficultyMultiplier = 1.0f;
    m_weatherChangeInterval = 60000; // 60秒
    m_weatherChangeCounter = 0;
    m_screenShakeCounter = 0;
    m_screenShakeIntensity = 0.0f;
    m_effectOpacity = 0.0f;
    m_effectDuration = 0;

    // 初始化游戏元素
    m_player = new Player(GAME_WIDTH / 2 - PLAYER_WIDTH / 2, GAME_HEIGHT - PLAYER_HEIGHT - 20, PLAYER_WIDTH, PLAYER_HEIGHT, this);
    m_monster = nullptr;

    // 初始化定时器
    m_gameTimer = new QTimer(this);
    connect(m_gameTimer, &QTimer::timeout, this, &GameWindow::updateGame);

    // 连接信号和槽
    connect(m_player, &Player::hpChanged, this, &GameWindow::onPlayerHPChanged);
    connect(m_player, &Player::stateChanged, this, &GameWindow::onPlayerStateChanged);

    // 加载资源
    loadResources();

    // 加载最高分数
    loadHighScore();

    // 初始化视差背景和粒子系统
    initParallaxBackground();
    initParticleSystem();

    // 显示主菜单
    update();
}

GameWindow::~GameWindow()
{
    delete m_player;
    qDeleteAll(m_obstacles);
    qDeleteAll(m_powerUps);
    delete m_monster;
    delete m_gameTimer;
    qDeleteAll(m_particles);

    delete m_moveSound;
    delete m_collisionSound;
    delete m_powerUpSound;
    delete m_gameOverSound;
    delete m_hurtSound;
    delete m_shieldSound;
    delete m_coinSound;
    delete m_backgroundMusic;
}

void GameWindow::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 绘制视差背景
    drawParallaxBackground(&painter);

    // 根据游戏状态绘制不同内容
    switch (m_gameState) {
    case Menu:
        drawMenu(&painter);
        break;
    case DifficultySelection:
        drawDifficultySelection(&painter);
        break;
    case SceneSelection:
        drawSceneSelection(&painter);
        break;
    case Playing:
        // 绘制游戏元素
        for (Obstacle *obstacle : m_obstacles) {
            obstacle->draw(&painter);
            obstacle->drawEffects(&painter);
        }
        for (PowerUp *powerUp : m_powerUps) {
            powerUp->draw(&painter);
        }
        if (m_monster) {
            m_monster->draw(&painter);
            m_monster->drawEffects(&painter);
        }
        m_player->draw(&painter, m_playerImage);
        m_player->drawEffects(&painter);

        // 绘制粒子效果
        drawParticles(&painter);

        // 绘制HUD
        drawHUD(&painter);

        // 绘制屏幕特效
        drawScreenEffects(&painter);
        break;
    case GameOver:
        drawGameOver(&painter);
        break;
    }
}

void GameWindow::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    // 处理窗口大小变化
}

void GameWindow::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Left:
    case Qt::Key_A:
        m_leftPressed = true;
        m_player->setAnimationState(Player::Moving);
        break;
    case Qt::Key_Right:
    case Qt::Key_D:
        m_rightPressed = true;
        m_player->setAnimationState(Player::Moving);
        break;
    case Qt::Key_Space:
        m_spacePressed = true;
        if (m_gameState == Playing) {
            // 暂停/继续游戏
            m_isGameRunning = !m_isGameRunning;
        }
        break;
    case Qt::Key_Escape:
        if (m_gameState == Playing) {
            m_gameState = Menu;
            m_isGameRunning = false;
            m_gameTimer->stop();
        }
        break;
    }
}

void GameWindow::keyReleaseEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Left:
    case Qt::Key_A:
        m_leftPressed = false;
        if (!m_rightPressed) {
            m_player->setAnimationState(Player::Idle);
        }
        break;
    case Qt::Key_Right:
    case Qt::Key_D:
        m_rightPressed = false;
        if (!m_leftPressed) {
            m_player->setAnimationState(Player::Idle);
        }
        break;
    case Qt::Key_Space:
        m_spacePressed = false;
        break;
    }
}

void GameWindow::mousePressEvent(QMouseEvent *event)
{
    m_mousePressed = true;

    if (m_gameState == Menu) {
        if (isButtonClicked(m_startGameButton, event->pos())) {
            m_startButtonPressed = true;
            m_gameState = DifficultySelection;
            update();
        } else if (isButtonClicked(m_exitButton, event->pos())) {
            m_exitButtonPressed = true;
            QApplication::quit();
        }
    } else if (m_gameState == DifficultySelection) {
        if (isButtonClicked(m_easyButton, event->pos())) {
            m_difficulty = Easy;
            m_gameState = SceneSelection;
            update();
        } else if (isButtonClicked(m_normalButton, event->pos())) {
            m_difficulty = Normal;
            m_gameState = SceneSelection;
            update();
        } else if (isButtonClicked(m_hardButton, event->pos())) {
            m_difficulty = Hard;
            m_gameState = SceneSelection;
            update();
        }
    } else if (m_gameState == SceneSelection) {
        if (isButtonClicked(m_snowButton, event->pos())) {
            m_scene = Snow;
            startNewGame();
        } else if (isButtonClicked(m_forestButton, event->pos())) {
            m_scene = Forest;
            startNewGame();
        } else if (isButtonClicked(m_nightButton, event->pos())) {
            m_scene = Night;
            startNewGame();
        }
    } else if (m_gameState == GameOver) {
        if (isButtonClicked(m_restartButton, event->pos())) {
            m_restartButtonPressed = true;
            startNewGame();
        } else if (isButtonClicked(m_menuButton, event->pos())) {
            m_menuButtonPressed = true;
            m_gameState = Menu;
            update();
        }
    }
}

void GameWindow::mouseReleaseEvent(QMouseEvent *event)
{
    m_mousePressed = false;
    m_startButtonPressed = false;
    m_exitButtonPressed = false;
    m_restartButtonPressed = false;
    m_menuButtonPressed = false;
    update();
}

void GameWindow::mouseMoveEvent(QMouseEvent *event)
{
    m_mousePos = event->pos();

    if (m_gameState == Menu) {
        m_startButtonHovered = isButtonHovered(m_startGameButton, m_mousePos);
        m_exitButtonHovered = isButtonHovered(m_exitButton, m_mousePos);
    } else if (m_gameState == GameOver) {
        m_restartButtonHovered = isButtonHovered(m_restartButton, m_mousePos);
        m_menuButtonHovered = isButtonHovered(m_menuButton, m_mousePos);
    }

    update();
}

void GameWindow::updateGame()
{
    if (!m_isGameRunning) {
        return;
    }

    // 更新视差背景
    updateParallaxBackground();

    // 更新天气
    updateWeather();

    // 更新粒子系统
    updateParticles();

    // 处理玩家输入
    if (m_leftPressed) {
        m_player->moveLeft(m_playerSpeed, true);
    }
    if (m_rightPressed) {
        m_player->moveRight(m_playerSpeed, GAME_WIDTH, true);
    }

    // 更新玩家状态和动画
    m_player->updateState();
    m_player->updateAnimation();

    // 更新障碍物
    for (int i = m_obstacles.size() - 1; i >= 0; --i) {
        Obstacle *obstacle = m_obstacles[i];
        obstacle->update();
        if (obstacle->getType() == Obstacle::Snowball && obstacle->getState() == Obstacle::Rolling) {
            obstacle->move();
        } else {
            obstacle->moveDown();
        }
        if (obstacle->isOutOfBounds(GAME_HEIGHT) || obstacle->getState() == Obstacle::Breaking) {
            m_obstacles.removeAt(i);
            delete obstacle;
        }
    }

    // 更新道具
    for (int i = m_powerUps.size() - 1; i >= 0; --i) {
        PowerUp *powerUp = m_powerUps[i];
        powerUp->moveDown();
        if (powerUp->isOutOfBounds(GAME_HEIGHT)) {
            m_powerUps.removeAt(i);
            delete powerUp;
        }
    }

    // 更新怪物
    if (m_monster) {
        m_monster->update(m_player);
        m_monster->move(m_player, m_difficulty);
        if (m_monster->isOutOfBounds(-m_monster->getHeight())) {
            delete m_monster;
            m_monster = nullptr;
        }
    }

    // 生成新障碍物
    m_obstacleGenerationCounter++;
    if (m_obstacleGenerationCounter >= m_obstacleGenerationInterval) {
        generateObstacle();
        m_obstacleGenerationCounter = 0;
    }

    // 生成新道具
    m_powerUpGenerationCounter++;
    if (m_powerUpGenerationCounter >= m_powerUpGenerationInterval) {
        generatePowerUp();
        m_powerUpGenerationCounter = 0;
    }

    // 生成雪花粒子
    generateSnowflakes();

    // 检查碰撞
    checkCollisions();

    // 更新分数
    updateScore();

    // 更新游戏难度
    updateGameDifficulty();

    // 更新屏幕特效
    if (m_screenEffect != None && m_effectDuration > 0) {
        m_effectDuration--;
        if (m_effectDuration == 0) {
            m_screenEffect = None;
        }
    }

    // 更新屏幕抖动
    if (m_screenShakeCounter > 0) {
        m_screenShakeCounter--;
        m_screenShakeIntensity *= 0.9f;
    }

    // 重绘
    update();
}

void GameWindow::startNewGame()
{
    // 清理游戏元素
    qDeleteAll(m_obstacles);
    m_obstacles.clear();
    qDeleteAll(m_powerUps);
    m_powerUps.clear();
    delete m_monster;
    m_monster = nullptr;
    qDeleteAll(m_particles);
    m_particles.clear();

    // 重置游戏状态
    m_score = 0;
    m_level = 1;
    m_combo = 0;
    m_scoreToNextLevel = 200;
    m_difficultyMultiplier = 1.0f;

    // 重置玩家
    delete m_player;
    m_player = new Player(GAME_WIDTH / 2 - PLAYER_WIDTH / 2, GAME_HEIGHT - PLAYER_HEIGHT - 20, PLAYER_WIDTH, PLAYER_HEIGHT, this);
    connect(m_player, &Player::hpChanged, this, &GameWindow::onPlayerHPChanged);
    connect(m_player, &Player::stateChanged, this, &GameWindow::onPlayerStateChanged);

    // 设置游戏参数
    switch (m_difficulty) {
    case Easy:
        m_playerSpeed = 6;
        m_baseObstacleSpeed = 3;
        m_obstacleGenerationInterval = 80;
        m_powerUpGenerationInterval = 200;
        m_monsterSpeed = 2;
        break;
    case Normal:
        m_playerSpeed = 8;
        m_baseObstacleSpeed = 5;
        m_obstacleGenerationInterval = 60;
        m_powerUpGenerationInterval = 300;
        m_monsterSpeed = 3;
        break;
    case Hard:
        m_playerSpeed = 10;
        m_baseObstacleSpeed = 7;
        m_obstacleGenerationInterval = 40;
        m_powerUpGenerationInterval = 400;
        m_monsterSpeed = 4;
        break;
    }

    // 开始游戏
    m_gameState = Playing;
    m_isGameRunning = true;
    m_gameTimer->start(16); // 60 FPS

    // 播放背景音乐
    updateBackgroundMusic();
}

void GameWindow::gameOver()
{
    m_isGameRunning = false;
    m_gameTimer->stop();
    m_gameState = GameOver;
    playSound("gameOver");
    saveHighScore();
    update();
}

void GameWindow::onPlayerHPChanged(int currentHP, int maxHP)
{
    if (currentHP <= 0) {
        gameOver();
    }
}

void GameWindow::onPlayerStateChanged(Player::PlayerState newState)
{
    // 处理玩家状态变化
    switch (newState) {
    case Player::SpeedUp:
        setScreenEffect(SpeedBoost, 120);
        break;
    case Player::Invincible:
        // 无敌状态特效
        break;
    case Player::Shield:
        // 护盾状态特效
        break;
    case Player::Magnet:
        // 磁铁状态特效
        break;
    default:
        break;
    }
}

void GameWindow::setScreenEffect(ScreenEffect effect, int duration)
{
    m_screenEffect = effect;
    m_effectDuration = duration;
    if (effect == FadeIn) {
        m_effectOpacity = 0.0f;
    } else if (effect == FadeOut) {
        m_effectOpacity = 1.0f;
    }
}

void GameWindow::playSound(const QString &soundName)
{
    if (soundName == "move") {
        m_moveSound->play();
    } else if (soundName == "collision") {
        m_collisionSound->play();
    } else if (soundName == "powerup") {
        m_powerUpSound->play();
    } else if (soundName == "gameOver") {
        m_gameOverSound->play();
    } else if (soundName == "hurt") {
        m_hurtSound->play();
    } else if (soundName == "shield") {
        m_shieldSound->play();
    } else if (soundName == "coin") {
        m_coinSound->play();
    }
}

void GameWindow::updateBackgroundMusic()
{
    QString musicPath;
    switch (m_scene) {
    case Snow:
        musicPath = ":/sounds/bgm_snow.wav";
        break;
    case Forest:
        musicPath = ":/sounds/bgm_forest.wav";
        break;
    case Night:
        musicPath = ":/sounds/bgm_night.wav";
        break;
    }
    m_backgroundMusic->setMedia(QUrl::fromLocalFile(musicPath));
    m_backgroundMusic->play();
}

void GameWindow::initGame()
{
    // 初始化游戏元素
    m_player = new Player(GAME_WIDTH / 2 - PLAYER_WIDTH / 2, GAME_HEIGHT - PLAYER_HEIGHT - 20, PLAYER_WIDTH, PLAYER_HEIGHT, this);
    m_monster = nullptr;
    m_obstacles.clear();
    m_powerUps.clear();
    m_particles.clear();

    // 初始化游戏参数
    m_obstacleGenerationCounter = 0;
    m_powerUpGenerationCounter = 0;
    m_particleGenerationCounter = 0;
}

void GameWindow::initParallaxBackground()
{
    ParallaxLayer layer;

    // 远景雪山
    layer.image = QImage(":/images/parallax_mountain.png");
    layer.scrollSpeed = 0.1f;
    layer.xOffset = 0.0f;
    layer.yOffset = 0.0f;
    m_parallaxLayers.append(layer);

    // 中景树林
    layer.image = QImage(":/images/parallax_trees.png");
    layer.scrollSpeed = 0.3f;
    layer.xOffset = 0.0f;
    layer.yOffset = 0.0f;
    m_parallaxLayers.append(layer);

    // 近景雪地
    layer.image = QImage(":/images/parallax_snow.png");
    layer.scrollSpeed = 0.5f;
    layer.xOffset = 0.0f;
    layer.yOffset = 0.0f;
    m_parallaxLayers.append(layer);
}

void GameWindow::initParticleSystem()
{
    m_particles.clear();
}

void GameWindow::loadResources()
{
    // 加载图像资源
    m_playerImage = QImage(":/images/player.png");
    m_monsterImage = QImage(":/images/monster.png");
    m_treeImage = QImage(":/images/tree.png");
    m_rockImage = QImage(":/images/rock.png");
    m_pitImage = QImage(":/images/pit.png");
    m_iceImage = QImage(":/images/ice.png");
    m_speedBoostImage = QImage(":/images/speed_boost.png");
    m_healthImage = QImage(":/images/health.png");
    m_invincibilityImage = QImage(":/images/invincibility.png");
    m_bombImage = QImage(":/images/bomb.png");
    m_shieldImage = QImage(":/images/shield.png");
    m_magnetImage = QImage(":/images/magnet.png");
    m_coinImage = QImage(":/images/coin.png");

    // 加载UI资源
    m_hpBarBackground = QImage(":/images/hp_bar_bg.png");
    m_hpBarFill = QImage(":/images/hp_bar_fill.png");
    m_hpBarBorder = QImage(":/images/hp_bar_border.png");
    m_menuBackground = QImage(":/images/menu_bg.png");

    // 加载音效
    m_moveSound = new QSoundEffect(this);
    m_moveSound->setSource(QUrl::fromLocalFile(":/sounds/move.wav"));
    m_moveSound->setVolume(0.5f);

    m_collisionSound = new QSoundEffect(this);
    m_collisionSound->setSource(QUrl::fromLocalFile(":/sounds/collision_sound.wav"));
    m_collisionSound->setVolume(0.7f);

    m_powerUpSound = new QSoundEffect(this);
    m_powerUpSound->setSource(QUrl::fromLocalFile(":/sounds/powerup_sound.wav"));
    m_powerUpSound->setVolume(0.7f);

    m_gameOverSound = new QSoundEffect(this);
    m_gameOverSound->setSource(QUrl::fromLocalFile(":/sounds/game_over_sound.wav"));
    m_gameOverSound->setVolume(0.7f);

    m_hurtSound = new QSoundEffect(this);
    m_hurtSound->setSource(QUrl::fromLocalFile(":/sounds/hurt.wav"));
    m_hurtSound->setVolume(0.7f);

    m_shieldSound = new QSoundEffect(this);
    m_shieldSound->setSource(QUrl::fromLocalFile(":/sounds/shield.wav"));
    m_shieldSound->setVolume(0.7f);

    m_coinSound = new QSoundEffect(this);
    m_coinSound->setSource(QUrl::fromLocalFile(":/sounds/coin.wav"));
    m_coinSound->setVolume(0.5f);

    // 加载背景音乐
    m_backgroundMusic = new QMediaPlayer(this);
    m_backgroundMusic->setVolume(0.5f);
    connect(m_backgroundMusic, &QMediaPlayer::mediaStatusChanged, this, &GameWindow::updateBackgroundMusic);

    // 加载场景资源
    loadSceneResources();
}

void GameWindow::loadSceneResources()
{
    // 根据场景加载特定资源
}

void GameWindow::generateObstacle()
{
    int rand = QRandomGenerator::global()->bounded(100);
    Obstacle::ObstacleType type;
    QImage image;

    if (rand < 30) {
        type = Obstacle::Tree;
        image = m_treeImage;
    } else if (rand < 50) {
        type = Obstacle::Rock;
        image = m_rockImage;
    } else if (rand < 70) {
        type = Obstacle::Pit;
        image = m_pitImage;
    } else if (rand < 90) {
        type = Obstacle::Ice;
        image = m_iceImage;
    } else {
        type = Obstacle::Snowball;
        image = m_rockImage; // 使用岩石图像作为占位符
    }

    int x = QRandomGenerator::global()->bounded(GAME_WIDTH - 40);
    int speed = m_baseObstacleSpeed + QRandomGenerator::global()->bounded(3);
    Obstacle *obstacle = new Obstacle(x, -40, 40, 40, speed, type, Qt::red, image, this);
    m_obstacles.append(obstacle);
}

void GameWindow::generatePowerUp()
{
    int rand = QRandomGenerator::global()->bounded(100);
    PowerUp::PowerUpType type;
    QImage image;

    if (rand < 20) {
        type = PowerUp::Health;
        image = m_healthImage;
    } else if (rand < 40) {
        type = PowerUp::SpeedBoost;
        image = m_speedBoostImage;
    } else if (rand < 50) {
        type = PowerUp::Invincibility;
        image = m_invincibilityImage;
    } else if (rand < 60) {
        type = PowerUp::Bomb;
        image = m_bombImage;
    } else if (rand < 70) {
        type = PowerUp::Shield;
        image = m_shieldImage;
    } else if (rand < 80) {
        type = PowerUp::Magnet;
        image = m_magnetImage;
    } else {
        type = PowerUp::Coin;
        image = m_coinImage;
    }

    int x = QRandomGenerator::global()->bounded(GAME_WIDTH - 30);
    PowerUp *powerUp = new PowerUp(x, -30, 30, 30, m_baseObstacleSpeed, type, Qt::yellow, image, this);
    m_powerUps.append(powerUp);
}

void GameWindow::checkCollisions()
{
    // 玩家与障碍物碰撞
    for (int i = m_obstacles.size() - 1; i >= 0; --i) {
        Obstacle *obstacle = m_obstacles[i];
        if (m_player->getRect().intersects(obstacle->getRect())) {
            if (m_player->hasState(Player::Invincible) || m_player->hasState(Player::Shield)) {
                // 无敌或护盾状态，摧毁障碍物
                if (obstacle->isDestructible()) {
                    obstacle->takeDamage(100);
                    generateEffectParticles(obstacle->getRect().center().x(), obstacle->getRect().center().y(), "explosion");
                }
                if (m_player->hasState(Player::Shield)) {
                    m_player->removeState(Player::Shield);
                    playSound("shield");
                }
            } else {
                // 普通状态，玩家受伤
                handlePlayerHurt(obstacle->getDamage());
                playSound("collision");
                generateEffectParticles(obstacle->getRect().center().x(), obstacle->getRect().center().y(), "collision");

                // 应用特殊效果
                if (obstacle->getSlowdownFactor() > 0) {
                    // 减速效果
                    m_player->setSpeed(m_player->getSpeed() * (1 - obstacle->getSlowdownFactor()));
                }
                if (obstacle->getSpeedupFactor() > 0) {
                    // 加速效果
                    m_player->addState(Player::SpeedUp, 120); // 2秒加速
                }
            }
        }
    }

    // 玩家与道具碰撞
    for (int i = m_powerUps.size() - 1; i >= 0; --i) {
        PowerUp *powerUp = m_powerUps[i];
        if (m_player->getRect().intersects(powerUp->getRect())) {
            handlePowerUpPickup(powerUp);
            m_powerUps.removeAt(i);
            delete powerUp;
        }
    }

    // 玩家与怪物碰撞
    if (m_monster && m_player->getRect().intersects(m_monster->getRect())) {
        if (!m_player->hasState(Player::Invincible) && !m_player->hasState(Player::Shield)) {
            handlePlayerHurt(m_monster->getDamage());
            playSound("collision");
            generateEffectParticles(m_monster->getRect().center().x(), m_monster->getRect().center().y(), "collision");
        } else {
            if (m_player->hasState(Player::Shield)) {
                m_player->removeState(Player::Shield);
                playSound("shield");
            }
            // 怪物被击退
            m_monster->setY(m_monster->getY() - 100);
        }
    }
}

void GameWindow::handlePlayerHurt(int damage)
{
    m_player->decreaseHP(damage);
    m_combo = 0; // 重置combo
    setScreenEffect(HurtFlash, 10); // 10帧闪烁
    m_screenShakeCounter = 15;
    m_screenShakeIntensity = 5.0f;
    playSound("hurt");
}

void GameWindow::handlePowerUpPickup(PowerUp *powerUp)
{
    applyPowerUp(powerUp);
    generateEffectParticles(powerUp->getRect().center().x(), powerUp->getRect().center().y(), "powerup");
    if (powerUp->getType() == PowerUp::Coin) {
        playSound("coin");
    } else {
        playSound("powerup");
    }
}

void GameWindow::applyPowerUp(PowerUp *powerUp)
{
    switch (powerUp->getType()) {
    case PowerUp::Health:
        m_player->increaseHP(20);
        break;
    case PowerUp::SpeedBoost:
        m_player->addState(Player::SpeedUp, 120); // 2秒
        m_isSpeedBoostActive = true;
        m_powerUpDurations[PowerUp::SpeedBoost] = 120;
        setScreenEffect(SpeedBoost, 120);
        break;
    case PowerUp::Invincibility:
        m_player->addState(Player::Invincible, 180); // 3秒
        break;
    case PowerUp::Bomb:
        // 清除所有障碍物
        for (Obstacle *obstacle : m_obstacles) {
            generateEffectParticles(obstacle->getRect().center().x(), obstacle->getRect().center().y(), "explosion");
            delete obstacle;
        }
        m_obstacles.clear();
        break;
    case PowerUp::Shield:
        m_player->addState(Player::Shield, 240); // 4秒
        m_isShieldActive = true;
        m_powerUpDurations[PowerUp::Shield] = 240;
        break;
    case PowerUp::Magnet:
        m_player->addState(Player::Magnet, 300); // 5秒
        m_isMagnetActive = true;
        m_powerUpDurations[PowerUp::Magnet] = 300;
        break;
    case PowerUp::Coin:
        m_score += 50;
        break;
    }
}

void GameWindow::updateScore()
{
    // 随时间增加分数
    m_score++;
    m_combo++;

    // 每200分升一级
    if (m_score >= m_scoreToNextLevel) {
        m_level++;
        m_scoreToNextLevel += 200 * m_level;
        // 增加游戏难度
        updateGameDifficulty();
    }
}

void GameWindow::updateGameDifficulty()
{
    m_difficultyMultiplier = 1.0f + (m_level - 1) * 0.1f;
    m_baseObstacleSpeed = static_cast<int>(5 * m_difficultyMultiplier);
    m_obstacleGenerationInterval = qMax(10, static_cast<int>(30 / m_difficultyMultiplier));
    m_monsterSpeed = static_cast<int>(3 * m_difficultyMultiplier);

    // 根据难度生成怪物
    if (m_level % 3 == 0 && !m_monster) {
        m_monster = new Monster(
            QRandomGenerator::global()->bounded(GAME_WIDTH - 80),
            -100,
            80,
            80,
            m_monsterSpeed,
            m_monsterImage,
            this
        );
        if (m_difficulty == Hard) {
            m_monster->setAIMode(Monster::Predictive);
        }
    }
}

void GameWindow::updateWeather()
{
    m_weatherChangeCounter++;
    if (m_weatherChangeCounter >= m_weatherChangeInterval / 16) { // 假设16ms一帧
        m_weatherChangeCounter = 0;
        // 随机切换天气
        int rand = QRandomGenerator::global()->bounded(3);
        m_weather = static_cast<WeatherType>(rand);
    }
}

void GameWindow::updateParallaxBackground()
{
    for (ParallaxLayer &layer : m_parallaxLayers) {
        layer.yOffset += layer.scrollSpeed * m_baseObstacleSpeed * 0.1f;
        if (layer.yOffset > layer.image.height()) {
            layer.yOffset = 0.0f;
        }
    }
}

void GameWindow::updateParticles()
{
    for (int i = m_particles.size() - 1; i >= 0; --i) {
        Particle *particle = static_cast<Particle*>(m_particles[i]);
        particle->position += particle->velocity;
        particle->life--;
        particle->opacity = particle->life / particle->maxLife;

        if (particle->life <= 0) {
            m_particles.removeAt(i);
            delete particle;
        }
    }
}

void GameWindow::generateSnowflakes()
{
    if (m_weather == Sunny) {
        return;
    }

    int particleCount = (m_weather == LightSnow) ? 1 : 3;
    for (int i = 0; i < particleCount; ++i) {
        Particle *particle = new Particle();
        particle->position.setX(QRandomGenerator::global()->bounded(GAME_WIDTH));
        particle->position.setY(-10);
        particle->velocity.setX(QRandomGenerator::global()->bounded(200) / 100.0f - 1.0f);
        particle->velocity.setY(QRandomGenerator::global()->bounded(200) / 100.0f + 1.0f);
        particle->size = QSize(QRandomGenerator::global()->bounded(4) + 2, QRandomGenerator::global()->bounded(4) + 2);
        particle->color = Qt::white;
        particle->opacity = 1.0f;
        particle->rotation = 0.0f;
        particle->life = QRandomGenerator::global()->bounded(120) + 60;
        particle->maxLife = particle->life;
        particle->type = "snow";
        m_particles.append(particle);
    }
}

void GameWindow::generateEffectParticles(int x, int y, const QString &type)
{
    int particleCount = 10;
    for (int i = 0; i < particleCount; ++i) {
        Particle *particle = new Particle();
        particle->position.setX(x);
        particle->position.setY(y);

        float angle = (2 * M_PI * i) / particleCount;
        float speed = QRandomGenerator::global()->bounded(500) / 100.0f + 2.0f;
        particle->velocity.setX(std::cos(angle) * speed);
        particle->velocity.setY(std::sin(angle) * speed);

        if (type == "explosion") {
            particle->size = QSize(QRandomGenerator::global()->bounded(8) + 4, QRandomGenerator::global()->bounded(8) + 4);
            particle->color = QColor(QRandomGenerator::global()->bounded(255), QRandomGenerator::global()->bounded(100), 0);
            particle->life = QRandomGenerator::global()->bounded(60) + 30;
        } else if (type == "collision") {
            particle->size = QSize(QRandomGenerator::global()->bounded(6) + 2, QRandomGenerator::global()->bounded(6) + 2);
            particle->color = Qt::red;
            particle->life = QRandomGenerator::global()->bounded(40) + 20;
        } else if (type == "powerup") {
            particle->size = QSize(QRandomGenerator::global()->bounded(8) + 4, QRandomGenerator::global()->bounded(8) + 4);
            particle->color = QColor(0, QRandomGenerator::global()->bounded(255), QRandomGenerator::global()->bounded(255));
            particle->life = QRandomGenerator::global()->bounded(80) + 40;
        }

        particle->maxLife = particle->life;
        particle->opacity = 1.0f;
        particle->rotation = 0.0f;
        particle->type = type;
        m_particles.append(particle);
    }
}

void GameWindow::drawMenu(QPainter *painter)
{
    // 绘制动态背景（视差滚动+粒子）
    drawParallaxBackground(painter);
    drawParticles(painter);

    // 绘制半透明遮罩
    painter->fillRect(rect(), QColor(0, 0, 0, 100));

    // 绘制标题（带阴影和发光效果）
    painter->setFont(QFont("Arial", 48, QFont::Bold));
    QPainterPath titlePath;
    titlePath.addText(GAME_WIDTH / 2 - 150, 150, QFont("Arial", 48, QFont::Bold), "滑雪大冒险");
    painter->setPen(QPen(QColor(255, 255, 255), 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter->drawPath(titlePath);
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(0, 100, 255));
    painter->drawPath(titlePath);

    // 绘制按钮
    int buttonWidth = 200;
    int buttonHeight = 50;
    int buttonY = GAME_HEIGHT / 2 - buttonHeight;

    m_startGameButton = QRect(GAME_WIDTH / 2 - buttonWidth / 2, buttonY, buttonWidth, buttonHeight);
    drawButton(painter, m_startGameButton, "开始游戏", m_startButtonHovered, m_startButtonPressed);

    buttonY += buttonHeight + 20;
    m_exitButton = QRect(GAME_WIDTH / 2 - buttonWidth / 2, buttonY, buttonWidth, buttonHeight);
    drawButton(painter, m_exitButton, "退出游戏", m_exitButtonHovered, m_exitButtonPressed);

    // 绘制版本信息
    painter->setFont(QFont("Arial", 12));
    painter->setPen(Qt::white);
    painter->drawText(rect(), Qt::AlignBottom | Qt::AlignRight, "v2.0");
}

void GameWindow::drawDifficultySelection(QPainter *painter)
{
    drawParallaxBackground(painter);
    painter->fillRect(rect(), QColor(0, 0, 0, 150));

    painter->setFont(QFont("Arial", 36, QFont::Bold));
    painter->setPen(Qt::white);
    painter->drawText(rect(), Qt::AlignTop | Qt::AlignHCenter, "选择难度");

    int buttonWidth = 200;
    int buttonHeight = 50;
    int buttonY = 200;

    m_easyButton = QRect(GAME_WIDTH / 2 - buttonWidth / 2, buttonY, buttonWidth, buttonHeight);
    drawButton(painter, m_easyButton, "简单", false, false);

    buttonY += buttonHeight + 20;
    m_normalButton = QRect(GAME_WIDTH / 2 - buttonWidth / 2, buttonY, buttonWidth, buttonHeight);
    drawButton(painter, m_normalButton, "普通", false, false);

    buttonY += buttonHeight + 20;
    m_hardButton = QRect(GAME_WIDTH / 2 - buttonWidth / 2, buttonY, buttonWidth, buttonHeight);
    drawButton(painter, m_hardButton, "困难", false, false);
}

void GameWindow::drawSceneSelection(QPainter *painter)
{
    drawParallaxBackground(painter);
    painter->fillRect(rect(), QColor(0, 0, 0, 150));

    painter->setFont(QFont("Arial", 36, QFont::Bold));
    painter->setPen(Qt::white);
    painter->drawText(rect(), Qt::AlignTop | Qt::AlignHCenter, "选择场景");

    int buttonWidth = 200;
    int buttonHeight = 50;
    int buttonY = 200;

    m_snowButton = QRect(GAME_WIDTH / 2 - buttonWidth / 2, buttonY, buttonWidth, buttonHeight);
    drawButton(painter, m_snowButton, "雪地", false, false);

    buttonY += buttonHeight + 20;
    m_forestButton = QRect(GAME_WIDTH / 2 - buttonWidth / 2, buttonY, buttonWidth, buttonHeight);
    drawButton(painter, m_forestButton, "森林", false, false);

    buttonY += buttonHeight + 20;
    m_nightButton = QRect(GAME_WIDTH / 2 - buttonWidth / 2, buttonY, buttonWidth, buttonHeight);
    drawButton(painter, m_nightButton, "夜晚", false, false);
}

void GameWindow::drawGameOver(QPainter *painter)
{
    // 绘制游戏结束时的场景
    drawParallaxBackground(painter);

    // 绘制半透明黑色背景（带渐变）
    QLinearGradient gradient(0, 0, 0, GAME_HEIGHT);
    gradient.setColorAt(0, QColor(0, 0, 0, 200));
    gradient.setColorAt(1, QColor(0, 0, 0, 255));
    painter->fillRect(rect(), gradient);

    // 绘制游戏结束文字（滑入动画）
    painter->setFont(QFont("Arial", 36, QFont::Bold));
    painter->setPen(Qt::white);
    painter->drawText(rect(), Qt::AlignTop | Qt::AlignHCenter, "游戏结束");

    // 绘制分数统计
    QRect statsRect(GAME_WIDTH / 2 - 200, 150, 400, 200);
    drawStatisticsChart(painter, statsRect);

    // 绘制最终得分
    painter->setFont(QFont("Arial", 24));
    painter->drawText(rect(), Qt::AlignCenter, QString("最终得分: %1").arg(m_score));

    if (m_score > m_highScore) {
        painter->setFont(QFont("Arial", 20, QFont::Bold));
        painter->setPen(QColor(255, 215, 0));
        painter->drawText(rect(), Qt::AlignCenter | Qt::AlignTop, "新纪录！");
    }

    // 绘制按钮
    int buttonWidth = 200;
    int buttonHeight = 50;
    int buttonY = GAME_HEIGHT - 200;

    m_restartButton = QRect(GAME_WIDTH / 2 - buttonWidth - 30, buttonY, buttonWidth, buttonHeight);
    drawButton(painter, m_restartButton, "重新开始", m_restartButtonHovered, m_restartButtonPressed);

    m_menuButton = QRect(GAME_WIDTH / 2 + 30, buttonY, buttonWidth, buttonHeight);
    drawButton(painter, m_menuButton, "返回主菜单", m_menuButtonHovered, m_menuButtonPressed);
}

void GameWindow::drawHUD(QPainter *painter)
{
    drawHPBar(painter);
    drawScoreAndLevel(painter);
    drawStatusIcons(painter);
    drawPowerUpIcons(painter);
}

void GameWindow::drawHPBar(QPainter *painter)
{
    // 绘制血条背景
    painter->setBrush(QColor(50, 50, 50, 200));
    painter->drawRoundedRect(20, 20, 200, 20, 5, 5);

    // 绘制血条填充（渐变）
    QLinearGradient hpGradient(20, 20, 220, 20);
    hpGradient.setColorAt(0, Qt::red);
    hpGradient.setColorAt(1, Qt::darkRed);
    painter->setBrush(hpGradient);
    int hpWidth = 200 * m_player->getHP() / m_player->getMaxHP();
    painter->drawRoundedRect(20, 20, hpWidth, 20, 5, 5);

    // 绘制血条边框
    painter->setPen(QPen(Qt::white, 2));
    painter->drawRoundedRect(20, 20, 200, 20, 5, 5);

    // 绘制血量文字
    painter->setFont(QFont("Arial", 12, QFont::Bold));
    painter->setPen(Qt::white);
    painter->drawText(QRect(20, 20, 200, 20), Qt::AlignCenter, QString("%1/%2").arg(m_player->getHP()).arg(m_player->getMaxHP()));
}

void GameWindow::drawScoreAndLevel(QPainter *painter)
{
    // 绘制分数背景
    painter->setBrush(QColor(0, 0, 0, 150));
    painter->drawRoundedRect(GAME_WIDTH - 150, 20, 130, 60, 10, 10);

    // 绘制分数和等级
    painter->setFont(QFont("Arial", 16, QFont::Bold));
    painter->setPen(Qt::white);
    painter->drawText(QRect(GAME_WIDTH - 150, 20, 130, 30), Qt::AlignCenter, QString("分数: %1").arg(m_score));
    painter->drawText(QRect(GAME_WIDTH - 150, 50, 130, 30), Qt::AlignCenter, QString("等级: %1").arg(m_level));

    // 绘制Combo
    if (m_combo > 10) {
        painter->setFont(QFont("Arial", 14, QFont::Bold));
        painter->setPen(QColor(255, 215, 0));
        painter->drawText(GAME_WIDTH / 2 - 50, 50, QString("Combo: %1").arg(m_combo));
    }
}

void GameWindow::drawStatusIcons(QPainter *painter)
{
    int iconSize = 32;
    int x = 20;
    int y = GAME_HEIGHT - iconSize - 20;

    // 绘制状态图标
    if (m_player->hasState(Player::SpeedUp)) {
        painter->drawImage(x, y, m_speedBoostImage.scaled(iconSize, iconSize));
        x += iconSize + 10;
    }
    if (m_player->hasState(Player::Invincible)) {
        painter->drawImage(x, y, m_invincibilityImage.scaled(iconSize, iconSize));
        x += iconSize + 10;
    }
    if (m_player->hasState(Player::Shield)) {
        painter->drawImage(x, y, m_shieldImage.scaled(iconSize, iconSize));
        x += iconSize + 10;
    }
    if (m_player->hasState(Player::Magnet)) {
        painter->drawImage(x, y, m_magnetImage.scaled(iconSize, iconSize));
    }
}

void GameWindow::drawPowerUpIcons(QPainter *painter)
{
    // 绘制道具冷却图标
}

void GameWindow::drawButton(QPainter *painter, const QRect &rect, const QString &text, bool isHovered, bool isPressed)
{
    // 绘制按钮阴影
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(0, 0, 0, 100));
    painter->drawRoundedRect(rect.x() + 5, rect.y() + 5, rect.width(), rect.height(), 10, 10);

    // 绘制按钮背景（带渐变）
    QLinearGradient gradient(rect.topLeft(), rect.bottomRight());
    if (isPressed) {
        gradient.setColorAt(0, QColor(0, 80, 200));
        gradient.setColorAt(1, QColor(0, 50, 150));
    } else if (isHovered) {
        gradient.setColorAt(0, QColor(0, 150, 255));
        gradient.setColorAt(1, QColor(0, 100, 200));
    } else {
        gradient.setColorAt(0, QColor(0, 120, 255));
        gradient.setColorAt(1, QColor(0, 80, 200));
    }
    painter->setBrush(gradient);
    painter->drawRoundedRect(rect, 10, 10);

    // 绘制按钮边框
    painter->setPen(QPen(Qt::white, 2));
    painter->drawRoundedRect(rect, 10, 10);

    // 绘制按钮文字
    painter->setFont(QFont("Arial", 16, QFont::Bold));
    painter->setPen(Qt::white);
    painter->drawText(rect, Qt::AlignCenter, text);

    // 如果悬停，绘制发光效果
    if (isHovered) {
        painter->setPen(QPen(QColor(255, 255, 255, 100), 5));
        painter->drawRoundedRect(rect, 10, 10);
    }
}

void GameWindow::drawParticles(QPainter *painter)
{
    for (QObject *obj : m_particles) {
        Particle *particle = static_cast<Particle*>(obj);
        painter->save();
        painter->setOpacity(particle->opacity);
        painter->setBrush(particle->color);
        painter->setPen(Qt::NoPen);
        painter->drawEllipse(particle->position, particle->size.width() / 2, particle->size.height() / 2);
        painter->restore();
    }
}

void GameWindow::drawParallaxBackground(QPainter *painter)
{
    for (const ParallaxLayer &layer : m_parallaxLayers) {
        painter->drawImage(0, static_cast<int>(layer.yOffset), layer.image);
        painter->drawImage(0, static_cast<int>(layer.yOffset) - layer.image.height(), layer.image);
    }
}

void GameWindow::drawScreenEffects(QPainter *painter)
{
    switch (m_screenEffect) {
    case HurtFlash:
        painter->fillRect(rect(), QColor(255, 0, 0, 50));
        break;
    case MonsterChase:
        // 绘制暗角
        QRadialGradient vignette(rect().center(), GAME_WIDTH / 2, rect().center());
        vignette.setColorAt(0, QColor(0, 0, 0, 0));
        vignette.setColorAt(1, QColor(0, 0, 0, 100));
        painter->fillRect(rect(), vignette);
        break;
    case SpeedBoost:
        // 绘制速度线
        painter->setPen(QPen(Qt::white, 2, Qt::SolidLine, Qt::RoundCap));
        for (int i = 0; i < GAME_WIDTH; i += 20) {
            painter->drawLine(i, 0, i - 50, GAME_HEIGHT);
        }
        break;
    case FadeIn:
        painter->fillRect(rect(), QColor(0, 0, 0, static_cast<int>(255 * (1.0f - m_effectOpacity))));
        break;
    case FadeOut:
        painter->fillRect(rect(), QColor(0, 0, 0, static_cast<int>(255 * m_effectOpacity)));
        break;
    default:
        break;
    }

    // 绘制屏幕抖动
    if (m_screenShakeCounter > 0) {
        painter->translate(
            QRandomGenerator::global()->bounded(static_cast<int>(m_screenShakeIntensity * 2)) - m_screenShakeIntensity,
            QRandomGenerator::global()->bounded(static_cast<int>(m_screenShakeIntensity * 2)) - m_screenShakeIntensity
        );
    }
}

void GameWindow::drawStatisticsChart(QPainter *painter, const QRect &rect)
{
    // 绘制简单的柱状图来显示得分统计
    painter->setFont(QFont("Arial", 14));
    painter->setPen(Qt::white);
    painter->drawText(rect, Qt::AlignTop | Qt::AlignHCenter, "得分统计");

    // 绘制柱状图背景
    painter->setBrush(QColor(0, 0, 0, 100));
    painter->drawRect(rect.x(), rect.y() + 30, rect.width(), rect.height() - 30);

    // 绘制最高分和当前分数
    int maxBarHeight = rect.height() - 60;
    int currentBarHeight = (m_score * maxBarHeight) / qMax(m_highScore, m_score + 1);
    int highScoreBarHeight = (m_highScore * maxBarHeight) / qMax(m_highScore, 1);

    painter->setBrush(QColor(0, 150, 255));
    painter->drawRect(rect.x() + 50, rect.y() + rect.height() - 30 - currentBarHeight, 50, currentBarHeight);

    painter->setBrush(QColor(255, 215, 0));
    painter->drawRect(rect.x() + rect.width() - 100, rect.y() + rect.height() - 30 - highScoreBarHeight, 50, highScoreBarHeight);

    painter->drawText(QRect(rect.x() + 50, rect.y() + rect.height() - 20, 50, 20), Qt::AlignCenter, "当前");
    painter->drawText(QRect(rect.x() + rect.width() - 100, rect.y() + rect.height() - 20, 50, 20), Qt::AlignCenter, "最高");
}

bool GameWindow::isButtonClicked(const QRect &buttonRect, const QPoint &clickPos)
{
    return buttonRect.contains(clickPos);
}

bool GameWindow::isButtonHovered(const QRect &buttonRect, const QPoint &hoverPos)
{
    return buttonRect.contains(hoverPos);
}

qreal GameWindow::lerp(qreal start, qreal end, qreal factor)
{
    return start + (end - start) * factor;
}

void GameWindow::loadHighScore()
{
    QSettings settings("SkiGame", "HighScore");
    m_highScore = settings.value("highScore", 0).toInt();
}

void GameWindow::saveHighScore()
{
    if (m_score > m_highScore) {
        m_highScore = m_score;
        QSettings settings("SkiGame", "HighScore");
        settings.setValue("highScore", m_highScore);
    }
}