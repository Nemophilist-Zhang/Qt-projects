#include "player.h"

Player::Player(int x, int y, int width, int height, QObject *parent)
    : QObject(parent), 
      m_x(x), 
      m_y(y), 
      m_width(width), 
      m_height(height),
      m_maxHP(3),
      m_hp(m_maxHP),
      m_speed(8),
      m_state(Normal),
      m_stateDuration(0)
{
}

QRect Player::getRect() const
{
    return QRect(m_x, m_y, m_width, m_height);
}

void Player::moveLeft(int step)
{
    if (m_x - step >= 0) {
        m_x -= step;
    } else {
        m_x = 0;
    }
}

void Player::moveRight(int step, int maxX)
{
    if (m_x + step <= maxX) {
        m_x += step;
    } else {
        m_x = maxX;
    }
}

void Player::setX(int x)
{
    m_x = x;
}

int Player::getX() const
{
    return m_x;
}

int Player::getY() const
{
    return m_y;
}

int Player::getWidth() const
{
    return m_width;
}

int Player::getHeight() const
{
    return m_height;
}

void Player::setHP(int hp)
{
    m_hp = qBound(0, hp, m_maxHP);
    emit hpChanged(m_hp, m_maxHP);
}

int Player::getHP() const
{
    return m_hp;
}

int Player::getMaxHP() const
{
    return m_maxHP;
}

void Player::decreaseHP(int amount)
{
    // 如果处于无敌状态，不减少血量
    if (m_state == Invincible) {
        return;
    }
    
    setHP(m_hp - amount);
}

void Player::increaseHP(int amount)
{
    setHP(m_hp + amount);
}

void Player::setState(PlayerState state, int duration)
{
    m_state = state;
    m_stateDuration = duration;
    emit stateChanged(m_state);
}

Player::PlayerState Player::getState() const
{
    return m_state;
}

void Player::updateState()
{
    if (m_stateDuration > 0) {
        m_stateDuration--;
        if (m_stateDuration == 0) {
            m_state = Normal;
            emit stateChanged(m_state);
        }
    }
}

void Player::setSpeed(int speed)
{
    m_speed = speed;
}

int Player::getSpeed() const
{
    return m_speed;
}

int Player::getCurrentSpeed() const
{
    // 根据状态调整速度
    switch (m_state) {
    case SpeedUp:
        // 加速状态：速度提升30%
        return static_cast<int>(m_speed * 1.3);
    case Invincible:
        // 无敌状态：速度正常
        return m_speed;
    case Normal:
    default:
        return m_speed;
    }
}

void Player::draw(QPainter *painter, const QImage &playerImage)
{
    // 根据状态设置绘制效果
    if (m_state == Invincible) {
        // 无敌状态：闪烁效果
        if (m_stateDuration % 10 < 5) {
            painter->setOpacity(0.5);
        }
    } else if (m_state == SpeedUp) {
        // 加速状态：绘制黄色边框
        painter->setPen(QPen(Qt::yellow, 3));
        painter->drawRect(m_x - 2, m_y - 2, m_width + 4, m_height + 4);
    }
    
    // 绘制玩家图像
    painter->drawImage(m_x, m_y, playerImage.scaled(m_width, m_height, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    
    // 重置透明度
    painter->setOpacity(1.0);
}