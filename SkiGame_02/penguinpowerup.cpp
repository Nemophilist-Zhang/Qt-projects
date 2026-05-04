#include "penguinpowerup.h"

PenguinPowerup::PenguinPowerup(double x, double y, double scale, double bobPhase)
    : Powerup(x, y, scale)
    , bobPhase_(bobPhase)
{
}

void PenguinPowerup::updateAnimation(double dt)
{
    bobPhase_ += dt * 4.0;
}

bool PenguinPowerup::collidesWith(double playerX, double playerY, double playerRadius) const
{
    const double dx = x_ - playerX;
    const double dy = y_ - playerY;
    const double radius = playerRadius + 18.0 * scale_;
    return dx * dx + dy * dy < radius * radius;
}

double PenguinPowerup::bobPhase() const { return bobPhase_; }
