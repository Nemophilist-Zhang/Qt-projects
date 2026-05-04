#ifndef AVALANCHE_H
#define AVALANCHE_H

#include <random>

class Avalanche
{
public:
    void reset();
    void spawn(double startFrontX, std::mt19937 &rng);
    void update(double dt, double playerX, double playerBaseSpeed);

    bool isActive() const;
    bool hasCaught(double playerX, double catchDistance) const;
    bool shouldDespawn(double cameraX) const;
    int gapMeters(double playerX) const;

    double frontX() const;
    double elapsed() const;

private:
    double frontX_ = 0;
    double elapsed_ = 0;
    double targetGap_ = 220.0;
    double pressure_ = 1.0;
    bool active_ = false;
};

#endif // AVALANCHE_H
