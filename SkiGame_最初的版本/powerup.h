#ifndef POWERUP_H
#define POWERUP_H

#include <QObject>
#include <QRect>
#include <QPainter>
#include <QImage>
#include <QColor>

/**
 * @brief 道具类，表示游戏中从上方落下的道具
 * 
 * 负责处理道具的位置、大小、速度和类型
 */
class PowerUp : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief 道具类型枚举
     */
    enum PowerUpType {
        SpeedBoost,  // 加速道具：玩家速度提升30%，持续5秒
        Health,      // 加血道具：恢复1点血量
        Invincibility, // 无敌道具：3秒内免疫所有伤害
        Bomb         // 炸弹道具：清除当前屏幕内所有障碍物和怪物
    };
    
    /**
     * @brief 构造函数
     * @param x 道具初始x坐标
     * @param y 道具初始y坐标
     * @param width 道具宽度
     * @param height 道具高度
     * @param speed 道具下落速度
     * @param type 道具类型
     * @param color 道具颜色
     * @param image 道具图像
     * @param parent 父对象
     */
    PowerUp(int x, int y, int width, int height, int speed, PowerUpType type, 
            const QColor& color, const QImage& image, QObject *parent = nullptr);
    
    /**
     * @brief 获取道具矩形（用于绘制和碰撞检测）
     * @return 道具的矩形区域
     */
    QRect getRect() const;
    
    /**
     * @brief 道具下落
     */
    void moveDown();
    
    /**
     * @brief 检查道具是否超出屏幕底部
     * @param maxY 屏幕最大y坐标
     * @return 如果道具超出屏幕底部返回true，否则返回false
     */
    bool isOutOfBounds(int maxY) const;
    
    /**
     * @brief 获取道具类型
     * @return 道具类型
     */
    PowerUpType getType() const;
    
    /**
     * @brief 绘制道具
     * @param painter 绘图设备
     */
    void draw(QPainter *painter);

private:
    int m_x;              // 道具x坐标
    int m_y;              // 道具y坐标
    int m_width;          // 道具宽度
    int m_height;         // 道具高度
    int m_speed;          // 道具下落速度
    PowerUpType m_type;   // 道具类型
    QColor m_color;       // 道具颜色
    QImage m_image;       // 道具图像
};

#endif // POWERUP_H