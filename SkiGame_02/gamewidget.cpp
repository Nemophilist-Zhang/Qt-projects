#include "gamewidget.h"

#include <QAudioOutput>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QFont>
#include <QFontMetrics>
#include <QLinearGradient>
#include <QMediaPlayer>
#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>
#include <QStringList>
#include <QUrl>
#include <QtMath>

#include <algorithm>
#include <chrono>
#include <cmath>

namespace {

inline double lerp(double a, double b, double t) { return a + (b - a) * t; }

inline QColor mix(const QColor &a, const QColor &b, double t) {
    return QColor::fromRgbF(
        lerp(a.redF(),   b.redF(),   t),
        lerp(a.greenF(), b.greenF(), t),
        lerp(a.blueF(),  b.blueF(),  t),
        lerp(a.alphaF(), b.alphaF(), t));
}

} // namespace

GameWidget::GameWidget(QWidget *parent)
    : QWidget(parent)
    , rng(static_cast<uint32_t>(std::chrono::steady_clock::now().time_since_epoch().count()))
{
    setMinimumSize(800, 500);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setAttribute(Qt::WA_OpaquePaintEvent, true);

    // Pre-generate stars in screen-space
    std::uniform_real_distribution<double> ux(0.0, 1.0);
    std::uniform_real_distribution<double> uy(0.0, 0.6);
    std::uniform_real_distribution<double> ub(0.3, 1.0);
    for (int i = 0; i < 70; ++i) {
        stars.push_back({ ux(rng), uy(rng), ub(rng) });
    }
    std::uniform_real_distribution<double> uc(0.0, 1.0);
    for (int i = 0; i < 8; ++i) {
        clouds.push_back({ uc(rng) * 2000.0, 60 + uc(rng) * 90.0,
                           0.6 + uc(rng) * 1.4, 8 + uc(rng) * 12 });
    }

    setupAudio();

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &GameWidget::tick);
    elapsed.start();
    lastTickMs = elapsed.elapsed();
    timer->start(16); // ~60 fps

    resetGame();
    state = State::Menu;
    updateAudioState();
}

void GameWidget::resetGame()
{
    player.reset(slopeY(0) - kPlayerRadius, kBaseSpeed);
    rotationInAir = 0;

    cameraX = 0;
    cameraY = 0;

    score = 0;
    coins = 0;
    distance = 0;

    nextCoinX = 400;
    nextWingX = 900;
    nextPenguinX = 1100;
    nextSnowmanSpawnX = 3200;
    nextRockX = 800;
    nextTreeNearX = 200;
    nextTreeMidX = 0;
    nextTreeFarX = 0;

    coinList.clear();
    wingList.clear();
    penguinList.clear();
    rockList.clear();
    treeList.clear();
    particles.clear();
    snowmanHunter.reset();
    avalanche.reset();
}

void GameWidget::startGame()
{
    resetGame();
    state = State::Playing;
    updateAudioState();
}

void GameWidget::endGame()
{
    if (score > bestScore) bestScore = score;
    state = State::GameOver;
    updateAudioState();
}

void GameWidget::pickupPenguin()
{
    player.pickupPenguin(kPenguinBoostDuration, kPenguinSpeedMultiplier);
    penguinList.clear();

    std::uniform_real_distribution<double> u(-1.0, 1.0);
    for (int i = 0; i < 16; ++i) {
        particles.push_back({
            player.x() + u(rng) * 14.0,
            player.y() - 10.0 + u(rng) * 10.0,
            u(rng) * 170.0,
            -120.0 + u(rng) * 110.0,
            0.7, 0.7,
            3.0 + std::abs(u(rng)) * 2.0,
            QColor(110, 230, 255, 230)
        });
    }
}

void GameWidget::pickupWings()
{
    player.pickupWings(kWingFlightDuration, -220.0);
    wingList.clear();

    std::uniform_real_distribution<double> u(-1.0, 1.0);
    for (int i = 0; i < 18; ++i) {
        particles.push_back({
            player.x() + u(rng) * 16.0,
            player.y() - 8.0 + u(rng) * 12.0,
            u(rng) * 160.0,
            -150.0 + u(rng) * 120.0,
            0.75, 0.75,
            3.5 + std::abs(u(rng)) * 2.0,
            QColor(255, 245, 210, 230)
        });
    }
}

void GameWidget::spawnSnowmanHunter()
{
    const double startX = cameraX - kPlayerScreenX - 180.0;
    snowmanHunter.spawn(startX, slopeY(startX) - 28.0, rng);

    std::uniform_real_distribution<double> u(-1.0, 1.0);
    for (int i = 0; i < 14; ++i) {
        particles.push_back({
            snowmanHunter.x() + u(rng) * 18.0,
            snowmanHunter.y() + 8.0 + u(rng) * 12.0,
            u(rng) * 70.0,
            -25.0 + u(rng) * 30.0,
            0.6, 0.6,
            2.4 + std::abs(u(rng)) * 2.2,
            QColor(250, 250, 255, 220)
        });
    }
}

void GameWidget::spawnAvalanche()
{
    avalanche.spawn(cameraX - kPlayerScreenX - 220.0, rng);

    std::uniform_real_distribution<double> u(-1.0, 1.0);
    std::uniform_real_distribution<double> u01(0.0, 1.0);
    for (int i = 0; i < 22; ++i) {
        particles.push_back({
            avalanche.frontX() + u(rng) * 24.0,
            slopeY(avalanche.frontX()) - 8.0 + u(rng) * 18.0,
            20.0 + u01(rng) * 90.0,
            -60.0 + u(rng) * 60.0,
            0.75, 0.75,
            3.0 + u01(rng) * 3.0,
            QColor(248, 248, 252, 225)
        });
    }
}

QString GameWidget::assetPath(const QString &fileName) const
{
    QStringList searchRoots;
#ifdef SKIING_ASSET_DIR
    searchRoots.push_back(QString::fromUtf8(SKIING_ASSET_DIR));
#endif
    searchRoots.push_back(QCoreApplication::applicationDirPath());
    searchRoots.push_back(QDir::currentPath());

    QDir parentDir(QCoreApplication::applicationDirPath());
    for (int i = 0; i < 4; ++i) {
        if (!parentDir.cdUp()) {
            break;
        }
        searchRoots.push_back(parentDir.absolutePath());
    }

    for (const QString &root : searchRoots) {
        const QString candidate = QDir(root).absoluteFilePath(fileName);
        if (QFileInfo::exists(candidate)) {
            return candidate;
        }
    }

    return QDir(QCoreApplication::applicationDirPath()).absoluteFilePath(fileName);
}

QMediaPlayer *GameWidget::createAudioPlayer(const QString &fileName, float volume, int loops)
{
    auto *output = new QAudioOutput(this);
    output->setVolume(volume);

    auto *playerObject = new QMediaPlayer(this);
    playerObject->setAudioOutput(output);
    playerObject->setSource(QUrl::fromLocalFile(assetPath(fileName)));
    playerObject->setLoops(loops);
    return playerObject;
}

void GameWidget::setupAudio()
{
    backgroundMusic = createAudioPlayer(QStringLiteral("background.mp3"), 0.42f, QMediaPlayer::Infinite);
    coinSounds.clear();
    for (int i = 0; i < 6; ++i) {
        coinSounds.push_back(createAudioPlayer(QStringLiteral("coin.mp3"), 0.50f, 1));
    }
    nextCoinSoundIndex = 0;
    flyLoop = createAudioPlayer(QStringLiteral("fly.mp3"), 0.55f, QMediaPlayer::Infinite);
    monsterLoop = createAudioPlayer(QStringLiteral("monster.mp3"), 0.54f, QMediaPlayer::Infinite);
    penguinLoop = createAudioPlayer(QStringLiteral("penguin.mp3"), 0.50f, QMediaPlayer::Infinite);
    skiingLoop = createAudioPlayer(QStringLiteral("skiing.mp3"), 0.38f, QMediaPlayer::Infinite);
    avalancheLoop = createAudioPlayer(QStringLiteral("avalanche.mp3"), 0.60f, QMediaPlayer::Infinite);
}

void GameWidget::playOneShot(QMediaPlayer *playerObject)
{
    if (playerObject == nullptr) {
        return;
    }

    playerObject->stop();
    playerObject->setPosition(0);
    playerObject->play();
}

void GameWidget::playCoinSound()
{
    if (coinSounds.empty()) {
        return;
    }

    QMediaPlayer *playerObject = coinSounds[static_cast<size_t>(nextCoinSoundIndex)];
    nextCoinSoundIndex = (nextCoinSoundIndex + 1) % static_cast<int>(coinSounds.size());

    if (playerObject->playbackState() == QMediaPlayer::PlayingState) {
        playerObject->stop();
    }
    playerObject->setPosition(0);
    playerObject->play();
}

