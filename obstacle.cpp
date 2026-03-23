#include "obstacle.h"

Obstacle::Obstacle(ObstacleType type, QGraphicsItem *parent)
    : QGraphicsItem(parent)
{
    m_type = type;
    // 按类型设置大小
    switch (m_type) {
    case OBSTACLE_ROCK:
        m_size = QSizeF(60, 40);
        break;
    case OBSTACLE_SNOWDRIFT:
        m_size = QSizeF(80, 30);
        break;
    }
}

Obstacle::~Obstacle()
{
}

QRectF Obstacle::boundingRect() const
{
    return QRectF(0, 0, m_size.width(), m_size.height());
}

void Obstacle::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    painter->setPen(Qt::NoPen);

    switch (m_type) {
    case OBSTACLE_ROCK:
        // 绘制石块
        painter->setBrush(QBrush(QColor(105, 105, 105)));
        painter->drawEllipse(boundingRect());
        // 石块纹理
        painter->setPen(QPen(QColor(80, 80, 80), 2));
        painter->drawLine(10, 10, 25, 15);
        painter->drawLine(30, 20, 45, 18);
        painter->drawLine(20, 25, 35, 30);
        break;
    case OBSTACLE_SNOWDRIFT:
        // 绘制雪堆
        painter->setBrush(QBrush(QColor(255, 255, 255)));
        painter->drawRoundedRect(boundingRect(), 10, 10);
        // 雪堆阴影
        painter->setPen(QPen(QColor(200, 220, 240), 1));
        painter->setBrush(QBrush(QColor(220, 240, 255)));
        painter->drawEllipse(10, 5, 60, 20);
        break;
    }
}

void Obstacle::updateObstacle(qreal gameSpeed)
{
    // 向左移动
    setX(x() - gameSpeed);
}