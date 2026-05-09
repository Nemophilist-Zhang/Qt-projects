#include "monster.h"

Monster::Monster(int x, int y, int width, int height, int speed, 
                 const QImage& image, QObject *parent)
    : QObject(parent), 
      m_x(x), 
      m_y(y), 
      m_width(width), 
      m_height(height), 
      m_speed(speed),
      m_image(image)
{
}

QRect Monster::getRect() const
{
    return QRect(m_x, m_y, m_width, m_height);
}

void Monster::move(Player *player)
{
    // 追踪玩家的x坐标
    if (player->getX() < m_x) {
        m_x -= m_speed;
    } else if (player->getX() > m_x) {
        m_x += m_speed;
    }
    
    // 向上移动
    m_y -= m_speed;
}

bool Monster::isOutOfBounds(int minY) const
{
    return m_y + m_height < minY;
}

void Monster::setSpeed(int speed)
{
    m_speed = speed;
}

int Monster::getSpeed() const
{
    return m_speed;
}

int Monster::getDamage() const
{
    // 怪物造成2点伤害
    return 2;
}

void Monster::draw(QPainter *painter)
{
    // 绘制怪物图像
    if (!m_image.isNull()) {
        painter->drawImage(m_x, m_y, m_image.scaled(m_width, m_height, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        // 如果没有图像，使用红色绘制
        painter->fillRect(m_x, m_y, m_width, m_height, Qt::red);
    }
}