void GameWidget::updateAudioState()
{
    auto syncLoop = [](QMediaPlayer *playerObject, bool shouldPlay, bool &isPlaying) {
        if (playerObject == nullptr) {
            return;
        }

        if (shouldPlay) {
            if (!isPlaying) {
                playerObject->play();
                isPlaying = true;
            }
            return;
        }

        if (isPlaying) {
            playerObject->stop();
            isPlaying = false;
        }
    };

    const bool inMenu = state == State::Menu;
    const bool onSlope = state == State::Playing
        && player.isGrounded()
        && !player.isJumping()
        && !player.isFlying()
        && !player.isLandingFromFlight();
    const bool penguinActive = state == State::Playing && player.isHoldingPenguin();
    const bool flyActive = state == State::Playing && (player.isFlying() || player.isLandingFromFlight());
    const bool snowmanActive = state == State::Playing && snowmanHunter.isActive();
    const bool avalancheActive = state == State::Playing && avalanche.isActive();

    syncLoop(backgroundMusic, inMenu, backgroundMusicPlaying);
    syncLoop(skiingLoop, onSlope, skiingLoopPlaying);
    syncLoop(penguinLoop, penguinActive, penguinLoopPlaying);
    syncLoop(flyLoop, flyActive, flyLoopPlaying);
    syncLoop(monsterLoop, snowmanActive, monsterLoopPlaying);
    syncLoop(avalancheLoop, avalancheActive, avalancheLoopPlaying);
}

double GameWidget::horizonY() const
{
    return height() * 0.62;
}

double GameWidget::playerBaseSpeed() const
{
    return player.baseSpeed(kBaseSpeed, kSpeedRamp, kMaxSpeed);
}

double GameWidget::playerSpeedFactor(double slope, bool airborne) const
{
    return player.speedFactor(slope, airborne);
}

double GameWidget::speedMultiplier() const
{
    return player.speedMultiplier(kPenguinSpeedMultiplier);
}

double GameWidget::slopeY(double x) const
{
    const double base = horizonY() + 60.0;
    return base
        + std::sin(x * 0.0040) * 70.0
        + std::sin(x * 0.0110 + 1.3) * 32.0
        + std::sin(x * 0.0023 + 2.7) * 50.0
        + std::sin(x * 0.0007) * 20.0;
}

double GameWidget::slopeAngle(double x) const
{
    const double dx = 1.0;
    const double y1 = slopeY(x - dx);
    const double y2 = slopeY(x + dx);
    return std::atan2(y2 - y1, 2.0 * dx);
}

QPointF GameWidget::worldToScreen(double wx, double wy) const
{
    return QPointF(wx - cameraX + kPlayerScreenX, wy - cameraY);
}

void GameWidget::tick()
{
    qint64 now = elapsed.elapsed();
    double dt = (now - lastTickMs) / 1000.0;
    lastTickMs = now;
    dt = std::clamp(dt, 0.001, 0.05);

    timeAccum += dt;
    menuPulse += dt;

    if (state == State::Playing) {
        updatePhysics(dt);
        spawnContent();
    } else {
        // gentle camera drift on menu so background looks alive
        cameraX += 30.0 * dt;
    }

    // cloud drift
    for (auto &c : clouds) {
        c.x -= c.speed * dt;
    }

    updateAudioState();
    update();
}

void GameWidget::updatePhysics(double dt)
{
    player.updatePenguinBoost(dt);

    const double initialSlope = slopeAngle(player.x());
    const double boostedBaseSpeed = playerBaseSpeed() * speedMultiplier();
    const double airborneSpeed = boostedBaseSpeed * playerSpeedFactor(initialSlope, true);
    const double prevPlayerY = player.y();

    player.setX(player.x() + player.vx() * dt);

    if (player.isFlying()) {
        player.setGrounded(false);
        player.setJumping(false);
        player.setJumpsAvailable(0);
        const bool flightEnded = player.updateWingFlight(dt);

        player.setVx(lerp(player.vx(), boostedBaseSpeed * 0.98, std::min(1.0, dt * 3.0)));
        const double hover = std::sin(timeAccum * 7.0) * 7.0;
        const double targetY = slopeY(player.x()) - kPlayerRadius - kWingFlightHeight + hover;
        player.setY(lerp(player.y(), targetY, std::min(1.0, dt * 5.0)));
        player.setVy((player.y() - prevPlayerY) / std::max(dt, 0.001));
        player.setAngle(lerp(player.angle(), -0.18 + std::sin(timeAccum * 5.0) * 0.05,
                             std::min(1.0, dt * 5.0)));

        if (flightEnded) {
            player.beginFlightLanding(120.0);
        }
    } else if (player.isJumping()) {
        player.setAirTime(player.airTime() + dt);
        player.setVy(player.vy() + kGravity * dt);
        player.setVx(lerp(player.vx(), airborneSpeed, std::min(1.0, dt * 2.5)));
        player.setY(player.y() + player.vy() * dt);
        rotationInAir += player.vx() * dt * 0.0;

        const double sy = slopeY(player.x());
        if (player.y() + kPlayerRadius >= sy && player.vy() >= 0) {
            player.setY(sy - kPlayerRadius);
            player.land();

            std::uniform_real_distribution<double> u(-1.0, 1.0);
            for (int i = 0; i < 18; ++i) {
                particles.push_back({
                    player.x() + u(rng) * 10,
                    player.y() + kPlayerRadius - 2,
                    u(rng) * 90.0,
                    -40 - std::abs(u(rng)) * 80.0,
                    0.55, 0.55,
                    2.5 + std::abs(u(rng)) * 2.5,
                    QColor(255, 255, 255, 220)
                });
            }

            const double a = slopeAngle(player.x());
            const double landingSpeed = playerBaseSpeed() * speedMultiplier() * playerSpeedFactor(a, false);
            player.setVx(std::cos(a) * landingSpeed);
            player.setVy(std::sin(a) * landingSpeed);
        }

        const double targetAngle = std::atan2(player.vy(), std::max(player.vx(), 1.0));
        player.setAngle(lerp(player.angle(), targetAngle, std::min(1.0, dt * 6.0)));
    } else {
        player.setGrounded(true);
        player.setJumpsAvailable(1);
        player.setY(slopeY(player.x()) - kPlayerRadius);
        const double a = slopeAngle(player.x());
        const double terrainSpeed = playerBaseSpeed() * speedMultiplier() * playerSpeedFactor(a, false);
        player.setVx(std::cos(a) * terrainSpeed);
        player.setVy(std::sin(a) * terrainSpeed);
        player.setAngle(a);

        if ((int)(timeAccum * 60) % 2 == 0) {
            std::uniform_real_distribution<double> u(-1.0, 1.0);
            particles.push_back({
                player.x() - 8,
                player.y() + kPlayerRadius - 1,
                -player.vx() * 0.15 + u(rng) * 30,
                -30 + u(rng) * 25,
                0.5, 0.5,
                2.0 + std::abs(u(rng)) * 2.0,
                QColor(255, 255, 255, 200)
            });
        }
    }

    if (snowmanHunter.isActive()) {
        snowmanHunter.update(dt, player.x(), playerBaseSpeed(), cameraX,
                             [this](double x) { return slopeY(x); });
        if (snowmanHunter.hasCaught(player.x(), player.y(), kSnowmanCatchDistance)) {
            std::uniform_real_distribution<double> u(-1.0, 1.0);
            for (int i = 0; i < 26; ++i) {
                particles.push_back({
                    player.x() + u(rng) * 10.0,
                    player.y() + u(rng) * 12.0,
                    u(rng) * 220.0,
                    -40.0 + u(rng) * 180.0,
                    0.7, 0.7,
                    2.0 + std::abs(u(rng)) * 3.0,
                    QColor(245, 245, 250, 230)
                });
            }
            endGame();
            return;
        }

        if (snowmanHunter.shouldDespawn(cameraX)) {
            snowmanHunter.reset();
        }
    }

    if (avalanche.isActive()) {
        avalanche.update(dt, player.x(), playerBaseSpeed());
        if (avalanche.hasCaught(player.x(), 18.0)) {
            std::uniform_real_distribution<double> u(-1.0, 1.0);
            for (int i = 0; i < 34; ++i) {
                particles.push_back({
                    player.x() + u(rng) * 18.0,
                    player.y() + u(rng) * 18.0,
                    u(rng) * 240.0,
                    -30.0 + u(rng) * 200.0,
                    0.9, 0.9,
                    2.4 + std::abs(u(rng)) * 3.2,
                    QColor(250, 250, 255, 235)
                });
            }
            endGame();
            return;
        }

        if (avalanche.shouldDespawn(cameraX)) {
            avalanche.reset();
        }
    }

    double targetCamX = player.x() - kPlayerScreenX;
    double targetCamY = player.y() - height() * 0.55;
    cameraX = lerp(cameraX, targetCamX, std::min(1.0, dt * 8.0));
    cameraY = lerp(cameraY, targetCamY, std::min(1.0, dt * 4.0));

    // Update particles
    for (auto &pa : particles) {
        pa.life -= dt;
        pa.x += pa.vx * dt;
        pa.y += pa.vy * dt;
        pa.vy += 250 * dt;
        pa.vx *= std::pow(0.2, dt);
    }
    particles.erase(std::remove_if(particles.begin(), particles.end(),
        [](const Particle &p){ return p.life <= 0; }), particles.end());

    // Coin collection
    for (auto &c : coinList) {
        c.updateAnimation(dt);
        if (c.tryCollect(player.x(), player.y(), kPlayerRadius + 14)) {
            coins += 1;
            score += 10;
            playCoinSound();
            std::uniform_real_distribution<double> u(-1.0, 1.0);
            for (int i = 0; i < 10; ++i) {
                particles.push_back({
                    c.x(), c.y(),
                    u(rng) * 120, u(rng) * 120 - 60,
                    0.4, 0.4,
                    2.0 + std::abs(u(rng)) * 2.0,
                    QColor(255, 215, 80, 230)
                });
            }
        }
    }

    for (auto &wing : wingList) {
        wing.updateAnimation(dt);
        if (wing.collidesWith(player.x(), player.y(), kPlayerRadius)) {
            pickupWings();
            break;
        }
    }

    if (!player.isHoldingPenguin()) {
        for (auto &penguin : penguinList) {
            penguin.updateAnimation(dt);
            if (penguin.collidesWith(player.x(), player.y(), kPlayerRadius)) {
                pickupPenguin();
                break;
            }
        }
    }

    if (!player.isFlying() && !player.isLandingFromFlight()) {
        for (auto &r : rockList) {
            if (r.collidesWith(player.x(), player.y(), kPlayerRadius)) {
                std::uniform_real_distribution<double> u(-1.0, 1.0);
                for (int i = 0; i < 30; ++i) {
                    particles.push_back({
                        player.x(), player.y(),
                        u(rng) * 250, -50 + u(rng) * 200,
                        0.8, 0.8,
                        2.0 + std::abs(u(rng)) * 3.0,
                        QColor(240, 240, 240, 230)
                    });
                }
                endGame();
                return;
            }
        }
    }

    distance = static_cast<int>(player.x() / 10.0);
    score = static_cast<int>(player.x() / 10.0) + coins * 10;

    const double cull = cameraX - 200;
    auto removeBefore = [&](auto &vec, auto getX){
        vec.erase(std::remove_if(vec.begin(), vec.end(),
            [&](const auto &it){ return getX(it) < cull; }), vec.end());
    };
    removeBefore(coinList, [](const Coin &c){ return c.x(); });
    removeBefore(wingList, [](const WingPowerup &wing){ return wing.x(); });
    removeBefore(penguinList, [](const PenguinPowerup &penguin){ return penguin.x(); });
    removeBefore(rockList, [](const RockObstacle &r){ return r.x(); });
    removeBefore(treeList, [](const Tree &t){ return t.x; });
}

