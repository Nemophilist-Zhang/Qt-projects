#ifndef OBSTACLE_H
#define OBSTACLE_H

#include <QObject>
#include <QRect>
#include <QColor>
#include <QPainter>
#include <QImage>
#include <QPointF>

/**
 * @brief 障碍物类，表示游戏中从上方落下的障碍物
 * 
 * 负责处理障碍物的位置、大小、速度、类型和移动逻辑
 */
class Obstacle : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief 障碍物类型枚举
     */
    enum ObstacleType {
        Tree,       // 小树（伤害1）
        Rock,       // 岩石（伤害2）
        Pit,        // 深坑（伤害1并减速）
        Ice,        // 冰面（滑步加速但难控制）
        Snowball    // 雪球（伤害1，可滚动）
    };
    
    /**
     * @brief 障碍物状态枚举
     */
    enum ObstacleState {
        Normal,     // 正常
        Breaking,   // 破碎中
        Falling,    // 下落中
        Rolling     // 滚动中
    };
    
    /**
     * @brief 构造函数
     * @param x 障碍物初始x坐标
     * @param y 障碍物初始y坐标
     * @param width 障碍物宽度
     * @param height 障碍物高度
     * @param speed 障碍物下落速度
     * @param type 障碍物类型
     * @param color 障碍物颜色
     * @param image 障碍物图像
     * @param parent 父对象
     */
    Obstacle(int x, int y, int width, int height, int speed, ObstacleType type, 
             const QColor& color, const QImage& image, QObject *parent = nullptr);
    
    /**
     * @brief 析构函数
     */
    ~Obstacle();
    
    /**
     * @brief 获取障碍物矩形（用于绘制和碰撞检测）
     * @return 障碍物的矩形区域
     */
    QRect getRect() const;
    
    /**
     * @brief 障碍物下落
     */
    void moveDown();
    
    /**
     * @brief 障碍物移动（用于滚动的雪球）
     */
    void move();
    
    /**
     * @brief 检查障碍物是否超出屏幕底部
     * @param maxY 屏幕最大y坐标
     * @return 如果障碍物超出屏幕底部返回true，否则返回false
     */
    bool isOutOfBounds(int maxY) const;
    
    /**
     * @brief 设置障碍物y坐标
     * @param y 新的y坐标
     */
    void setY(int y);
    
    /**
     * @brief 获取障碍物y坐标
     * @return 障碍物当前y坐标
     */
    int getY() const;
    
    /**
     * @brief 获取障碍物位置
     * @return 障碍物当前位置
     */
    QPointF getPosition() const;
    
    /**
     * @brief 设置障碍物下落速度
     * @param speed 新的下落速度
     */
    void setSpeed(int speed);
    
    /**
     * @brief 获取障碍物下落速度
     * @return 障碍物当前下落速度
     */
    int getSpeed() const;
    
    /**
     * @brief 获取障碍物类型
     * @return 障碍物类型
     */
    ObstacleType getType() const;
    
    /**
     * @brief 获取障碍物伤害值
     * @return 障碍物伤害值
     */
    int getDamage() const;
    
    /**
     * @brief 获取障碍物减速效果
     * @return 减速百分比（0.0-1.0）
     */
    float getSlowdownFactor() const;
    
    /**
     * @brief 获取障碍物加速效果
     * @return 加速百分比（0.0-1.0）
     */
    float getSpeedupFactor() const;
    
    /**
     * @brief 设置障碍物状态
     * @param state 新的状态
     */
    void setState(ObstacleState state);
    
    /**
     * @brief 获取障碍物状态
     * @return 当前状态
     */
    ObstacleState getState() const;
    
    /**
     * @brief 更新障碍物状态
     */
    void update();
    
    /**
     * @brief 检查障碍物是否可被摧毁
     * @return 是否可被摧毁
     */
    bool isDestructible() const;
    
    /**
     * @brief 对障碍物造成伤害
     * @param damage 伤害值
     * @return 是否被摧毁
     */
    bool takeDamage(int damage);

    /**
     * @brief 绘制障碍物
     * @param painter 绘图设备
     */
    void draw(QPainter *painter);
    
    /**
     * @brief 绘制障碍物特效
     * @param painter 绘图设备
     */
    void drawEffects(QPainter *painter);
signals:
    /**
     * @brief 障碍物被摧毁信号
     * @param obstacle 被摧毁的障碍物
     */
    void destroyed(Obstacle *obstacle);
    
    /**
     * @brief 障碍物状态变化信号
     * @param newState 新的状态
     */
    void stateChanged(ObstacleState newState);

private:
    int m_x;              // 障碍物x坐标
    int m_y;              // 障碍物y坐标
    int m_width;          // 障碍物宽度
    int m_height;         // 障碍物高度
    int m_speed;          // 障碍物下落速度
    ObstacleType m_type;  // 障碍物类型
    QColor m_color;       // 障碍物颜色
    QImage m_image;       // 障碍物图像
    ObstacleState m_state;// 障碍物状态
    int m_health;         // 障碍物血量
    float m_rotation;     // 旋转角度（用于雪球滚动）
    float m_breakProgress;// 破碎进度（0.0-1.0）
    QPointF m_velocity;   // 速度向量（用于复杂移动）