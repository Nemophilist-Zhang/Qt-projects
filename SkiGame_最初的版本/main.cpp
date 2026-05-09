#include <QApplication>
#include "gamewindow.h"

/**
 * @brief 程序入口点
 * @param argc 命令行参数数量
 * @param argv 命令行参数数组
 * @return 程序退出状态
 */
int main(int argc, char *argv[])
{
    // 创建Qt应用程序实例
    QApplication app(argc, argv);
    
    // 创建游戏窗口
    GameWindow gameWindow;
    
    // 显示游戏窗口
    gameWindow.show();
    
    // 运行应用程序主循环
    return app.exec();
}