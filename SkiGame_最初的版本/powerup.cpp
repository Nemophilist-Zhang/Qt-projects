#include "powerup.h"

PowerUp::PowerUp(int x, int y, int width, int height, int speed, PowerUpType type, 
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

QRect PowerUp::getRect() const
{
    return QRect(m_x, m_y, m_width, m_height);
}

void PowerUp::moveDown()
{
    m_y += m_speed;
}

bool PowerUp::isOutOfBounds(int maxY) const
{
    return m_y > maxY;
}

PowerUp::PowerUpType PowerUp::getType() const
{
    return m_type;
}

void PowerUp::draw(QPainter *painter)
{
    // 绘制道具图像
    if (!m_image.isNull()) {
        painter->drawImage(m_x, m_y, m_image.scaled(m_width, m_height, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        // 如果没有图像，使用颜色绘制
        painter->fillRect(m_x, m_y, m_width, m_height, m_color);
    }
    
    // 绘制旋转效果
    painter->save();
    painter->translate(m_x + m_width / 2, m_y + m_height / 2);
    painter->rotate(QTime::currentTime().msec() / 10);
    painter->setPen(QPen(Qt::white, 2));
    painter->drawEllipse(-m_width / 2 - 5, -m_height / 2 - 5, m_width + 10, m_height + 10);
    painter->restore();
}