void GameWidget::spawnContent()
{
    const double aheadX = cameraX + width() + 200;

    std::uniform_real_distribution<double> u(0.0, 1.0);

    if (!snowmanHunter.isActive() && !avalanche.isActive() && player.x() >= nextSnowmanSpawnX) {
        if (u(rng) < 0.5) spawnSnowmanHunter();
        else spawnAvalanche();
        nextSnowmanSpawnX = player.x() + 5200.0 + u(rng) * 4200.0;
    }

    // Coins (groups, tracing slope or arches for jumps)
    while (nextCoinX < aheadX) {
        // skip placement if too close to a rock
        bool nearRock = false;
        for (auto &r : rockList) {
            if (std::abs(r.x() - nextCoinX) < 80) { nearRock = true; break; }
        }
        double r = u(rng);
        int groupSize = 4 + static_cast<int>(u(rng) * 6);
        if (!nearRock) {
            for (int i = 0; i < groupSize; ++i) {
                double x = nextCoinX + i * 28.0;
                double baseY = slopeY(x) - 50;
                double arch = 0;
                if (r > 0.55) {
                    // arch shape over a jump area
                    double t = static_cast<double>(i) / std::max(1, groupSize - 1);
                    arch = -std::sin(t * M_PI) * 80;
                }
                coinList.emplace_back(x, baseY + arch, u(rng) * 6.28);
            }
        }
        nextCoinX += 280 + u(rng) * 320;
    }

    while (nextWingX < aheadX) {
        bool nearObstacle = false;
            for (const auto &r : rockList) {
            if (std::abs(r.x() - nextWingX) < 120) { nearObstacle = true; break; }
        }
        if (!nearObstacle) {
            for (const auto &penguin : penguinList) {
                if (std::abs(penguin.x() - nextWingX) < 140) { nearObstacle = true; break; }
            }
        }
        if (!nearObstacle) {
            double x = nextWingX;
            double scale = 0.95 + u(rng) * 0.2;
            wingList.emplace_back(x, slopeY(x) - 46.0 - u(rng) * 12.0, scale, u(rng) * 6.28);
        }
        nextWingX += 1700 + u(rng) * 1800;
    }

    if (player.isHoldingPenguin()) {
        nextPenguinX = std::max(nextPenguinX, aheadX + 700 + u(rng) * 900);
    } else {
        while (nextPenguinX < aheadX) {
            bool nearHazard = false;
            for (const auto &r : rockList) {
                if (std::abs(r.x() - nextPenguinX) < 120) { nearHazard = true; break; }
            }
            if (!nearHazard) {
                for (const auto &c : coinList) {
                    if (std::abs(c.x() - nextPenguinX) < 90) { nearHazard = true; break; }
                }
            }
            if (!nearHazard) {
                double x = nextPenguinX;
                double scale = 0.9 + u(rng) * 0.25;
                penguinList.emplace_back(x, slopeY(x) - 22.0 * scale, scale, u(rng) * 6.28);
            }
            nextPenguinX += 1200 + u(rng) * 1500;
        }
    }

    // Rocks
    while (nextRockX < aheadX) {
        // not too close to coins
        bool nearCoin = false;
        for (auto &c : coinList) {
            if (std::abs(c.x() - nextRockX) < 60) { nearCoin = true; break; }
        }
        if (!nearCoin && u(rng) > 0.15) {
            double size = 22 + u(rng) * 16;
            rockList.emplace_back(nextRockX, slopeY(nextRockX) - size * 0.45, size);
        }
        nextRockX += 360 + u(rng) * 380;
    }

    // Trees in 3 layers
    while (nextTreeNearX < aheadX) {
        treeList.push_back({ nextTreeNearX, 0.9 + u(rng) * 0.5, 2 });
        nextTreeNearX += 90 + u(rng) * 70;
    }
    while (nextTreeMidX < aheadX) {
        treeList.push_back({ nextTreeMidX, 0.55 + u(rng) * 0.25, 1 });
        nextTreeMidX += 130 + u(rng) * 90;
    }
    while (nextTreeFarX < aheadX) {
        treeList.push_back({ nextTreeFarX, 0.3 + u(rng) * 0.15, 0 });
        nextTreeFarX += 200 + u(rng) * 120;
    }
}

void GameWidget::tryJump()
{
    if (state == State::Menu) {
        startGame();
        return;
    }
    if (state == State::GameOver) {
        startGame();
        return;
    }
    if (player.tryJump(kJumpVel)) {
        // jump puff
        std::uniform_real_distribution<double> u(-1.0, 1.0);
        for (int i = 0; i < 12; ++i) {
            particles.push_back({
                player.x() + u(rng) * 8,
                player.y() + kPlayerRadius - 2,
                u(rng) * 70.0,
                -20 - std::abs(u(rng)) * 60.0,
                0.45, 0.45,
                2.0 + std::abs(u(rng)) * 2.0,
                QColor(255, 255, 255, 220)
            });
        }
    }
}

void GameWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->isAutoRepeat()) { QWidget::keyPressEvent(event); return; }
    switch (event->key()) {
    case Qt::Key_Space:
    case Qt::Key_Up:
    case Qt::Key_W:
        tryJump();
        break;
    case Qt::Key_R:
        if (state == State::GameOver) startGame();
        break;
    case Qt::Key_Escape:
        if (state == State::Playing) endGame();
        break;
    default:
        QWidget::keyPressEvent(event);
    }
}

void GameWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) tryJump();
}

void GameWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
}

// ----- Drawing -----

void GameWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);

    drawSky(p);
    drawStars(p);
    drawSun(p);
    drawClouds(p);
    drawMountains(p);
    drawTrees(p);
    drawSlope(p);
    drawCoins(p);
    drawWingPowerups(p);
    drawPenguins(p);
    drawSnowmanHunter(p);
    drawAvalanche(p);
    drawRocks(p);
    drawPlayer(p);
    drawParticles(p);

    if (state == State::Playing) drawHUD(p);
    if (state == State::Menu)    drawMenuOverlay(p);
    if (state == State::GameOver) drawGameOverOverlay(p);
}

void GameWidget::drawSky(QPainter &p)
{
    QLinearGradient g(0, 0, 0, height());
    g.setColorAt(0.00, QColor("#1b1140"));
    g.setColorAt(0.35, QColor("#5b3585"));
    g.setColorAt(0.62, QColor("#d96e7a"));
    g.setColorAt(0.78, QColor("#ffa15c"));
    g.setColorAt(1.00, QColor("#ffd28a"));
    p.fillRect(rect(), g);
}

void GameWidget::drawStars(QPainter &p)
{
    p.setPen(Qt::NoPen);
    for (const auto &s : stars) {
        double tw = 0.6 + 0.4 * std::sin(timeAccum * 2.0 + s.x * 30.0);
        QColor c(255, 255, 255, static_cast<int>(180 * s.brightness * tw));
        p.setBrush(c);
        double sx = s.x * width();
        double sy = s.y * height();
        p.drawEllipse(QPointF(sx, sy), 1.4, 1.4);
    }
}

