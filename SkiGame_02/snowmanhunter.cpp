#include "snowmanhunter.h"

#include <algorithm>

void SnowmanHunter::reset()
{
    *this = {};
}

void SnowmanHunter::spawn(double startX, double startY, std::mt19937 &rng)
{
    active_ = true;
    elapsed_ = 0.0;
    scale_ = 1.05;
    x_ = startX;
    y_ = startY;

    std::uniform_real_distribution<double> u01(0.0, 1.0);
    lethalChase_ = u01(rng) < 0.42;
    if (lethalChase_) {
        targetGap_ = 125.0 + u01(rng) * 25.0;
        aggression_ = 1.10 + u01(rng) * 0.08;
    } else {
        targetGap_ = 205.0 + u01(rng) * 40.0;
        aggression_ = 0.98 + u01(rng) * 0.06;
    }
}

void SnowmanHunter::update(double dt, double playerX, double playerBaseSpeed, double /*cameraX*/,
                           const std::function<double(double)> &slopeY)
{
    if (!active_) {
        return;
    }

    elapsed_ += dt;
    const double gap = std::max(0.0, playerX - x_);
    const double minDangerGap = targetGap_ - (lethalChase_ ? 38.0 : 52.0);
    const double maxDangerGap = targetGap_ + (lethalChase_ ? 48.0 : 72.0);
    double chaseFactor = lethalChase_ ? 0.92 : 0.84;

    if (elapsed_ < 4.5 && gap > maxDangerGap) {
        const double catchupBoost = std::clamp((gap - maxDangerGap) / 230.0, 0.0, 1.0);
        chaseFactor = lethalChase_
            ? 1.22 + 0.16 * catchupBoost
            : 1.10 + 0.08 * catchupBoost;
    } else if (elapsed_ < (lethalChase_ ? 10.0 : 9.5)) {
        if (gap > maxDangerGap) {
            const double pressureBoost = std::clamp((gap - maxDangerGap) / 190.0, 0.0, 1.0);
            chaseFactor = lethalChase_
                ? 1.10 + 0.14 * pressureBoost
                : 1.02 + 0.08 * pressureBoost;
        } else if (gap < minDangerGap) {
            const double easeOff = std::clamp((minDangerGap - gap) / 100.0, 0.0, 1.0);
            chaseFactor = lethalChase_
                ? 0.97 - 0.08 * easeOff
                : 0.87 - 0.10 * easeOff;
        } else {
            chaseFactor = lethalChase_ ? 1.05 : 0.98;
        }
    } else {
        chaseFactor = lethalChase_
            ? std::clamp(1.00 - (elapsed_ - 10.0) / 23.75, 0.84, 1.00)
            : std::clamp(0.96 - (elapsed_ - 9.5) / 21.875, 0.80, 0.96);
    }

    x_ += playerBaseSpeed * chaseFactor * aggression_ * dt;
    y_ = slopeY(x_) - 28.0 * scale_;
}

bool SnowmanHunter::isActive() const { return active_; }
bool SnowmanHunter::isLethalChase() const { return lethalChase_; }

bool SnowmanHunter::hasCaught(double playerX, double playerY, double catchDistance) const
{
    if (!active_) {
        return false;
    }

    const double dx = playerX - x_;
    const double dy = playerY - y_;
    return dx * dx + dy * dy < catchDistance * catchDistance;
}

bool SnowmanHunter::shouldDespawn(double cameraX) const
{
    return active_ && ((elapsed_ > 10.5 && x_ < cameraX - 360.0) || elapsed_ > 15.0);
}

int SnowmanHunter::gapMeters(double playerX) const
{
    return std::max(0, static_cast<int>((playerX - x_) / 10.0));
}

double SnowmanHunter::x() const { return x_; }
double SnowmanHunter::y() const { return y_; }
double SnowmanHunter::scale() const { return scale_; }
