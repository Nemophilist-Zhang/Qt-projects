#ifndef MONSTER_H
#define MONSTER_H

#include <QObject>
#include <QRect>
#include <QPainter>
#include <QImage>
#include <QPointF>
#include <QElapsedTimer>
#include "player.h"

/**
 * @brief 怪物类，表示游戏中从下方追击玩家的怪物
 * 
 * 负责处理怪物的位置、大小、速度和移动逻辑
 */
class Monster : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief 怪物AI模式枚举
     */
    enum AIMode {
        Basic,      // 基础追踪
        Predictive  // 预判走位
    };
    
    /**
     * @brief 怪物动画状态枚举
     */
    enum AnimationState {
        Idle,       //  idle
        Chasing,    // 追击
        Roaring     // 咆哮
    };
    /**
     * @brief 构造函数
     * @param x 怪物初始x坐标
     * @param y 怪物初始y坐标
     * @param width 怪物宽度
     * @param height 怪物高度
     * @param speed 怪物移动速度
     * @param image 怪物图像
     * @param parent 父对象
     */
    Monster(int x, int y, int width, int height, int speed, 
            const QImage& image, QObject *parent = nullptr);
    
    /**
     * @brief 析构函数
     */
    ~Monster();
    
    /**
     * @brief 获取怪物矩形（用于绘制和碰撞检测）
     * @return 怪物的矩形区域
     */
    QRect getRect() const;
    
    /**
     * @brief 怪物移动（追踪玩家）
     * @param player 玩家对象
     * @param difficulty 游戏难度
     */
    void move(Player *player, GameWindow::GameDifficulty difficulty);
    
    /**
     * @brief 检查怪物是否超出屏幕顶部
     * @param minY 屏幕最小y坐标
     * @return 如果怪物超出屏幕顶部返回true，否则返回false
     */
    bool isOutOfBounds(int minY) const;
    
    /**
     * @brief 设置怪物AI模式
     * @param mode AI模式
     */
    void setAIMode(AIMode mode);
    
    /**
     * @brief 获取怪物AI模式
     * @return 当前AI模式
     */
    AIMode getAIMode() const;
    
    /**
     * @brief 设置怪物愤怒状态
     * @param isEnraged 是否愤怒
     */
    void setEnraged(bool isEnraged);
    
    /**
     * @brief 获取怪物是否愤怒
     * @return 是否愤怒
     */
    bool isEnraged() const;
    
    /**
     * @brief 获取怪物与玩家的距离
     * @param player 玩家对象
     * @return 距离
     */
    float getDistanceToPlayer(Player *player) const;
    
    /**
     * @brief 更新怪物状态
     * @param player 玩家对象
     */
    void update(Player *player);
    
    /**
     * @brief 更新怪物动画
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
     * @brief 获取怪物伤害值
     * @return 怪物伤害值
     */
    int getDamage() const;
    
    /**
     * @brief 绘制怪物
     * @param painter 绘图设备
     */
    void draw(QPainter *painter);
    
    /**
     * @brief 绘制怪物特效
     * @param painter 绘图设备
     */
    void drawEffects(QPainter *painter);

signals:
    /**
     * @brief 怪物愤怒状态变化信号
     * @param enraged 是否愤怒
     */
    void enragedChanged(bool enraged);
    
    /**
     * @brief 怪物动画状态变化信号
     * @param newAnimState 新的动画状态
     */
    void animationStateChanged(AnimationState newAnimState);
private:
    int m_x;              // 怪物x坐标
    int m_y;              // 怪物y坐标
    int m_width;          // 怪物宽度
    int m_height;         // 怪物高度
    int m_baseSpeed;      // 基础移动速度
    int m_currentSpeed;   // 当前移动速度
    QImage m_image;       // 怪物图像
    AIMode m_aiMode;      // AI模式
    bool m_isEnraged;     // 是否愤怒
    float m_rageLevel;    // 愤怒等级（0.0-1.0）
    AnimationState m_animationState; // 当前动画状态
    int m_animationFrame; // 当前动画帧
    int m_animationCounter; // 动画计数器
    QElapsedTimer m_animationTimer; // 动画计时器
    QElapsedTimer m_roarTimer; // 咆哮计时器
    float m_rotation;     // 旋转角度
    QPointF m_targetPos;  // 目标位置
    float m_smokeParticleCounter; // 烟尘粒子计数器