void GameWidget::drawSun(QPainter &p)
{
    // Sun parallax: very slow horizontal scroll
    double sunX = width() * 0.78 - cameraX * 0.02;
    // wrap-ish to keep on screen
    sunX = std::fmod(sunX, width() * 1.0);
    if (sunX < 0) sunX += width();
    double sunY = horizonY() - 110;
    double r = 46;

    // outer halo
    QRadialGradient halo(sunX, sunY, r * 3.5);
    halo.setColorAt(0.0, QColor(255, 220, 140, 130));
    halo.setColorAt(0.4, QColor(255, 180, 120, 60));
    halo.setColorAt(1.0, QColor(255, 160, 100, 0));
    p.setPen(Qt::NoPen);
    p.setBrush(halo);
    p.drawEllipse(QPointF(sunX, sunY), r * 3.5, r * 3.5);

    // sun body
    QRadialGradient body(sunX - r * 0.2, sunY - r * 0.2, r * 1.3);
    body.setColorAt(0.0, QColor(255, 250, 220));
    body.setColorAt(0.5, QColor(255, 220, 130));
    body.setColorAt(1.0, QColor(255, 170, 80));
    p.setBrush(body);
    p.drawEllipse(QPointF(sunX, sunY), r, r);
}

void GameWidget::drawClouds(QPainter &p)
{
    p.setPen(Qt::NoPen);
    for (const auto &c : clouds) {
        double sx = std::fmod(c.x - cameraX * 0.05, width() + 200.0);
        if (sx < -200) sx += width() + 200;
        double sy = c.y;
        QColor col(255, 220, 230, 110);
        p.setBrush(col);
        p.drawEllipse(QPointF(sx, sy), 36 * c.scale, 12 * c.scale);
        p.drawEllipse(QPointF(sx - 22 * c.scale, sy + 4 * c.scale), 24 * c.scale, 9 * c.scale);
        p.drawEllipse(QPointF(sx + 26 * c.scale, sy + 3 * c.scale), 28 * c.scale, 10 * c.scale);
    }
}

void GameWidget::drawMountains(QPainter &p)
{
    // 3 layers of mountains, each scrolling at a different parallax rate
    const double horizon = horizonY();
    struct Layer { double parallax; double height; QColor color; double freq; double phase; };
    Layer layers[] = {
        { 0.10, 130, QColor(80,  60, 110, 230), 0.0030, 0.0 },
        { 0.25, 170, QColor(60,  44, 92,  240), 0.0040, 1.7 },
        { 0.45, 210, QColor(40,  28, 70,  255), 0.0055, 3.3 },
    };

    for (const auto &L : layers) {
        QPainterPath path;
        path.moveTo(-50, height() + 10);
        for (int sx = -50; sx <= width() + 50; sx += 6) {
            double wx = sx + cameraX * L.parallax;
            double y = horizon
                       - L.height * (0.5 + 0.5 * std::sin(wx * L.freq + L.phase))
                       + 40 * std::sin(wx * L.freq * 0.3 + L.phase * 1.3);
            path.lineTo(sx, y);
        }
        path.lineTo(width() + 50, height() + 10);
        path.closeSubpath();
        p.setPen(Qt::NoPen);
        p.setBrush(L.color);
        p.drawPath(path);

        // snow caps on near layer
        if (&L == &layers[2]) {
            p.setBrush(QColor(255, 240, 230, 120));
            for (int sx = -50; sx <= width() + 50; sx += 6) {
                double wx = sx + cameraX * L.parallax;
                double y = horizon
                           - L.height * (0.5 + 0.5 * std::sin(wx * L.freq + L.phase))
                           + 40 * std::sin(wx * L.freq * 0.3 + L.phase * 1.3);
                // detect peaks
                double yL = horizon - L.height * (0.5 + 0.5 * std::sin((wx-6) * L.freq + L.phase))
                           + 40 * std::sin((wx-6) * L.freq * 0.3 + L.phase * 1.3);
                double yR = horizon - L.height * (0.5 + 0.5 * std::sin((wx+6) * L.freq + L.phase))
                           + 40 * std::sin((wx+6) * L.freq * 0.3 + L.phase * 1.3);
                if (y < yL && y < yR) {
                    QPainterPath cap;
                    cap.moveTo(sx - 20, y + 18);
                    cap.lineTo(sx, y - 2);
                    cap.lineTo(sx + 20, y + 18);
                    cap.closeSubpath();
                    p.drawPath(cap);
                }
            }
        }
    }
}

void GameWidget::drawTrees(QPainter &p)
{
    // Draw far/mid in mountains layer space, near after slope (handled below).
    auto drawTree = [&](double sx, double baseY, double scale, const QColor &col){
        QPainterPath tri;
        double h = 50 * scale;
        double w = 18 * scale;
        tri.moveTo(sx, baseY - h);
        tri.lineTo(sx - w, baseY);
        tri.lineTo(sx + w, baseY);
        tri.closeSubpath();
        p.setPen(Qt::NoPen);
        p.setBrush(col);
        p.drawPath(tri);
        // second triangle
        QPainterPath tri2;
        tri2.moveTo(sx, baseY - h * 1.4);
        tri2.lineTo(sx - w * 0.7, baseY - h * 0.4);
        tri2.lineTo(sx + w * 0.7, baseY - h * 0.4);
        tri2.closeSubpath();
        p.drawPath(tri2);
    };

    for (const auto &t : treeList) {
        double parallax = (t.layer == 0) ? 0.35 : (t.layer == 1) ? 0.6 : 1.0;
        double sx = t.x - cameraX * parallax;
        if (t.layer == 2) continue; // near trees drawn after slope
        if (sx < -60 || sx > width() + 60) continue;

        double baseY = horizonY() + (t.layer == 0 ? -30 : 20);
        QColor col = (t.layer == 0) ? QColor(50, 60, 90, 220) : QColor(34, 46, 70, 240);
        drawTree(sx, baseY, t.scale, col);
    }
}

void GameWidget::drawSlope(QPainter &p)
{
    // Build slope path across the full screen width.
    // Convert screen-x range to world-x range using the inverse of worldToScreen.
    double startWx = cameraX - kPlayerScreenX - 20;
    double endWx   = cameraX - kPlayerScreenX + width() + 20;
    int step = 4;

    QPainterPath path;
    path.moveTo(-20, height() + 20);
    for (double wx = startWx; wx <= endWx; wx += step) {
        path.lineTo(worldToScreen(wx, slopeY(wx)));
    }
    path.lineTo(width() + 20, height() + 20);
    path.closeSubpath();

    // Snow gradient: white top with blue/pink shadow
    QLinearGradient g(0, horizonY() - 20, 0, height());
    g.setColorAt(0.0, QColor("#fff7f1"));
    g.setColorAt(0.4, QColor("#f3e2e9"));
    g.setColorAt(1.0, QColor("#9e7da8"));
    p.setPen(Qt::NoPen);
    p.setBrush(g);
    p.drawPath(path);

    // Top highlight stroke (stroke only, no fill — drawPath would otherwise
    // fill the implicit-closed area with the slope gradient still set as brush)
    QPainterPath topLine;
    bool firstLine = true;
    for (double wx = startWx; wx <= endWx; wx += step) {
        QPointF s = worldToScreen(wx, slopeY(wx));
        if (firstLine) { topLine.moveTo(s); firstLine = false; }
        else topLine.lineTo(s);
    }
    QPen highlight(QColor(255, 255, 255, 230), 2.5);
    highlight.setCapStyle(Qt::RoundCap);
    highlight.setJoinStyle(Qt::RoundJoin);
    p.strokePath(topLine, highlight);

    // Near-layer trees (drawn on top of slope so they sit on it)
    for (const auto &t : treeList) {
        if (t.layer != 2) continue;
        QPointF s = worldToScreen(t.x, slopeY(t.x));
        if (s.x() < -60 || s.x() > width() + 60) continue;

        QPainterPath tri;
        double scale = t.scale;
        double h = 56 * scale;
        double w = 18 * scale;
        tri.moveTo(s.x(), s.y() - h);
        tri.lineTo(s.x() - w, s.y() + 4);
        tri.lineTo(s.x() + w, s.y() + 4);
        tri.closeSubpath();
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(28, 38, 58, 255));
        p.drawPath(tri);

        QPainterPath tri2;
        tri2.moveTo(s.x(), s.y() - h * 1.45);
        tri2.lineTo(s.x() - w * 0.75, s.y() - h * 0.4);
        tri2.lineTo(s.x() + w * 0.75, s.y() - h * 0.4);
        tri2.closeSubpath();
        p.drawPath(tri2);

        // tiny snow cap
        p.setBrush(QColor(255, 250, 245, 230));
        QPainterPath cap;
        cap.moveTo(s.x() - 4 * scale, s.y() - h * 1.05);
        cap.lineTo(s.x(), s.y() - h * 1.45);
        cap.lineTo(s.x() + 4 * scale, s.y() - h * 1.05);
        cap.closeSubpath();
        p.drawPath(cap);
    }

    drawStartCabin(p);
}

