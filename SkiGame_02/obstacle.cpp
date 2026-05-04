#include "obstacle.h"

RockObstacle::RockObstacle(double x, double y, double size)
    : x_(x)
    , y_(y)
    , size_(size)
{
}

double RockObstacle::x() const { return x_; }
double RockObstacle::y() const { return y_; }
double RockObstacle::size() const { return size_; }

double RockObstacle::collisionRadius(double playerRadius) const
{
    return playerRadius + size_ * 0.5;
}

bool RockObstacle::collidesWith(double playerX, double playerY, double playerRadius) const
{
    const double dx = x_ - playerX;
    const double dy = y_ - playerY;
    const double radius = collisionRadius(playerRadius);
    return dx * dx + dy * dy < radius * radius;
}
