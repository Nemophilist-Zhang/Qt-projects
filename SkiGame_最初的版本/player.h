#ifndef PLAYER_H
#define PLAYER_H

#include <QObject>
#include <QRect>
#include <QPainter>
#include <QImage>
#include <QPointF>
#include <QElapsedTimer>

/**
 * @brief 玩家类，表示游戏中的滑雪者
 * 
 * 负责处理玩家的位置、大小、移动逻辑、血量和状态
 */
class Player : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief 玩家状态枚举
     */
    enum PlayerState {
        Normal,     // 正常状态
        SpeedUp,    // 加速状态
        Invincible, // 无敌状态
        Shield,     // 护盾状态
        Magnet      // 磁铁状态
    };
    
    /**
     * @brief 玩家动画状态枚举
     */
    enum AnimationState {
        Idle,       //  idle
        Moving,     // 移动
        Jumping,    // 跳跃
        Landing     // 落地
    };
    
    /**
     * @brief 构造函数
     * @param x 玩家初始x坐标
     * @param y 玩家初始y坐标
     * @param width 玩家宽度
     * @param height 玩家高度
     * @param parent 父对象
     */
    Player(int x, int y, int width, int height, QObject *parent = nullptr);
    
    /**
     * @brief 析构函数
     */
    ~Player();
    
    /**
     * @brief 获取玩家矩形（用于绘制和碰撞检测）
     * @return 玩家的矩形区域
     */
    QRect getRect() const;
    
    /**
     * @brief 玩家向左移动
     * @param step 移动步长
     * @param acceleration 是否使用加速度
     */
    void moveLeft(int step, bool acceleration = true);
    
    /**
     * @brief 玩家向右移动
     * @param step 移动步长
     * @param maxX 最大x坐标（边界）
     * @param acceleration 是否使用加速度
     */
    void moveRight(int step, int maxX, bool acceleration = true);
    
    /**
     * @brief 设置玩家y坐标
     * @param y 新的y坐标
     */
    void setY(int y);
    
    /**
     * @brief 设置玩家位置
     * @param x 新的x坐标
     * @param y 新的y坐标
     */
    void setPosition(int x, int y);
    
    /**
     * @brief 获取玩家位置
     * @return 玩家当前位置
     */
    QPointF getPosition() const;
    
    /**
     * @brief 获取玩家x坐标
     * @return 玩家当前x坐标
     */
    int getX() const;
    
    /**
     * @brief 获取玩家y坐标
     * @return 玩家当前y坐标
     */
    int getY() const;
    
    /**
     * @brief 获取玩家宽度
     * @return 玩家宽度
     */
    int getWidth() const;
    
    /**
     * @brief 获取玩家高度
     * @return 玩家高度
     */
    int getHeight() const;
    
    /**
     * @brief 设置玩家血量
     * @param hp 新的血量值
     */
    void setHP(int hp);
    
    /**
     * @brief 获取玩家当前血量
     * @return 当前血量值
     */
    int getHP() const;
    
    /**
     * @brief 获取玩家最大血量
     * @return 最大血量值
     */
    int getMaxHP() const;
    
    /**
     * @brief 减少玩家血量
     * @param amount 减少的血量值
     */
    void decreaseHP(int amount);
    
    /**
     * @brief 增加玩家血量
     * @param amount 增加的血量值
     */
    void increaseHP(int amount);
    
    /**
     * @brief 设置玩家状态
     * @param state 新的状态
     * @param duration 状态持续时间（帧数），0表示无限持续
     */
    void setState(PlayerState state, int duration = 0);
    
    /**
     * @brief 添加玩家状态（可叠加）
     * @param state 要添加的状态
     * @param duration 状态持续时间（帧数）
     */
    void addState(PlayerState state, int duration = 0);
    
    /**
     * @brief 移除玩家状态
     * @param state 要移除的状态
     */
    void removeState(PlayerState state);
    
    /**
     * @brief 检查玩家是否处于某状态
     * @param state 要检查的状态
     * @return 如果处于该状态返回true，否则返回false
     */
    bool hasState(PlayerState state) const;
    
    /**
     * @brief 获取玩家当前状态
     * @return 当前状态
     */
    PlayerState getState() const;
    
    /**
     * @brief 获取玩家所有状态
     * @return 状态列表
     */
    QList<PlayerState> getStates() const;
    
    /**
     * @brief 更新玩家状态（减少持续时间）
     */
    void updateState();
    
    /**
     * @brief 更新玩家动画
     */
    void updateAnimation();
    
    /**
     * @brief 设置动画状态
     * @param animState 新的动画状态
     */
    void setAnimationState(AnimationState animState);
    
    /**
     * @brief 获取动画状态
     * @return 当前动画状态
     */
    AnimationState getAnimationState() const;
    
    /**
     * @brief 设置玩家基础移动速度
     * @param speed 新的速度值
     */
    void setSpeed(int speed);
    
    /**
     * @brief 获取玩家基础移动速度
     * @return 基础移动速度
     */
    int getSpeed() const;
    
    /**
     * @brief 获取玩家当前移动速度（考虑状态加成）
     * @return 当前移动速度
     */
    int getCurrentSpeed() const;
    
    /**
     * @brief 获取玩家当前加速度
     * @return 当前加速度
     */
    float getAcceleration() const;
    
    /**
     * @brief 设置玩家加速度
     * @param acceleration 新的加速度
     */
    void setAcceleration(float acceleration);
    
    /**
     * @brief 绘制玩家
     * @param painter 绘图设备
     * @param playerImage 玩家图像
     */
    void draw(QPainter *painter, const QImage &playerImage);
    
    /**
     * @brief 绘制玩家特效
     * @param painter 绘图设备
     */
    void drawEffects(QPainter *painter);

signals:
    /**
     * @brief 血量变化信号
     * @param currentHP 当前血量
     * @param maxHP 最大血量
     */
    void hpChanged(int currentHP, int maxHP);
    
    /**
     * @brief 状态变化信号
     * @param newState 新的状态
     */
    void stateChanged(PlayerState newState);
    
    /**
     * @brief 动画状态变化信号
     * @param newAnimState 新的动画状态
     */
    void animationStateChanged(AnimationState newAnimState);

private:
    int m_x;              // 玩家x坐标
    int m_y;              // 玩家y坐标
    int m_width;          // 玩家宽度
    int m_height;         // 玩家高度
    int m_hp;             // 当前血量
    const int m_maxHP;    // 最大血量
    int m_speed;          // 基础移动速度
    PlayerState m_state;  // 当前主状态
    QList<PlayerState> m_activeStates; // 所有激活的状态
    QMap<PlayerState, int> m_stateDurations; // 状态持续时间
    float m_acceleration; // 加速度
    float m_currentSpeed; // 当前速度（带方向）
    AnimationState m_animationState; // 当前动画状态
    int m_animationFrame; // 当前动画帧
    int m_animationCounter; // 动画计数器
    QElapsedTimer m_animationTimer; // 动画计时器
    float m_rotation;     // 旋转角度（用于倾斜效果）
    float m_jumpHeight;   // 跳跃高度
    bool m_isJumping;     // 是否正在跳跃
    bool m_isLanding;     // 是否正在落地
    float m_jumpProgress; // 跳跃进度（0.0-1.0）
    QElapsedTimer m_jumpTimer; // 跳跃计时器
};

#endif // PLAYER_H