void GameWidget::drawStartCabin(QPainter &p)
{
    const double cabinX = -165.0;
    const double flagX = -105.0;
    const QPointF cabinBase = worldToScreen(cabinX, slopeY(cabinX));
    const QPointF flagBase = worldToScreen(flagX, slopeY(flagX));
    const QPointF cabinFoot(cabinBase.x(), cabinBase.y() + 14.0);

    if (cabinFoot.x() < -180 || cabinFoot.x() > width() + 180) {
        return;
    }

    p.save();
    p.setPen(Qt::NoPen);

    QPainterPath snowMound;
    snowMound.moveTo(cabinFoot.x() - 52, cabinFoot.y() - 2);
    snowMound.quadTo(cabinFoot.x() - 14, cabinFoot.y() - 22, cabinFoot.x() + 30, cabinFoot.y() - 1);
    snowMound.lineTo(cabinFoot.x() + 46, cabinFoot.y() + 3);
    snowMound.lineTo(cabinFoot.x() - 48, cabinFoot.y() + 5);
    snowMound.closeSubpath();
    p.setBrush(QColor(245, 236, 240, 235));
    p.drawPath(snowMound);

    QRectF cabinBody(cabinFoot.x() - 36, cabinFoot.y() - 46, 76, 46);
    QLinearGradient wood(cabinBody.topLeft(), cabinBody.bottomLeft());
    wood.setColorAt(0.0, QColor("#8f5d3f"));
    wood.setColorAt(1.0, QColor("#5d3727"));
    p.setBrush(wood);
    p.drawRoundedRect(cabinBody, 4, 4);

    p.setBrush(QColor("#4a2b1e"));
    for (int i = 1; i < 4; ++i) {
        p.drawRect(QRectF(cabinBody.left(), cabinBody.top() + i * 10.0, cabinBody.width(), 2.0));
    }

    QPainterPath roof;
    roof.moveTo(cabinFoot.x() - 44, cabinFoot.y() - 44);
    roof.lineTo(cabinFoot.x() - 2, cabinFoot.y() - 78);
    roof.lineTo(cabinFoot.x() + 48, cabinFoot.y() - 38);
    roof.lineTo(cabinFoot.x() + 38, cabinFoot.y() - 26);
    roof.lineTo(cabinFoot.x() - 38, cabinFoot.y() - 32);
    roof.closeSubpath();
    QLinearGradient roofGrad(cabinFoot.x(), cabinFoot.y() - 78, cabinFoot.x(), cabinFoot.y() - 24);
    roofGrad.setColorAt(0.0, QColor("#71453f"));
    roofGrad.setColorAt(1.0, QColor("#4d292a"));
    p.setBrush(roofGrad);
    p.drawPath(roof);

    QPainterPath roofSnow;
    roofSnow.moveTo(cabinFoot.x() - 34, cabinFoot.y() - 48);
    roofSnow.quadTo(cabinFoot.x() - 8, cabinFoot.y() - 72, cabinFoot.x() + 28, cabinFoot.y() - 42);
    roofSnow.lineTo(cabinFoot.x() + 18, cabinFoot.y() - 35);
    roofSnow.quadTo(cabinFoot.x() - 8, cabinFoot.y() - 55, cabinFoot.x() - 28, cabinFoot.y() - 40);
    roofSnow.closeSubpath();
    p.setBrush(QColor(252, 245, 244, 240));
    p.drawPath(roofSnow);

    QRectF door(cabinBase.x() - 12, cabinBase.y() - 36, 18, 24);
    p.setBrush(QColor("#3a221a"));
    p.drawRoundedRect(door, 3, 3);
    p.setBrush(QColor("#f6c36a"));
    p.drawEllipse(QPointF(door.right() - 4, door.center().y()), 1.6, 1.6);

    p.setBrush(QColor("#bfe5ff"));
    p.drawRoundedRect(QRectF(cabinFoot.x() + 12, cabinFoot.y() - 28, 16, 12), 2, 2);
    p.setBrush(QColor(255, 240, 170, 120));
    p.drawRoundedRect(QRectF(cabinFoot.x() + 14, cabinFoot.y() - 26, 12, 8), 2, 2);

    p.setBrush(QColor("#66504f"));
    p.drawRect(QRectF(cabinFoot.x() + 18, cabinFoot.y() - 74, 7, 20));
    p.setBrush(QColor(255, 248, 245, 180));
    p.drawEllipse(QPointF(cabinFoot.x() + 23, cabinFoot.y() - 80), 8, 10);
    p.drawEllipse(QPointF(cabinFoot.x() + 29, cabinFoot.y() - 86), 10, 12);

    if (flagBase.x() > -80 && flagBase.x() < width() + 80) {
        p.setBrush(QColor(0, 0, 0, 40));
        p.drawEllipse(QPointF(flagBase.x(), flagBase.y() + 4), 10, 3);

        p.setBrush(QColor("#745f5f"));
        p.drawRect(QRectF(flagBase.x() - 1.5, flagBase.y() - 38, 3, 40));

        QPainterPath flag;
        flag.moveTo(flagBase.x() + 1, flagBase.y() - 36);
        flag.quadTo(flagBase.x() + 18, flagBase.y() - 31, flagBase.x() + 16, flagBase.y() - 20);
        flag.lineTo(flagBase.x() + 1, flagBase.y() - 16);
        flag.closeSubpath();
        p.setBrush(QColor("#d94a4a"));
        p.drawPath(flag);

        p.setBrush(QColor(255, 245, 245, 220));
        p.drawEllipse(QPointF(flagBase.x() + 8, flagBase.y() - 27), 2.2, 2.2);
    }

    p.restore();
}

void GameWidget::drawCoins(QPainter &p)
{
    p.setPen(Qt::NoPen);
    for (const auto &c : coinList) {
        if (c.isCollected()) continue;
        QPointF s = worldToScreen(c.x(), c.y());
        if (s.x() < -40 || s.x() > width() + 40) continue;

        // shadow / glow
        QRadialGradient glow(s.x(), s.y(), 22);
        glow.setColorAt(0.0, QColor(255, 220, 100, 120));
        glow.setColorAt(1.0, QColor(255, 220, 100, 0));
        p.setBrush(glow);
        p.drawEllipse(s, 22, 22);

        // coin: spinning ellipse simulation
        double wScale = std::abs(std::cos(c.spinPhase()));
        double rx = 11 * wScale + 2.5;
        double ry = 11;
        QLinearGradient gold(s.x(), s.y() - ry, s.x(), s.y() + ry);
        gold.setColorAt(0.0, QColor(255, 240, 160));
        gold.setColorAt(0.5, QColor(255, 200, 60));
        gold.setColorAt(1.0, QColor(200, 130, 30));
        p.setBrush(gold);
        p.drawEllipse(s, rx, ry);

        // inner highlight
        p.setBrush(QColor(255, 250, 200, 200));
        p.drawEllipse(QPointF(s.x() - rx * 0.3, s.y() - ry * 0.4), rx * 0.35, ry * 0.4);
    }
}

void GameWidget::drawWingPowerups(QPainter &p)
{
    p.setPen(Qt::NoPen);
    for (const auto &wing : wingList) {
        QPointF s = worldToScreen(wing.x(), wing.y());
        if (s.x() < -60 || s.x() > width() + 60) continue;

        const double scale = wing.scale();
        const double flutter = 0.65 + 0.35 * std::sin(wing.flutterPhase());
        const double rise = std::sin(wing.flutterPhase() * 0.7) * 3.0;
        const QPointF center(s.x(), s.y() + rise);

        QRadialGradient glow(center, 30.0 * scale);
        glow.setColorAt(0.0, QColor(255, 240, 180, 90));
        glow.setColorAt(1.0, QColor(255, 240, 180, 0));
        p.setBrush(glow);
        p.drawEllipse(center, 28.0 * scale, 24.0 * scale);

        p.setBrush(QColor(255, 250, 235, 235));
        QPainterPath leftWing;
        leftWing.moveTo(center.x() - 2.0 * scale, center.y());
        leftWing.quadTo(center.x() - 18.0 * scale, center.y() - 16.0 * flutter * scale,
                        center.x() - 28.0 * scale, center.y() - 2.0 * scale);
        leftWing.quadTo(center.x() - 18.0 * scale, center.y() + 12.0 * scale,
                        center.x() - 1.0 * scale, center.y() + 4.0 * scale);
        leftWing.closeSubpath();
        p.drawPath(leftWing);

        QPainterPath rightWing;
        rightWing.moveTo(center.x() + 2.0 * scale, center.y());
        rightWing.quadTo(center.x() + 18.0 * scale, center.y() - 16.0 * flutter * scale,
                         center.x() + 28.0 * scale, center.y() - 2.0 * scale);
        rightWing.quadTo(center.x() + 18.0 * scale, center.y() + 12.0 * scale,
                         center.x() + 1.0 * scale, center.y() + 4.0 * scale);
        rightWing.closeSubpath();
        p.drawPath(rightWing);

        p.setBrush(QColor("#ffd27a"));
        p.drawEllipse(center, 5.5 * scale, 5.5 * scale);
    }
}

