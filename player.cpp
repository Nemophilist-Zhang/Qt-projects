#include "player.h"
#include <QTimer>

Player::Player(QGraphicsItem *parent)
    : QGraphicsItem(parent)
{
    m_state = STATE_RUNNING;
    m_hp = 2;
    m_energy = 100;
    m_invincible = false;

    m_velocityY = 0;
    m_gravity = 0.6;
    m_jumpForce = -15;
    m_groundY = 0;
    m_isOnGround = true;

    m_rotationAngle = 0;
    m_flipRotation = 0;
    m_flipSuccess = false;

    m_isAccelerating = false;
    m_speedMultiplier = 1.0;
}

Player::~Player()
{
}

QRectF Player::boundingRect() const
{
    return QRectF(-20, -40, 40, 80);
}

void Player::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    painter->save();

    // 无敌状态闪烁效果
    if (m_invincible) {
        static int alpha = 255;
        static bool fade = true;
        alpha += fade ? -15 : 15;
        if (alpha <= 100) fade = false;
        if (alpha >= 255) fade = true;
        painter->setOpacity(alpha / 255.0);
    }

    // 空翻旋转
    painter->rotate(m_rotationAngle);

    // 绘制滑雪者
    painter->setPen(Qt::NoPen);
    // 加速状态身体变色
    painter->setBrush(QBrush(m_isAccelerating ? QColor(255, 100, 100) : QColor(30, 144, 255)));
    painter->drawRect(-10, -20, 20, 30); // 身体
    // 头部
    painter->setBrush(QBrush(QColor(255, 220, 180)));
    painter->drawEllipse(-8, -35, 16, 16);
    // 滑雪板
    painter->setBrush(QBrush(QColor(0, 0, 0)));
    painter->drawRect(-20, 40, 40, 5);
    // 滑雪杖
    painter->setPen(QPen(QColor(139, 69, 19), 3));
    painter->drawLine(-15, -10, -25, 35);
    painter->drawLine(15, -10, 25, 35);

    painter->restore();
}

void Player::jump()
{
    if (m_isOnGround && m_state != STATE_FALLING) {
        m_isOnGround = false;
        m_velocityY = m_jumpForce;
        m_state = STATE_JUMPING;
    }
}

void Player::frontFlip()
{
    if (!m_isOnGround && m_state != STATE_FRONTFLIP && m_state != STATE_BACKFLIP && m_state != STATE_FALLING) {
        m_state = STATE_FRONTFLIP;
        m_flipRotation = 360;
        m_rotationAngle = 0;
        m_flipSuccess = false;
    }
}

void Player::backFlip()
{
    if (!m_isOnGround && m_state != STATE_FRONTFLIP && m_state != STATE_BACKFLIP && m_state != STATE_FALLING) {
        m_state = STATE_BACKFLIP;
        m_flipRotation = -360;
        m_rotationAngle = 0;
        m_flipSuccess = false;
    }
}

void Player::startAccelerate()
{
    if (m_energy > 0 && m_state != STATE_FALLING) {
        m_isAccelerating = true;
        m_speedMultiplier = 1.5;
        m_state = STATE_ACCELERATING;
    }
}

void Player::stopAccelerate()
{
    m_isAccelerating = false;
    m_speedMultiplier = 1.0;
    if (m_state == STATE_ACCELERATING) {
        m_state = STATE_RUNNING;
    }
}

void Player::takeDamage(int damage)
{
    if (m_invincible) return;
    m_hp -= damage;
    if (m_hp < 0) m_hp = 0;
}

void Player::fall()
{
    m_state = STATE_FALLING;
    m_velocityY = 0;
    // 摔倒后1秒恢复
    QTimer::singleShot(1000, [this]() {
        if (m_hp > 0) {
            m_state = STATE_RUNNING;
        }
    });
}

void Player::slowDown()
{
    m_speedMultiplier = 0.5;
    QTimer::singleShot(500, [this]() {
        if (m_state != STATE_FALLING) {
            m_speedMultiplier = 1.0;
        }
    });
}

void Player::updatePlayer(qreal groundY)
{
    m_groundY = groundY;

    // 加速能量消耗与恢复
    if (m_isAccelerating) {
        m_energy -= 1;
        if (m_energy <= 0) {
            m_energy = 0;
            stopAccelerate();
        }
    } else {
        if (m_energy < 100) {
            m_energy += 0.1;
        }
    }

    // 摔倒状态不更新物理
    if (m_state == STATE_FALLING) {
        return;
    }

    // 空翻动画更新
    if (m_state == STATE_FRONTFLIP || m_state == STATE_BACKFLIP) {
        qreal rotateSpeed = m_flipRotation / 15.0;
        m_rotationAngle += rotateSpeed;
        m_flipRotation -= rotateSpeed;

        if (qAbs(m_flipRotation) <= 5) {
            m_flipSuccess = true;
        }
    }

    // 重力物理更新
    if (!m_isOnGround) {
        m_velocityY += m_gravity;
        setY(y() + m_velocityY);

        // 落地检测
        if (y() + boundingRect().height() >= m_groundY) {
            setY(m_groundY - boundingRect().height());
            m_velocityY = 0;
            m_isOnGround = true;

            // 空翻落地处理
            if (m_state == STATE_FRONTFLIP || m_state == STATE_BACKFLIP) {
                if (m_flipSuccess) {
                    m_energy = qMin(100, m_energy + 30);
                } else {
                    takeDamage(1);
                    fall();
                }
                m_rotationAngle = 0;
                m_state = STATE_RUNNING;
            } else {
                m_state = STATE_RUNNING;
            }
        }
    }
}
