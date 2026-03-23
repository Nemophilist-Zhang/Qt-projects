#ifndef GAMEWINDOW_H
#define GAMEWINDOW_H

#include <QGraphicsView>
#include <QTimer>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QProgressBar>
#include "player.h"
#include "obstacle.h"

enum GameState {
    STATE_MENU,
    STATE_PLAYING,
    STATE_PAUSED,
    STATE_GAMEOVER
};

enum Difficulty {
    DIFFICULTY_EASY = 0,
    DIFFICULTY_MEDIUM,
    DIFFICULTY_HARD
};

class GameWindow : public QGraphicsView
{
    Q_OBJECT

public:
    explicit GameWindow(QWidget *parent = nullptr);
    ~GameWindow();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void updateGame();
    void spawnObstacle();
    void startGame();
    void pauseGame();
    void resumeGame();
    void gameOver();
    void revivePlayer();
    void restartGame();
    void updateDifficulty(int index);
    void updateUI();

private:
    void initScene();
    void initMenuUI();
    void initGameUI();
    void initGameOverUI();
    void clearScene();
    void setGameState(GameState state);

    QGraphicsScene *m_scene;
    QTimer *m_gameTimer;
    QTimer *m_spawnTimer;
    Player *m_player;

    GameState m_gameState;
    Difficulty m_difficulty;
    int m_score;
    int m_reviveCount;
    qreal m_gameSpeed;

    QWidget *m_menuWidget;
    QWidget *m_gameOverWidget;
    QLabel *m_scoreLabel;
    QLabel *m_hpLabel;
    QProgressBar *m_energyBar;
    QComboBox *m_difficultyCombo;
};

#endif // GAMEWINDOW_H