void GameWidget::drawPenguins(QPainter &p)
{
    p.setPen(Qt::NoPen);
    for (const auto &penguin : penguinList) {
        QPointF s = worldToScreen(penguin.x(), penguin.y());
        if (s.x() < -50 || s.x() > width() + 50) continue;

        const double bob = std::sin(penguin.bobPhase()) * 2.0;
        const double scale = penguin.scale();
        const double bodyW = 18.0 * scale;
        const double bodyH = 24.0 * scale;
        const QPointF bodyCenter(s.x(), s.y() - 6.0 * scale + bob);

        p.setBrush(QColor(0, 0, 0, 45));
        p.drawEllipse(QPointF(s.x(), s.y() + 10.0 * scale), 16.0 * scale, 5.0 * scale);

        QRadialGradient glow(bodyCenter, 30.0 * scale);
        glow.setColorAt(0.0, QColor(130, 230, 255, 80));
        glow.setColorAt(1.0, QColor(130, 230, 255, 0));
        p.setBrush(glow);
        p.drawEllipse(bodyCenter, 28.0 * scale, 28.0 * scale);

        p.setBrush(QColor("#1d2438"));
        p.drawEllipse(bodyCenter, bodyW, bodyH);

        p.setBrush(QColor("#eef6ff"));
        p.drawEllipse(QPointF(bodyCenter.x(), bodyCenter.y() + 3.0 * scale), 10.0 * scale, 13.0 * scale);

        p.setBrush(QColor("#1d2438"));
        p.drawEllipse(QPointF(bodyCenter.x() - 5.0 * scale, bodyCenter.y() - 20.0 * scale), 8.0 * scale, 9.0 * scale);
        p.drawEllipse(QPointF(bodyCenter.x() + 5.0 * scale, bodyCenter.y() - 20.0 * scale), 8.0 * scale, 9.0 * scale);

        p.setBrush(QColor("#ffb347"));
        QPainterPath beak;
        beak.moveTo(bodyCenter.x(), bodyCenter.y() - 16.0 * scale);
        beak.lineTo(bodyCenter.x() + 8.0 * scale, bodyCenter.y() - 13.0 * scale);
        beak.lineTo(bodyCenter.x(), bodyCenter.y() - 10.0 * scale);
        beak.closeSubpath();
        p.drawPath(beak);

        p.setBrush(QColor("#f46f5c"));
        p.drawRoundedRect(QRectF(bodyCenter.x() - 11.0 * scale, bodyCenter.y() - 8.0 * scale,
                                 22.0 * scale, 5.0 * scale), 2.0, 2.0);
    }
}

void GameWidget::drawSnowmanHunter(QPainter &p)
{
    if (!snowmanHunter.isActive()) {
        return;
    }

    QPointF s = worldToScreen(snowmanHunter.x(), snowmanHunter.y());
    if (s.x() < -120 || s.x() > width() + 120) {
        return;
    }

    const double scale = snowmanHunter.scale();
    p.save();
    p.setPen(Qt::NoPen);

    p.setBrush(QColor(0, 0, 0, 55));
    p.drawEllipse(QPointF(s.x(), s.y() + 30.0 * scale), 24.0 * scale, 7.0 * scale);

    QRadialGradient aura(s, 42.0 * scale);
    aura.setColorAt(0.0, QColor(180, 220, 255, 70));
    aura.setColorAt(1.0, QColor(180, 220, 255, 0));
    p.setBrush(aura);
    p.drawEllipse(s, 42.0 * scale, 42.0 * scale);

    p.setBrush(QColor(245, 247, 252));
    p.drawEllipse(QPointF(s.x(), s.y() + 18.0 * scale), 18.0 * scale, 16.0 * scale);
    p.drawEllipse(QPointF(s.x(), s.y() - 6.0 * scale), 14.0 * scale, 13.0 * scale);

    p.setBrush(QColor(35, 45, 70));
    p.drawEllipse(QPointF(s.x() - 4.0 * scale, s.y() - 10.0 * scale), 1.8 * scale, 1.8 * scale);
    p.drawEllipse(QPointF(s.x() + 4.0 * scale, s.y() - 10.0 * scale), 1.8 * scale, 1.8 * scale);

    p.setBrush(QColor("#ff9450"));
    QPainterPath nose;
    nose.moveTo(s.x(), s.y() - 7.0 * scale);
    nose.lineTo(s.x() + 9.0 * scale, s.y() - 5.0 * scale);
    nose.lineTo(s.x(), s.y() - 3.0 * scale);
    nose.closeSubpath();
    p.drawPath(nose);

    p.setBrush(QColor("#7a2028"));
    p.drawRoundedRect(QRectF(s.x() - 18.0 * scale, s.y() - 28.0 * scale, 36.0 * scale, 5.0 * scale), 2, 2);
    p.drawRoundedRect(QRectF(s.x() - 8.0 * scale, s.y() - 41.0 * scale, 16.0 * scale, 13.0 * scale), 3, 3);

    p.setBrush(QColor("#8b5a3c"));
    p.drawRoundedRect(QRectF(s.x() - 26.0 * scale, s.y() + 4.0 * scale, 10.0 * scale, 3.0 * scale), 1.5, 1.5);
    p.drawRoundedRect(QRectF(s.x() + 16.0 * scale, s.y() + 4.0 * scale, 10.0 * scale, 3.0 * scale), 1.5, 1.5);

    p.setBrush(QColor("#2f3b54"));
    p.drawEllipse(QPointF(s.x(), s.y() + 11.0 * scale), 1.8 * scale, 1.8 * scale);
    p.drawEllipse(QPointF(s.x(), s.y() + 18.0 * scale), 1.8 * scale, 1.8 * scale);

    p.restore();
}

void GameWidget::drawAvalanche(QPainter &p)
{
    if (!avalanche.isActive()) {
        return;
    }

    const double frontScreenX = worldToScreen(avalanche.frontX(), slopeY(avalanche.frontX())).x();
    if (frontScreenX < -260 || frontScreenX > width() + 260) {
        return;
    }

    const double leftWorld = cameraX - kPlayerScreenX - 30.0;
    const double rightWorld = avalanche.frontX();
    const int step = 10;

    QPainterPath body;
    bool started = false;
    for (double wx = leftWorld; wx <= rightWorld; wx += step) {
        const double span = std::max(1.0, rightWorld - leftWorld);
        const double t = std::clamp((wx - leftWorld) / span, 0.0, 1.0);
        const double thickness = lerp(68.0, 22.0, t);
        const double lump = std::sin(wx * 0.035 + timeAccum * 2.6) * 8.0
            + std::sin(wx * 0.083 + timeAccum * 4.4) * 4.5;
        QPointF slopePoint = worldToScreen(wx, slopeY(wx));
        QPointF crest(slopePoint.x(), slopePoint.y() - thickness + lump);
        if (!started) {
            body.moveTo(crest);
            started = true;
        } else {
            body.lineTo(crest);
        }
    }
    for (double wx = rightWorld; wx >= leftWorld; wx -= step) {
        QPointF slopePoint = worldToScreen(wx, slopeY(wx) + 2.0);
        body.lineTo(slopePoint);
    }
    body.closeSubpath();

    QLinearGradient snowGrad(0, 0, 0, height());
    snowGrad.setColorAt(0.0, QColor(252, 252, 255, 244));
    snowGrad.setColorAt(0.45, QColor(239, 244, 251, 236));
    snowGrad.setColorAt(1.0, QColor(208, 220, 234, 210));
    p.setPen(Qt::NoPen);
    p.setBrush(snowGrad);
    p.drawPath(body);

    QPainterPath foam;
    bool foamStarted = false;
    for (double wx = leftWorld; wx <= rightWorld; wx += step) {
        const double span = std::max(1.0, rightWorld - leftWorld);
        const double t = std::clamp((wx - leftWorld) / span, 0.0, 1.0);
        const double thickness = lerp(68.0, 22.0, t);
        const double lump = std::sin(wx * 0.035 + timeAccum * 2.6) * 8.0
            + std::sin(wx * 0.083 + timeAccum * 4.4) * 4.5;
        QPointF slopePoint = worldToScreen(wx, slopeY(wx));
        QPointF crest(slopePoint.x(), slopePoint.y() - thickness + lump);
        if (!foamStarted) {
            foam.moveTo(crest);
            foamStarted = true;
        } else {
            foam.lineTo(crest);
        }
    }
    QPen foamPen(QColor(255, 255, 255, 225), 2.4);
    foamPen.setCapStyle(Qt::RoundCap);
    foamPen.setJoinStyle(Qt::RoundJoin);
    p.strokePath(foam, foamPen);

    for (double wx = leftWorld; wx <= rightWorld; wx += 34.0) {
        const double span = std::max(1.0, rightWorld - leftWorld);
        const double t = std::clamp((wx - leftWorld) / span, 0.0, 1.0);
        const double thickness = lerp(68.0, 22.0, t);
        const double lump = std::sin(wx * 0.035 + timeAccum * 2.6) * 8.0
            + std::sin(wx * 0.083 + timeAccum * 4.4) * 4.5;
        QPointF slopePoint = worldToScreen(wx, slopeY(wx));
        if (slopePoint.x() < -140 || slopePoint.x() > width() + 140) continue;

        const int pileCount = 2 + static_cast<int>(lerp(3.0, 1.0, t));
        for (int i = 0; i < pileCount; ++i) {
            const double localShift = (i - (pileCount - 1) * 0.5) * lerp(16.0, 10.0, t);
            const double localLift = lerp(14.0, 7.0, t) + i * lerp(7.0, 4.0, t);
            const double bob = std::sin(timeAccum * 7.0 + wx * 0.025 + i * 0.9) * 2.0;
            const double size = lerp(24.0, 14.0, t) - i * lerp(2.6, 1.5, t);
            QPointF center(
                slopePoint.x() + localShift,
                slopePoint.y() - std::min(thickness - 6.0, localLift) + lump * 0.45 + bob);

            QRadialGradient puff(center.x() - size * 0.22, center.y() - size * 0.28, size * 1.2);
            puff.setColorAt(0.0, QColor(255, 255, 255, 246));
            puff.setColorAt(0.62, QColor(244, 247, 252, 230));
            puff.setColorAt(1.0, QColor(216, 226, 238, 168));
            p.setBrush(puff);
            p.drawEllipse(center, size, size * 0.76);
        }
    }
}

