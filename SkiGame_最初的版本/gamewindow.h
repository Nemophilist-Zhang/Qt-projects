#ifndef GAMEWINDOW_H
#define GAMEWINDOW_H

#include <QWidget>
#include <QTimer>
#include <QList>
#include <QKeyEvent>
#include <QPainter>
#include <QMessageBox>
#include <QRandomGenerator>
#include <QMouseEvent>
#include <QImage>
#include <QSoundEffect>
#include <QMediaPlayer>
#include <QSettings>
#include <QTime>
#include <QPainterPath>
#include <QElapsedTimer>
#include <QResizeEvent>
#include "player.h"
#include "obstacle.h"
#include "monster.h"
#include "powerup.h"

/**
 * @brief 游戏窗口类，负责游戏的主界面和逻辑控制
 * 
 * 整合玩家、障碍物、道具、怪物和游戏逻辑，处理用户输入和游戏循环
 */
class GameWindow : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief 游戏场景枚举
     */
    enum GameScene {
        Snow,    // 雪地场景
        Forest,  // 森林场景
        Night    // 夜晚场景
    };
    
    /**
     * @brief 游戏难度枚举
     */
    enum GameDifficulty {
        Easy,    // 简单难度
        Normal,  // 普通难度
        Hard     // 困难难度
    };
    
    /**
     * @brief 天气类型枚举
     */
    enum WeatherType {
        Sunny,      // 晴天
        LightSnow,  // 小雪
        Blizzard    // 暴雪
    };
    
    /**
     * @brief 屏幕特效枚举
     */
    enum ScreenEffect {
        None,           // 无特效
        HurtFlash,      // 受伤闪烁
        MonsterChase,   // 怪物追击
        SpeedBoost,     // 加速
        FadeIn,         // 淡入
        FadeOut         // 淡出
    };
    
    /**
     * @brief 游戏状态枚举
     */
    enum GameState {
        Menu,                // 主菜单
        DifficultySelection, // 难度选择
        SceneSelection,      // 场景选择
        Playing,             // 游戏中
        GameOver             // 游戏结束
    };
    
    /**
     * @brief 构造函数
     * @param parent 父窗口
     */
    GameWindow(QWidget *parent = nullptr);
    
    /**
     * @brief 析构函数
     */
    ~GameWindow();

protected:
    /**
     * @brief 重写绘图事件，绘制游戏元素
     * @param event 绘图事件
     */
    void paintEvent(QPaintEvent *event) override;
    
    /**
     * @brief 重写窗口大小变化事件
     * @param event 窗口大小变化事件
     */
    void resizeEvent(QResizeEvent *event) override;
    
    /**
     * @brief 重写按键事件，处理用户输入
     * @param event 按键事件
     */
    void keyPressEvent(QKeyEvent *event) override;
    
    /**
     * @brief 重写按键释放事件，处理用户输入
     * @param event 按键事件
     */
    void keyReleaseEvent(QKeyEvent *event) override;
    
    /**
     * @brief 重写鼠标事件，处理用户点击
     * @param event 鼠标事件
     */
    void mousePressEvent(QMouseEvent *event) override;

private slots:
    /**
     * @brief 游戏循环更新
     */
    void updateGame();
    
    /**
     * @brief 开始新游戏
     */
    void startNewGame();
    
    /**
     * @brief 游戏结束处理
     */
    void gameOver();
    
    /**
     * @brief 处理玩家血量变化
     * @param currentHP 当前血量
     * @param maxHP 最大血量
     */
    void onPlayerHPChanged(int currentHP, int maxHP);
    
    /**
     * @brief 处理玩家状态变化
     * @param newState 新的状态
     */
    void onPlayerStateChanged(Player::PlayerState newState);
    
    /**
     * @brief 切换屏幕特效
     * @param effect 要切换的特效
     * @param duration 特效持续时间（毫秒）
     */
    void setScreenEffect(ScreenEffect effect, int duration = 0);
    
    /**
     * @brief 播放音效
     * @param soundName 音效名称
     */
    void playSound(const QString &soundName);
    
    /**
     * @brief 更新背景音乐
     */
    void updateBackgroundMusic();

