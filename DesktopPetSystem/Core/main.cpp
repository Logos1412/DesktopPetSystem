#pragma execution_character_set("utf-8")

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include "PetWidget.h"
#include "PetFSM.h"
#include "PetAttribute.h"
#include "PetConfig.h"

static QString findPetConfigPath()
{
	/* 从 exe 目录逐级向上；同一相对路径可能在「内层工程目录」与「仓库根」各出现一次，取最外层（最后命中）的那一份 */
	QString found;
	QDir dir(QCoreApplication::applicationDirPath());
	for (int depth = 0; depth < 12; ++depth) {
		const QString candidate = dir.absoluteFilePath(QStringLiteral("resources/config/pet_config.json"));
		const QFileInfo fi(candidate);
		if (fi.isFile()) {
			found = fi.absoluteFilePath();
		}
		if (!dir.cdUp()) {
			break;
		}
	}
	return found;
}

// 程序入口
int main(int argc, char* argv[])
{
	QApplication a(argc, argv);
	a.setApplicationName("DesktopPet");

	// 获取应用程序目录
	QString appDir = QCoreApplication::applicationDirPath();
	qDebug() << "[main] 应用程序目录:" << appDir;

	const QString configPath = findPetConfigPath();
	qDebug() << "[main] 配置文件路径:" << configPath;

	// 加载配置
	PetConfig* config = PetConfig::getInstance();
	if (configPath.isEmpty() || !config->loadConfig(configPath)) {
		qCritical() << "[致命错误] 配置加载失败（未找到 resources/config/pet_config.json），程序退出！";
		return -1;
	}

	// 创建属性对象
	PetAttribute* petAttr = new PetAttribute();
	qDebug() << "[main] 初始精力:" << petAttr->getEnergy();

	// 创建状态机
	PetFSM* petFsm = new PetFSM(petAttr);

	// 创建并显示主窗口
	PetWidget w(petFsm, petAttr);
	w.show();

	return a.exec();
}
