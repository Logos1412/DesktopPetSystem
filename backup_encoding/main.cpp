#pragma execution_character_set("utf-8")

#include <QApplication>
#include "PetWidget.h"
#include "PetFSM.h"
#include "PetAttribute.h"
#include "PetConfig.h" // 包含配置类

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("桌面宠物");

    // 第一步：加载配置文件（必须在属性初始化前！）
    PetConfig* config = PetConfig::getInstance();
    if (!config->loadConfig("../resources/config/pet_config.json")) {
        qCritical() << "[致命错误] 配置文件加载失败，程序退出！";
        return -1;
    }

    // 第二步：初始化属性（此时会读取配置的初始值）
    PetAttribute* petAttr = new PetAttribute();
    qDebug() << "[main] 初始精力：" << petAttr->getEnergy(); // 验证是否为60

    // 第三步：初始化状态机和窗口
    PetFSM* petFsm = new PetFSM(petAttr);
    PetWidget w(petFsm, petAttr);
    w.show();

    return a.exec();
}