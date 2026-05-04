#include "player.h"

#include <algorithm>
#include <cmath>

void Player::reset(double initialY, double initialSpeed)
{
    x_ = 0;
    y_ = initialY;
    vx_ = initialSpeed;
    vy_ = 0;
    angle_ = 0;
    grounded_ = true;
    jumping_ = false;
    airTime_ = 0;
    jumpsAvailable_ = 0;
    holdingPenguin_ = false;
    penguinBoostRemaining_ = 0;
    flying_ = false;
    landingFromFlight_ = false;
    wingFlightRemaining_ = 0;
}

void Player::updatePenguinBoost(double dt)
{
    if (!holdingPenguin_) {
        return;
    }

    penguinBoostRemaining_ = std::max(0.0, penguinBoostRemaining_ - dt);
    if (penguinBoostRemaining_ <= 0.0) {
        holdingPenguin_ = false;
    }
}

bool Player::updateWingFlight(double dt)
{
    if (!flying_) {
        return false;
    }

    wingFlightRemaining_ = std::max(0.0, wingFlightRemaining_ - dt);
    if (wingFlightRemaining_ <= 0.0) {
        flying_ = false;
        return true;
    }
    return false;
}

void Player::pickupPenguin(double duration, double speedMultiplier)
{
    holdingPenguin_ = true;
    penguinBoostRemaining_ = duration;
    vx_ *= speedMultiplier;
}

void Player::pickupWings(double duration, double liftVelocity)
{
    flying_ = true;
    landingFromFlight_ = false;
    wingFlightRemaining_ = duration;
    jumping_ = false;
    grounded_ = false;
    jumpsAvailable_ = 0;
    vy_ = liftVelocity;
}

bool Player::tryJump(double jumpVelocity)
{
    if (flying_ || landingFromFlight_ || jumpsAvailable_ <= 0) {
        return false;
    }

    jumping_ = true;
    grounded_ = false;
    vy_ = jumpVelocity;
    --jumpsAvailable_;
    airTime_ = 0;
    return true;
}

void Player::beginFlightLanding(double fallVelocity)
{
    landingFromFlight_ = true;
    jumping_ = true;
    grounded_ = false;
    vy_ = fallVelocity;
}

void Player::land()
{
    jumping_ = false;
    grounded_ = true;
    jumpsAvailable_ = 1;
    landingFromFlight_ = false;
    airTime_ = 0;
}

double Player::baseSpeed(double baseSpeed, double speedRamp, double maxSpeed) const
{
    return std::min(maxSpeed, baseSpeed + x_ * speedRamp);
}

double Player::speedFactor(double slope, bool airborne) const
{
    const double terrainFactor = 1.0 + std::clamp(slope * 1.15, -0.12, 0.18);
    return airborne ? terrainFactor * 0.86 : terrainFactor;
}

double Player::speedMultiplier(double penguinMultiplier) const
{
    return holdingPenguin_ ? penguinMultiplier : 1.0;
}

double Player::x() const { return x_; }
double Player::y() const { return y_; }
double Player::vx() const { return vx_; }
double Player::vy() const { return vy_; }
double Player::angle() const { return angle_; }
double Player::airTime() const { return airTime_; }
double Player::penguinBoostRemaining() const { return penguinBoostRemaining_; }
double Player::wingFlightRemaining() const { return wingFlightRemaining_; }
bool Player::isGrounded() const { return grounded_; }
bool Player::isJumping() const { return jumping_; }
bool Player::isFlying() const { return flying_; }
bool Player::isLandingFromFlight() const { return landingFromFlight_; }
bool Player::isHoldingPenguin() const { return holdingPenguin_; }
int Player::jumpsAvailable() const { return jumpsAvailable_; }

void Player::setX(double value) { x_ = value; }
void Player::setY(double value) { y_ = value; }
void Player::setVx(double value) { vx_ = value; }
void Player::setVy(double value) { vy_ = value; }
void Player::setAngle(double value) { angle_ = value; }
void Player::setGrounded(bool value) { grounded_ = value; }
void Player::setJumping(bool value) { jumping_ = value; }
void Player::setFlying(bool value) { flying_ = value; }
void Player::setLandingFromFlight(bool value) { landingFromFlight_ = value; }
void Player::setAirTime(double value) { airTime_ = value; }
void Player::setJumpsAvailable(int value) { jumpsAvailable_ = value; }
