#include "obstacle.h"

Obstacle::Obstacle(int x, int y, int width, int height, int speed, ObstacleType type, 
                   const QColor& color, const QImage& image, QObject *parent)
    : QObject(parent), 
      m_x(x), 
      m_y(y), 
      m_width(width), 
      m_height(height), 
      m_speed(speed), 
      m_type(type),
      m_color(color),
      m_image(image)
{
}

QRect Obstacle::getRect() const
{
    return QRect(m_x, m_y, m_width, m_height);
}

void Obstacle::moveDown()
{
    m_y += m_speed;
}

bool Obstacle::isOutOfBounds(int maxY) const
{
    return m_y > maxY;
}

void Obstacle::setY(int y)
{
    m_y = y;
}

int Obstacle::getY() const
{
    return m_y;
}

void Obstacle::setSpeed(int speed)
{
    m_speed = speed;
}

int Obstacle::getSpeed() const
{
    return m_speed;
}

Obstacle::ObstacleType Obstacle::getType() const
{
    return m_type;
}

int Obstacle::getDamage() const
{
    // 根据障碍物类型返回伤害值
    switch (m_type) {
    case Tree:
        return 1;
    case Rock:
        return 2;
    case Pit:
        return 1;
    case Ice:
        return 0;  // 冰面不造成伤害，但会影响移动
    default:
        return 0;
    }
}

void Obstacle::draw(QPainter *painter)
{
    // 绘制障碍物图像
    if (!m_image.isNull()) {
        painter->drawImage(m_x, m_y, m_image.scaled(m_width, m_height, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        // 如果没有图像，使用颜色绘制
        painter->fillRect(m_x, m_y, m_width, m_height, m_color);
    }
}