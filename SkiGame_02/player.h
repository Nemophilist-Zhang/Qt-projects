#ifndef PLAYER_H
#define PLAYER_H

class Player
{
public:
    void reset(double initialY, double initialSpeed);

    void updatePenguinBoost(double dt);
    bool updateWingFlight(double dt);

    void pickupPenguin(double duration, double speedMultiplier);
    void pickupWings(double duration, double liftVelocity);

    bool tryJump(double jumpVelocity);
    void beginFlightLanding(double fallVelocity);
    void land();

    double baseSpeed(double baseSpeed, double speedRamp, double maxSpeed) const;
    double speedFactor(double slope, bool airborne) const;
    double speedMultiplier(double penguinMultiplier) const;

    double x() const;
    double y() const;
    double vx() const;
    double vy() const;
    double angle() const;
    double airTime() const;
    double penguinBoostRemaining() const;
    double wingFlightRemaining() const;
    bool isGrounded() const;
    bool isJumping() const;
    bool isFlying() const;
    bool isLandingFromFlight() const;
    bool isHoldingPenguin() const;
    int jumpsAvailable() const;

    void setX(double value);
    void setY(double value);
    void setVx(double value);
    void setVy(double value);
    void setAngle(double value);
    void setGrounded(bool value);
    void setJumping(bool value);
    void setFlying(bool value);
    void setLandingFromFlight(bool value);
    void setAirTime(double value);
    void setJumpsAvailable(int value);

private:
    double x_ = 0;
    double y_ = 0;
    double vx_ = 0;
    double vy_ = 0;
    double angle_ = 0;
    bool grounded_ = false;
    bool jumping_ = false;
    double airTime_ = 0;
    int jumpsAvailable_ = 0;
    bool holdingPenguin_ = false;
    double penguinBoostRemaining_ = 0;
    bool flying_ = false;
    bool landingFromFlight_ = false;
    double wingFlightRemaining_ = 0;
};

#endif // PLAYER_H
