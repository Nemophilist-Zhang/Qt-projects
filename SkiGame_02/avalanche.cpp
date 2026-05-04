#include "avalanche.h"

#include <algorithm>

void Avalanche::reset()
{
    *this = {};
}

void Avalanche::spawn(double startFrontX, std::mt19937 &rng)
{
    active_ = true;
    elapsed_ = 0.0;
    frontX_ = startFrontX;

    std::uniform_real_distribution<double> u01(0.0, 1.0);
    targetGap_ = 135.0 + u01(rng) * 55.0;
    pressure_ = 1.02 + u01(rng) * 0.12;
}

void Avalanche::update(double dt, double playerX, double playerBaseSpeed)
{
    if (!active_) {
        return;
    }

    elapsed_ += dt;
    const double gap = std::max(0.0, playerX - frontX_);
    double surgeFactor = 0.78;
    if (elapsed_ < 3.2 && gap > targetGap_ + 40.0) {
        surgeFactor = 1.10 + 0.10 * std::clamp((gap - targetGap_ - 40.0) / 220.0, 0.0, 1.0);
    } else if (elapsed_ < 8.8) {
        if (gap > targetGap_ + 20.0) {
            surgeFactor = 0.98 + 0.08 * std::clamp((gap - targetGap_ - 20.0) / 180.0, 0.0, 1.0);
        } else if (gap < targetGap_ - 35.0) {
            surgeFactor = 0.82 - 0.08 * std::clamp((targetGap_ - 35.0 - gap) / 110.0, 0.0, 1.0);
        } else {
            surgeFactor = 0.92;
        }
    } else {
        const double fade = std::clamp((elapsed_ - 8.8) / 3.8, 0.0, 1.0);
        surgeFactor = 0.88 + (0.68 - 0.88) * fade;
    }

    frontX_ += playerBaseSpeed * surgeFactor * pressure_ * dt;
}

bool Avalanche::isActive() const { return active_; }

bool Avalanche::hasCaught(double playerX, double catchDistance) const
{
    return active_ && playerX - frontX_ < catchDistance;
}

bool Avalanche::shouldDespawn(double cameraX) const
{
    return active_ && ((elapsed_ > 10.0 && frontX_ < cameraX - 360.0) || elapsed_ > 13.5);
}

int Avalanche::gapMeters(double playerX) const
{
    return std::max(0, static_cast<int>((playerX - frontX_) / 10.0));
}

double Avalanche::frontX() const { return frontX_; }
double Avalanche::elapsed() const { return elapsed_; }