void GameWidget::drawRocks(QPainter &p)
{
    p.setPen(Qt::NoPen);
    for (const auto &r : rockList) {
        QPointF s = worldToScreen(r.x(), r.y());
        if (s.x() < -60 || s.x() > width() + 60) continue;

        // rock body
        QPainterPath path;
        double sz = r.size();
        path.moveTo(s.x() - sz * 0.9, s.y() + sz * 0.4);
        path.lineTo(s.x() - sz * 0.6, s.y() - sz * 0.6);
        path.lineTo(s.x() - sz * 0.1, s.y() - sz * 0.85);
        path.lineTo(s.x() + sz * 0.5, s.y() - sz * 0.55);
        path.lineTo(s.x() + sz * 0.95, s.y() - sz * 0.05);
        path.lineTo(s.x() + sz * 0.7, s.y() + sz * 0.45);
        path.closeSubpath();

        QLinearGradient grad(0, s.y() - sz, 0, s.y() + sz);
        grad.setColorAt(0.0, QColor(110, 100, 120));
        grad.setColorAt(1.0, QColor(50, 40, 60));
        p.setBrush(grad);
        p.drawPath(path);

        // small snow cap on top
        p.setBrush(QColor(255, 250, 245, 230));
        QPainterPath cap;
        cap.moveTo(s.x() - sz * 0.5, s.y() - sz * 0.55);
        cap.lineTo(s.x() - sz * 0.1, s.y() - sz * 0.85);
        cap.lineTo(s.x() + sz * 0.4, s.y() - sz * 0.6);
        cap.lineTo(s.x() + sz * 0.2, s.y() - sz * 0.5);
        cap.lineTo(s.x() - sz * 0.2, s.y() - sz * 0.45);
        cap.closeSubpath();
        p.drawPath(cap);
    }
}

void GameWidget::drawParticles(QPainter &p)
{
    p.setPen(Qt::NoPen);
    for (const auto &pa : particles) {
        QPointF s = worldToScreen(pa.x, pa.y);
        QColor c = pa.color;
        c.setAlphaF(std::clamp(pa.life / pa.maxLife, 0.0, 1.0) * c.alphaF());
        p.setBrush(c);
        p.drawEllipse(s, pa.size, pa.size);
    }
}

void GameWidget::drawPlayer(QPainter &p)
{
    QPointF s = worldToScreen(player.x(), player.y());
    const bool showFlightWings = player.isFlying() || player.isLandingFromFlight();

    p.save();
    p.translate(s);
    p.rotate(qRadiansToDegrees(player.angle()));

    if (player.isHoldingPenguin()) {
        QLinearGradient boost(-42, -10, 10, 6);
        boost.setColorAt(0.0, QColor(120, 230, 255, 0));
        boost.setColorAt(0.5, QColor(120, 230, 255, 80));
        boost.setColorAt(1.0, QColor(120, 230, 255, 0));
        p.setPen(Qt::NoPen);
        p.setBrush(boost);
        p.drawRoundedRect(QRectF(-54, -10, 56, 14), 7, 7);
    }

    if (showFlightWings) {
        const double flap = 0.7 + 0.3 * std::sin(timeAccum * 16.0);
        p.setBrush(QColor(255, 250, 240, 235));

        QPainterPath leftWing;
        leftWing.moveTo(-8, -2);
        leftWing.quadTo(-30, -24 * flap, -40, -3);
        leftWing.quadTo(-28, 16, -8, 10);
        leftWing.closeSubpath();
        p.drawPath(leftWing);

        QPainterPath rightWing;
        rightWing.moveTo(8, -2);
        rightWing.quadTo(30, -24 * flap, 40, -3);
        rightWing.quadTo(28, 16, 8, 10);
        rightWing.closeSubpath();
        p.drawPath(rightWing);

        p.setBrush(QColor(255, 222, 140, 210));
        p.drawEllipse(QPointF(0, 0), 4.0, 4.0);
    }

    // Shadow on slope
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 0, 60));
    p.drawEllipse(QPointF(0, showFlightWings ? kPlayerRadius + 14 : kPlayerRadius + 4),
                  showFlightWings ? 26 : 22, showFlightWings ? 5 : 4);

    // Ski (under feet)
    p.setBrush(QColor(40, 40, 60));
    p.drawRoundedRect(QRectF(-26, kPlayerRadius - 2, 52, 5), 3, 3);
    p.setBrush(QColor(255, 200, 90));
    p.drawRoundedRect(QRectF(-24, kPlayerRadius - 2, 48, 2), 1.5, 1.5);

    // Body (parka)
    QLinearGradient body(0, -10, 0, kPlayerRadius);
    body.setColorAt(0.0, QColor("#ff7a59"));
    body.setColorAt(1.0, QColor("#c63b3b"));
    p.setBrush(body);
    p.drawRoundedRect(QRectF(-10, -8, 20, 22), 6, 6);

    // Backpack
    p.setBrush(QColor("#3a2a4a"));
    p.drawRoundedRect(QRectF(-13, -5, 7, 16), 3, 3);

    // Head
    p.setBrush(QColor("#f1c8a3"));
    p.drawEllipse(QPointF(2, -14), 7, 8);

    // Hood / hat
    p.setBrush(QColor("#2c2240"));
    QPainterPath hat;
    hat.moveTo(-6, -16);
    hat.lineTo(10, -22);
    hat.lineTo(10, -10);
    hat.lineTo(-6, -8);
    hat.closeSubpath();
    p.drawPath(hat);

    // Scarf
    p.setBrush(QColor("#ffd866"));
    p.drawRoundedRect(QRectF(-6, -6, 12, 5), 2, 2);

    // Goggles
    p.setBrush(QColor(40, 40, 80, 230));
    p.drawRoundedRect(QRectF(-1, -16, 9, 4), 2, 2);

    if (player.isHoldingPenguin()) {
        p.setBrush(QColor("#1d2438"));
        p.drawEllipse(QPointF(15, 1), 10, 13);
        p.setBrush(QColor("#eef6ff"));
        p.drawEllipse(QPointF(15, 3), 5.5, 8);
        p.setBrush(QColor("#1d2438"));
        p.drawEllipse(QPointF(11, -10), 4.5, 5.5);
        p.drawEllipse(QPointF(18, -10), 4.5, 5.5);
        p.setBrush(QColor("#ffb347"));
        QPainterPath beak;
        beak.moveTo(21, -3);
        beak.lineTo(28, -1);
        beak.lineTo(21, 1);
        beak.closeSubpath();
        p.drawPath(beak);
        p.setBrush(QColor("#f46f5c"));
        p.drawRoundedRect(QRectF(6, -2, 14, 4), 2, 2);
    }

    p.restore();
}

