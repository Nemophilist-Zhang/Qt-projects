#ifndef PLAYER_H
#define PLAYER_H

#include <QGraphicsItem>
#include <QPainter>
#include <QRectF>
#include <QTimer>

// 玩家状态枚举
enum PlayerState {
    STATE_RUNNING,    // 跑步
    STATE_JUMPING,    // 跳跃
    STATE_FRONTFLIP,  // 前空翻
    STATE_BACKFLIP,   // 后空翻
    STATE_FALLING,    // 摔倒
    STATE_ACCELERATING // 加速
};

class Player : public QGraphicsItem
{
public:
    Player(QGraphicsItem *parent = nullptr);
    ~Player();

    // 重写绘图区域和绘制函数
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    // 玩家操作接口
    void jump();
    void frontFlip();
    void backFlip();
    void startAccelerate();
    void stopAccelerate();
    void takeDamage(int damage);
    void fall();
    void slowDown();
    void updatePlayer(qreal groundY); // 每帧更新玩家状态

    // Getter和Setter
    int getHp() const { return m_hp; }
    void setHp(int hp) { m_hp = hp; }
    int getEnergy() const { return m_energy; }
    qreal getSpeedMultiplier() const { return m_speedMultiplier; }
    void setInvincible(bool invincible) { m_invincible = invincible; }
    bool isInvincible() const { return m_invincible; }

private:
    PlayerState m_state;      // 当前状态
    int m_hp;                 // 生命值
    int m_energy;             // 加速能量
    bool m_invincible;        // 是否无敌

    // 物理属性
    qreal m_velocityY;        // 垂直速度
    qreal m_gravity;          // 重力
    qreal m_jumpForce;        // 跳跃力
    qreal m_groundY;          // 地面Y坐标
    bool m_isOnGround;        // 是否在地面

    // 空翻相关
    qreal m_rotationAngle;    // 旋转角度
    qreal m_flipRotation;     // 空翻剩余旋转角度
    bool m_flipSuccess;       // 空翻是否成功

    // 加速相关
    bool m_isAccelerating;    // 是否正在加速
    qreal m_speedMultiplier;  // 速度倍数
};

#endif // PLAYER_H
