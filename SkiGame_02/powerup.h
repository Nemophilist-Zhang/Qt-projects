#ifndef POWERUP_H
#define POWERUP_H

class Powerup
{
public:
    Powerup(double x = 0, double y = 0, double scale = 1.0);
    virtual ~Powerup() = default;

    double x() const;
    double y() const;
    double scale() const;

    void setPosition(double x, double y);

protected:
    double x_ = 0;
    double y_ = 0;
    double scale_ = 1.0;
};

class Coin : public Powerup
{
public:
    Coin(double x = 0, double y = 0, double spinPhase = 0);

    void updateAnimation(double dt);
    bool tryCollect(double playerX, double playerY, double collectionRadius);

    bool isCollected() const;
    double spinPhase() const;

private:
    bool collected_ = false;
    double spinPhase_ = 0;
};

class WingPowerup : public Powerup
{
public:
    WingPowerup(double x = 0, double y = 0, double scale = 1.0, double flutterPhase = 0);

    void updateAnimation(double dt);
    bool collidesWith(double playerX, double playerY, double playerRadius) const;

    double flutterPhase() const;

private:
    double flutterPhase_ = 0;
};

#endif // POWERUP_H