private:
    /**
     * @brief 初始化游戏
     */
    void initGame();
    
    /**
     * @brief 初始化视差背景
     */
    void initParallaxBackground();
    
    /**
     * @brief 初始化粒子系统
     */
    void initParticleSystem();
    
    /**
     * @brief 加载资源（图像和音效）
     */
    void loadResources();
    
    /**
     * @brief 加载场景资源
     */
    void loadSceneResources();
    
    /**
     * @brief 更新视差背景
     */
    void updateParallaxBackground();
    
    /**
     * @brief 更新粒子系统
     */
    void updateParticles();
    
    /**
     * @brief 生成雪花粒子
     */
    void generateSnowflakes();
    
    /**
     * @brief 生成特效粒子
     * @param x 生成位置x坐标
     * @param y 生成位置y坐标
     * @param type 粒子类型
     */
    void generateEffectParticles(int x, int y, const QString &type);
    
    /**
     * @brief 生成新障碍物
     */
    void generateObstacle();
    
    /**
     * @brief 生成新道具
     */
    void generatePowerUp();
    
    /**
     * @brief 检查碰撞
     */
    void checkCollisions();
    
    /**
     * @brief 处理玩家受伤
     * @param damage 伤害值
     */
    void handlePlayerHurt(int damage);
    
    /**
     * @brief 处理道具拾取
     * @param powerUp 道具对象
     */
    void handlePowerUpPickup(PowerUp *powerUp);
    
    /**
     * @brief 应用道具效果
     * @param powerUp 道具对象
     */
    void applyPowerUp(PowerUp *powerUp);
    
    /**
     * @brief 更新分数
     */
    void updateScore();
    
    /**
     * @brief 更新游戏难度
     */
    void updateGameDifficulty();
    
    /**
     * @brief 更新天气效果
     */
    void updateWeather();
    
    /**
     * @brief 绘制开始菜单
     * @param painter 绘图设备
     */
    void drawMenu(QPainter *painter);
    
    /**
     * @brief 绘制难度选择菜单
     * @param painter 绘图设备
     */
    void drawDifficultySelection(QPainter *painter);
    
    /**
     * @brief 绘制场景选择菜单
     * @param painter 绘图设备
     */
    void drawSceneSelection(QPainter *painter);
    
    /**
     * @brief 绘制游戏结束界面
     * @param painter 绘图设备
     */
    void drawGameOver(QPainter *painter);
    
    /**
     * @brief 绘制视差背景
     * @param painter 绘图设备
     */
    void drawParallaxBackground(QPainter *painter);
    
    /**
     * @brief 绘制粒子效果
     * @param painter 绘图设备
     */
    void drawParticles(QPainter *painter);
    
    /**
     * @brief 绘制屏幕特效
     * @param painter 绘图设备
     */
    void drawScreenEffects(QPainter *painter);
    
    /**
     * @brief 绘制HUD界面
     * @param painter 绘图设备
     */
    void drawHUD(QPainter *painter);
    
    /**
     * @brief 绘制道具图标
     * @param painter 绘图设备
     */
    void drawPowerUpIcons(QPainter *painter);
    
    /**
     * @brief 绘制按钮
     * @param painter 绘图设备
     * @param rect 按钮矩形
     * @param text 按钮文本
     * @param isHovered 是否悬停
     * @param isPressed 是否按下
     */
    void drawButton(QPainter *painter, const QRect &rect, const QString &text, bool isHovered = false, bool isPressed = false);
    
    /**
     * @brief 绘制血条
     * @param painter 绘图设备
     */
    void drawHPBar(QPainter *painter);
    
    /**
     * @brief 绘制分数和等级
     * @param painter 绘图设备
     */
    void drawScoreAndLevel(QPainter *painter);
    
    /**
     * @brief 绘制状态图标
     * @param painter 绘图设备
     */
    void drawStatusIcons(QPainter *painter);
    
    /**
     * @brief 绘制统计图表
     * @param painter 绘图设备
     * @param rect 图表矩形区域
     */
    void drawStatisticsChart(QPainter *painter, const QRect &rect);
    
    /**
     * @brief 检查按钮点击
     * @param buttonRect 按钮矩形
     * @param clickPos 点击位置
     * @return 如果点击在按钮内返回true，否则返回false
     */
    bool isButtonClicked(const QRect &buttonRect, const QPoint &clickPos);
    
    /**
     * @brief 检查按钮悬停
     * @param buttonRect 按钮矩形
     * @param hoverPos 悬停位置
     * @return 如果悬停在按钮内返回true，否则返回false
     */
    bool isButtonHovered(const QRect &buttonRect, const QPoint &hoverPos);
    
    /**
     * @brief 平滑插值函数
     * @param start 起始值
     * @param end 结束值
     * @param factor 插值因子（0.0-1.0）
     * @return 插值结果
     */
    qreal lerp(qreal start, qreal end, qreal factor);
    
    /**
     * @brief 加载最高分数
     */
    void loadHighScore();
    
    /**
     * @brief 保存最高分数
     */
    void saveHighScore();
    
    // 游戏状态
    GameState m_gameState;      // 当前游戏状态
    GameDifficulty m_difficulty; // 当前游戏难度
    GameScene m_scene;          // 当前游戏场景
    bool m_isGameRunning;       // 游戏是否正在运行
    int m_score;                // 当前分数
    int m_level;                // 当前等级
    int m_highScore;            // 最高分数
    
    // 游戏元素
    Player *m_player;                  // 玩家对象
    QList<Obstacle *> m_obstacles;     // 障碍物列表
    QList<PowerUp *> m_powerUps;       // 道具列表
    Monster *m_monster;                // 追击怪物
    QList<QObject *> m_particles;      // 粒子效果列表
    
    // 游戏参数
    const int GAME_WIDTH;              // 游戏窗口宽度
    const int GAME_HEIGHT;             // 游戏窗口高度
    const int PLAYER_WIDTH;            // 玩家宽度
    const int PLAYER_HEIGHT;           // 玩家高度
    int m_playerSpeed;                 // 玩家移动速度（根据难度调整）
    int m_baseObstacleSpeed;           // 基础障碍物速度（根据难度调整）
    int m_obstacleGenerationInterval;  // 障碍物生成间隔（根据难度调整）
    int m_powerUpGenerationInterval;   // 道具生成间隔
    int m_monsterSpeed;                // 怪物速度（根据难度调整）
    float m_difficultyMultiplier;      // 难度乘数
    int m_weatherChangeInterval;       // 天气变化间隔
    int m_weatherChangeCounter;        // 天气变化计数器
    
    // 游戏定时器
    QTimer *m_gameTimer;               // 游戏主循环定时器
    int m_obstacleGenerationCounter;   // 障碍物生成计数器
    int m_powerUpGenerationCounter;    // 道具生成计数器
    int m_particleGenerationCounter;   // 粒子生成计数器
    int m_screenShakeCounter;          // 屏幕抖动计数器
    float m_screenShakeIntensity;      // 屏幕抖动强度
    float m_effectOpacity;             // 特效透明度
    int m_effectDuration;              // 特效持续时间
    
    // 视差背景
    struct ParallaxLayer {
        QImage image;
        float scrollSpeed;
        float xOffset;
        float yOffset;
    };
    QList<ParallaxLayer> m_parallaxLayers;
    
    // 粒子系统
    struct Particle {
        QPointF position;
        QPointF velocity;
        QSize size;
        QColor color;
        float opacity;
        float rotation;
        float life;
        float maxLife;
        QString type;
    };
    
    // 音效和音乐
    QSoundEffect *m_moveSound;         // 移动音效
    QSoundEffect *m_collisionSound;    // 碰撞音效
    QSoundEffect *m_powerUpSound;      // 拾取道具音效
    QSoundEffect *m_gameOverSound;     // 游戏结束音效
    QSoundEffect *m_hurtSound;         // 受伤音效
    QSoundEffect *m_shieldSound;       // 护盾音效
    QSoundEffect *m_coinSound;         // 金币音效
    QMediaPlayer *m_backgroundMusic;   // 背景音乐
    
    // 输入状态
    bool m_leftPressed;                // 左键是否按下
    bool m_rightPressed;               // 右键是否按下
    bool m_spacePressed;               // 空格键是否按下
    QPoint m_mousePos;                 // 鼠标位置
    bool m_mousePressed;               // 鼠标是否按下
    
    // 按钮状态
    bool m_startButtonHovered;         // 开始按钮悬停
    bool m_startButtonPressed;         // 开始按钮按下
    bool m_exitButtonHovered;          // 退出按钮悬停
    bool m_exitButtonPressed;          // 退出按钮按下
    bool m_restartButtonHovered;       // 重新开始按钮悬停
    bool m_restartButtonPressed;       // 重新开始按钮按下
    bool m_menuButtonHovered;          // 返回菜单按钮悬停
    bool m_menuButtonPressed;          // 返回菜单按钮按下
    
    // 道具系统
    QMap<PowerUp::PowerUpType, int> m_powerUpCooldowns;  // 道具冷却时间
    QMap<PowerUp::PowerUpType, int> m_powerUpDurations;  // 道具持续时间
    bool m_isShieldActive;             // 护盾是否激活
    bool m_isMagnetActive;             // 磁铁是否激活
    bool m_isSpeedBoostActive;         // 加速是否激活
};

#endif // GAMEWINDOW_H