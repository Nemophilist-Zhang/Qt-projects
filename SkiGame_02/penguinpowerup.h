#ifndef PENGUINPOWERUP_H
#define PENGUINPOWERUP_H

#include "powerup.h"

class PenguinPowerup : public Powerup
{
public:
    PenguinPowerup(double x = 0, double y = 0, double scale = 1.0, double bobPhase = 0);

    void updateAnimation(double dt);
    bool collidesWith(double playerX, double playerY, double playerRadius) const;

    double bobPhase() const;

private:
    double bobPhase_ = 0;
};

#endif // PENGUINPOWERUP_H