void GameWidget::drawHUD(QPainter &p)
{
    // Top-left card with score, coins, distance
    QRectF card(16, 16, 230, 86);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(20, 14, 40, 160));
    p.drawRoundedRect(card, 14, 14);

    QFont f = p.font();
    f.setPointSizeF(11);
    f.setBold(true);
    p.setFont(f);
    p.setPen(QColor(255, 255, 255, 230));
    p.drawText(QRectF(card.left() + 14, card.top() + 8, 200, 20), Qt::AlignLeft | Qt::AlignVCenter,
               QStringLiteral("分数"));

    QFont big = p.font();
    big.setPointSizeF(22);
    big.setBold(true);
    p.setFont(big);
    p.setPen(QColor(255, 230, 120));
    p.drawText(QRectF(card.left() + 14, card.top() + 24, 200, 30), Qt::AlignLeft | Qt::AlignVCenter,
               QString::number(score));

    f.setPointSizeF(10);
    p.setFont(f);
    p.setPen(QColor(255, 255, 255, 220));
    QString line = QStringLiteral("金币 %1   距离 %2 m").arg(coins).arg(distance);
    p.drawText(QRectF(card.left() + 14, card.top() + 58, 220, 20), Qt::AlignLeft | Qt::AlignVCenter, line);

    // Top-right: best score
    QString best = QStringLiteral("最佳  %1").arg(bestScore);
    f.setPointSizeF(11);
    f.setBold(true);
    p.setFont(f);
    QFontMetrics fm(f);
    int w = fm.horizontalAdvance(best) + 28;
    QRectF bestCard(width() - w - 16, 16, w, 36);
    p.setBrush(QColor(20, 14, 40, 160));
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(bestCard, 12, 12);
    p.setPen(QColor(255, 230, 150));
    p.drawText(bestCard, Qt::AlignCenter, best);

    if (player.isHoldingPenguin()) {
        QFont boostFont = p.font();
        boostFont.setPointSizeF(10);
        boostFont.setBold(true);
        p.setFont(boostFont);

        QRectF boostCard(16, card.bottom() + 12, 230, 34);
        p.setBrush(QColor(20, 14, 40, 170));
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(boostCard, 12, 12);

        p.setPen(QColor(130, 240, 255));
        QString boostText = QStringLiteral("Penguin Boost %1s")
            .arg(QString::number(player.penguinBoostRemaining(), 'f', 1));
        p.drawText(boostCard, Qt::AlignCenter, boostText);
    }

    if (player.isFlying() || player.isLandingFromFlight()) {
        QFont wingFont = p.font();
        wingFont.setPointSizeF(10);
        wingFont.setBold(true);
        p.setFont(wingFont);

        const double wingCardTop = player.isHoldingPenguin() ? card.bottom() + 52 : card.bottom() + 12;
        QRectF wingCard(16, wingCardTop, 230, 34);
        p.setBrush(QColor(20, 14, 40, 170));
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(wingCard, 12, 12);

        p.setPen(QColor(255, 240, 180));
        const QString wingText = player.isFlying()
            ? QStringLiteral("Wing Flight %1s").arg(QString::number(player.wingFlightRemaining(), 'f', 1))
            : QStringLiteral("Wing Landing");
        p.drawText(wingCard, Qt::AlignCenter, wingText);
    }

    if (snowmanHunter.isActive()) {
        QFont hunterFont = p.font();
        hunterFont.setPointSizeF(10);
        hunterFont.setBold(true);
        p.setFont(hunterFont);

        double hunterTop = card.bottom() + 12;
        if (player.isHoldingPenguin()) hunterTop += 40;
        if (player.isFlying() || player.isLandingFromFlight()) hunterTop += 40;

        QRectF hunterCard(16, hunterTop, 230, 34);
        p.setBrush(QColor(55, 20, 26, 180));
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(hunterCard, 12, 12);

        p.setPen(snowmanHunter.isLethalChase() ? QColor(255, 205, 205) : QColor(255, 225, 225));
        const int gap = snowmanHunter.gapMeters(player.x());
        const QString hunterText = snowmanHunter.isLethalChase()
            ? QStringLiteral("Snowman Hunter %1 m").arg(gap)
            : QStringLiteral("Snowman Stalking %1 m").arg(gap);
        p.drawText(hunterCard, Qt::AlignCenter, hunterText);
    }

    if (avalanche.isActive()) {
        QFont avalancheFont = p.font();
        avalancheFont.setPointSizeF(10);
        avalancheFont.setBold(true);
        p.setFont(avalancheFont);

        double avalancheTop = card.bottom() + 12;
        if (player.isHoldingPenguin()) avalancheTop += 40;
        if (player.isFlying() || player.isLandingFromFlight()) avalancheTop += 40;
        if (snowmanHunter.isActive()) avalancheTop += 40;

        QRectF avalancheCard(16, avalancheTop, 230, 34);
        p.setBrush(QColor(32, 52, 72, 185));
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(avalancheCard, 12, 12);

        p.setPen(QColor(235, 245, 255));
        const int gap = avalanche.gapMeters(player.x());
        const QString avalancheText = QStringLiteral("Avalanche %1 m").arg(gap);
        p.drawText(avalancheCard, Qt::AlignCenter, avalancheText);
    }

    // Bottom hint
    f.setPointSizeF(9);
    f.setBold(false);
    p.setFont(f);
    p.setPen(QColor(255, 255, 255, 160));
    p.drawText(QRectF(0, height() - 26, width(), 20), Qt::AlignHCenter,
               QStringLiteral("空格 / ↑ / W / 鼠标左键 跳跃    Esc 结束"));
}

static void drawCenteredOutlinedText(QPainter &p, const QRectF &rect, const QString &text,
                                     const QColor &fill, const QColor &outline, int outlineWidth = 2)
{
    QPainterPath path;
    QFont f = p.font();
    QFontMetrics fm(f);
    QRectF tr = fm.boundingRect(text);
    QPointF pos(rect.center().x() - tr.width() / 2.0,
                rect.center().y() + fm.ascent() / 2.0 - 2);
    path.addText(pos, f, text);
    p.setPen(QPen(outline, outlineWidth));
    p.setBrush(Qt::NoBrush);
    p.drawPath(path);
    p.fillPath(path, fill);
}

void GameWidget::drawMenuOverlay(QPainter &p)
{
    // Soft vignette
    QLinearGradient g(0, 0, 0, height());
    g.setColorAt(0.0, QColor(0, 0, 0, 70));
    g.setColorAt(0.5, QColor(0, 0, 0, 30));
    g.setColorAt(1.0, QColor(0, 0, 0, 90));
    p.fillRect(rect(), g);

    // Title
    QFont titleFont = p.font();
    titleFont.setFamily("Segoe UI");
    titleFont.setPointSizeF(46);
    titleFont.setBold(true);
    p.setFont(titleFont);
    drawCenteredOutlinedText(p, QRectF(0, height() * 0.18, width(), 80),
                             QStringLiteral("阿尔托的冒险"),
                             QColor(255, 240, 200), QColor(40, 20, 60), 3);

    // Subtitle
    QFont sub = p.font();
    sub.setPointSizeF(14);
    sub.setBold(false);
    p.setFont(sub);
    p.setPen(QColor(255, 255, 255, 220));
    p.drawText(QRectF(0, height() * 0.30, width(), 30), Qt::AlignHCenter,
               QStringLiteral("Side Slope Adventure"));

    // Pulsing prompt
    double pulse = 0.5 + 0.5 * std::sin(menuPulse * 3.0);
    QFont prompt = p.font();
    prompt.setPointSizeF(16);
    prompt.setBold(true);
    p.setFont(prompt);
    p.setPen(QColor(255, 230, 130, static_cast<int>(180 + 60 * pulse)));
    p.drawText(QRectF(0, height() * 0.55, width(), 30), Qt::AlignHCenter,
               QStringLiteral("按 空格 / 鼠标左键 开始"));

    // Instruction card — size derived from real font metrics so it scales with DPI
    const QString infoText = QStringLiteral(
        "玩法：\n"
        "• 在山坡上不断滑行，距离越远分数越高\n"
        "• 收集金币：+10 分\n"
        "• 撞到岩石：游戏结束\n"
        "• 操作：空格 / ↑ / W / 鼠标左键 跳跃");

    QFont info = p.font();
    info.setPointSizeF(12);
    info.setBold(false);

    QFontMetrics fm(info);
    QRect textBound = fm.boundingRect(QRect(0, 0, 440, 1000),
                                      Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
                                      infoText);
    const double padX = 22, padY = 16;
    double boxW = std::max(440.0, textBound.width() + padX * 2);
    double boxH = textBound.height() + padY * 2;
    QRectF box(width() / 2.0 - boxW / 2.0,
               std::min(height() * 0.66, height() - boxH - 60.0),
               boxW, boxH);

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(20, 14, 40, 170));
    p.drawRoundedRect(box, 14, 14);

    p.setFont(info);
    p.setPen(QColor(255, 255, 255, 230));
    p.drawText(box.adjusted(padX, padY, -padX, -padY),
               Qt::AlignLeft | Qt::AlignTop,
               infoText);

    // Best score
    QFont bf = p.font();
    bf.setPointSizeF(13);
    bf.setBold(true);
    p.setFont(bf);
    p.setPen(QColor(255, 220, 130));
    p.drawText(QRectF(0, height() * 0.93, width(), 24), Qt::AlignHCenter,
               QStringLiteral("最佳成绩：%1").arg(bestScore));
}

void GameWidget::drawGameOverOverlay(QPainter &p)
{
    p.fillRect(rect(), QColor(0, 0, 0, 110));

    // Title
    QFont titleFont = p.font();
    titleFont.setFamily("Segoe UI");
    titleFont.setPointSizeF(46);
    titleFont.setBold(true);
    p.setFont(titleFont);
    drawCenteredOutlinedText(p, QRectF(0, height() * 0.18, width(), 80),
                             QStringLiteral("游戏结束"),
                             QColor(255, 200, 200), QColor(60, 10, 20), 3);

    // Stats card
    QRectF card(width() / 2.0 - 200, height() * 0.36, 400, 180);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(20, 14, 40, 200));
    p.drawRoundedRect(card, 16, 16);

    auto drawStatRow = [&](double y, const QString &label, const QString &value, const QColor &valueColor){
        QFont lab = p.font();
        lab.setPointSizeF(12);
        lab.setBold(false);
        p.setFont(lab);
        p.setPen(QColor(255, 255, 255, 200));
        p.drawText(QRectF(card.left() + 24, y, 200, 26), Qt::AlignLeft | Qt::AlignVCenter, label);

        QFont val = p.font();
        val.setPointSizeF(16);
        val.setBold(true);
        p.setFont(val);
        p.setPen(valueColor);
        p.drawText(QRectF(card.right() - 224, y, 200, 26), Qt::AlignRight | Qt::AlignVCenter, value);
    };

    drawStatRow(card.top() + 18, QStringLiteral("分数"),
                QString::number(score), QColor(255, 230, 130));
    drawStatRow(card.top() + 56, QStringLiteral("金币"),
                QString::number(coins), QColor(255, 200, 90));
    drawStatRow(card.top() + 94, QStringLiteral("距离"),
                QStringLiteral("%1 m").arg(distance), QColor(180, 220, 255));
    drawStatRow(card.top() + 132, QStringLiteral("最佳"),
                QString::number(bestScore), QColor(180, 255, 200));

    // Pulsing prompt
    double pulse = 0.5 + 0.5 * std::sin(menuPulse * 3.0);
    QFont prompt = p.font();
    prompt.setPointSizeF(15);
    prompt.setBold(true);
    p.setFont(prompt);
    p.setPen(QColor(255, 230, 130, static_cast<int>(180 + 60 * pulse)));
    p.drawText(QRectF(0, height() * 0.78, width(), 30), Qt::AlignHCenter,
               QStringLiteral("按 空格 / 鼠标左键 / R 重新开始"));
}
