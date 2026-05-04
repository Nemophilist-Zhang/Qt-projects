#ifndef GAMEWIDGET_H
#define GAMEWIDGET_H

#include <QWidget>
#include <QTimer>
#include <QElapsedTimer>
#include <QKeyEvent>
#include <QMouseEvent>
#include "avalanche.h"
#include "obstacle.h"
#include "penguinpowerup.h"
#include "player.h"
#include "powerup.h"
#include "snowmanhunter.h"
#include <random>
#include <vector>

class QMediaPlayer;

class GameWidget : public QWidget
{
    Q_OBJECT

public:
    enum class State { Menu, Playing, GameOver };

    explicit GameWidget(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void tick();

private:
    struct Tree {
        double x;
        double scale;
        int layer; // 0 = far, 1 = mid, 2 = near
    };
    struct Star {
        double x, y;
        double brightness;
    };
    struct Particle {
        double x, y;
        double vx, vy;
        double life, maxLife;
        double size;
        QColor color;
    };
    struct CloudPuff {
        double x, y;
        double scale;
        double speed;
    };

    void resetGame();
    void startGame();
    void endGame();
    void pickupPenguin();
    void pickupWings();
    void spawnSnowmanHunter();
    void spawnAvalanche();
    void updatePhysics(double dt);
    void spawnContent();
    void tryJump();
    void setupAudio();
    void updateAudioState();
    QString assetPath(const QString &fileName) const;
    QMediaPlayer *createAudioPlayer(const QString &fileName, float volume, int loops);
    void playOneShot(QMediaPlayer *player);
    void playCoinSound();

    double slopeY(double x) const;
    double slopeAngle(double x) const;
    double horizonY() const;
    double playerBaseSpeed() const;
    double playerSpeedFactor(double slope, bool airborne) const;
    double speedMultiplier() const;

    // Drawing
    void drawSky(QPainter &p);
    void drawStars(QPainter &p);
    void drawSun(QPainter &p);
    void drawClouds(QPainter &p);
    void drawMountains(QPainter &p);
    void drawTrees(QPainter &p);
    void drawSlope(QPainter &p);
    void drawStartCabin(QPainter &p);
    void drawCoins(QPainter &p);
    void drawWingPowerups(QPainter &p);
    void drawPenguins(QPainter &p);
    void drawSnowmanHunter(QPainter &p);
    void drawAvalanche(QPainter &p);
    void drawRocks(QPainter &p);
    void drawParticles(QPainter &p);
    void drawPlayer(QPainter &p);
    void drawHUD(QPainter &p);
    void drawMenuOverlay(QPainter &p);
    void drawGameOverOverlay(QPainter &p);

    QPointF worldToScreen(double wx, double wy) const;

    State state = State::Menu;

    Player player;
    double rotationInAir = 0;

    // Camera
    double cameraX = 0;
    double cameraY = 0;

    // Stats
    int score = 0;
    int coins = 0;
    int distance = 0;
    int bestScore = 0;

    // Spawning
    double nextCoinX = 0;
    double nextWingX = 0;
    double nextPenguinX = 0;
    double nextSnowmanSpawnX = 0;
    double nextRockX = 0;
    double nextTreeNearX = 0;
    double nextTreeMidX = 0;
    double nextTreeFarX = 0;

    // Content
    std::vector<Coin> coinList;
    std::vector<WingPowerup> wingList;
    std::vector<PenguinPowerup> penguinList;
    std::vector<RockObstacle> rockList;
    std::vector<Tree> treeList;
    std::vector<Particle> particles;
    std::vector<Star> stars;
    std::vector<CloudPuff> clouds;

    // Animation
    double timeAccum = 0;
    double menuPulse = 0;
    SnowmanHunter snowmanHunter;
    Avalanche avalanche;

    // Random
    std::mt19937 rng;

    // Timing
    QTimer *timer;
    QElapsedTimer elapsed;
    qint64 lastTickMs = 0;
    QMediaPlayer *backgroundMusic = nullptr;
    std::vector<QMediaPlayer *> coinSounds;
    int nextCoinSoundIndex = 0;
    QMediaPlayer *flyLoop = nullptr;
    QMediaPlayer *monsterLoop = nullptr;
    QMediaPlayer *penguinLoop = nullptr;
    QMediaPlayer *skiingLoop = nullptr;
    QMediaPlayer *avalancheLoop = nullptr;
    bool backgroundMusicPlaying = false;
    bool flyLoopPlaying = false;
    bool monsterLoopPlaying = false;
    bool penguinLoopPlaying = false;
    bool skiingLoopPlaying = false;
    bool avalancheLoopPlaying = false;

    // Constants
    static constexpr double kGravity = 1500.0;
    static constexpr double kJumpVel = -640.0;
    static constexpr double kBaseSpeed = 320.0;
    static constexpr double kMaxSpeed = 620.0;
    static constexpr double kSpeedRamp = 0.0035;
    static constexpr double kPenguinBoostDuration = 15.0;
    static constexpr double kPenguinSpeedMultiplier = 1.45;
    static constexpr double kWingFlightDuration = 10.0;
    static constexpr double kWingFlightHeight = 170.0;
    static constexpr double kSnowmanCatchDistance = 26.0;
    static constexpr double kPlayerScreenX = 260.0;
    static constexpr double kPlayerRadius = 16.0;
};

#endif // GAMEWIDGET_H
