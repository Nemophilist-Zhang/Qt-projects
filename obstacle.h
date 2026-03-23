#ifndef OBSTACLE_H
#define OBSTACLE_H

#include <QGraphicsItem>
#include <QPainter>
#include <QRectF>

// 障碍物类型枚举
enum ObstacleType {
    OBSTACLE_ROCK = 0,    // 石块（碰撞扣血）
    OBSTACLE_SNOWDRIFT    // 雪堆（碰撞减速）
};

class Obstacle : public QGraphicsItem
{
public:
    Obstacle(ObstacleType type, QGraphicsItem *parent = nullptr);
    ~Obstacle();

    // 重写绘图区域和绘制函数
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    // 更新障碍物位置
    void updateObstacle(qreal gameSpeed);

    // Getter
    ObstacleType getType() const { return m_type; }

private:
    ObstacleType m_type;    // 障碍物类型
    QSizeF m_size;          // 障碍物大小
};

#endif // OBSTACLE_H