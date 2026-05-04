#include "powerup.h"

#include <cmath>

Powerup::Powerup(double x, double y, double scale)
    : x_(x)
    , y_(y)
    , scale_(scale)
{
}

double Powerup::x() const { return x_; }
double Powerup::y() const { return y_; }
double Powerup::scale() const { return scale_; }

void Powerup::setPosition(double x, double y)
{
    x_ = x;
    y_ = y;
}

Coin::Coin(double x, double y, double spinPhase)
    : Powerup(x, y, 1.0)
    , spinPhase_(spinPhase)
{
}

void Coin::updateAnimation(double dt)
{
    spinPhase_ += dt * 6.0;
}

bool Coin::tryCollect(double playerX, double playerY, double collectionRadius)
{
    if (collected_) {
        return false;
    }

    const double dx = x_ - playerX;
    const double dy = y_ - playerY;
    if (dx * dx + dy * dy >= collectionRadius * collectionRadius) {
        return false;
    }

    collected_ = true;
    return true;
}

bool Coin::isCollected() const { return collected_; }
double Coin::spinPhase() const { return spinPhase_; }

WingPowerup::WingPowerup(double x, double y, double scale, double flutterPhase)
    : Powerup(x, y, scale)
    , flutterPhase_(flutterPhase)
{
}

void WingPowerup::updateAnimation(double dt)
{
    flutterPhase_ += dt * 8.0;
}

bool WingPowerup::collidesWith(double playerX, double playerY, double playerRadius) const
{
    const double dx = x_ - playerX;
    const double dy = y_ - playerY;
    const double radius = playerRadius + 20.0 * scale_;
    return dx * dx + dy * dy < radius * radius;
}

double WingPowerup::flutterPhase() const { return flutterPhase_; }
