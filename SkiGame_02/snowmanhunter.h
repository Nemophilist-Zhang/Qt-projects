#ifndef SNOWMANHUNTER_H
#define SNOWMANHUNTER_H

#include <functional>
#include <random>

class SnowmanHunter
{
public:
    void reset();
    void spawn(double startX, double startY, std::mt19937 &rng);
    void update(double dt, double playerX, double playerBaseSpeed, double cameraX,
                const std::function<double(double)> &slopeY);

    bool isActive() const;
    bool isLethalChase() const;
    bool hasCaught(double playerX, double playerY, double catchDistance) const;
    bool shouldDespawn(double cameraX) const;
    int gapMeters(double playerX) const;

    double x() const;
    double y() const;
    double scale() const;

private:
    double x_ = 0;
    double y_ = 0;
    double scale_ = 1.0;
    double elapsed_ = 0;
    double targetGap_ = 220.0;
    double aggression_ = 1.0;
    bool lethalChase_ = false;
    bool active_ = false;
};

#endif // SNOWMANHUNTER_H
