#ifndef OBSTACLE_H
#define OBSTACLE_H

class RockObstacle
{
public:
    RockObstacle(double x = 0, double y = 0, double size = 0);

    double x() const;
    double y() const;
    double size() const;
    double collisionRadius(double playerRadius) const;

    bool collidesWith(double playerX, double playerY, double playerRadius) const;

private:
    double x_ = 0;
    double y_ = 0;
    double size_ = 0;
};

#endif // OBSTACLE_H
