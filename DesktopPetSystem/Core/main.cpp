#pragma execution_character_set("utf-8")

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include "PetWidget.h"
#include "PetFSM.h"
#include "PetAttribute.h"
#include "PetConfig.h"

/** 从 exe 目录逐级向上查找相对路径；若多处存在则取最外层（最后一次命中） */
static QString findConfigFileUpward(const QString& relativePath)
{
	QString found;
	QDir dir(QCoreApplication::applicationDirPath());
	for (int depth = 0; depth < 12; ++depth) {
		const QString candidate = dir.absoluteFilePath(relativePath);
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

/** 返回可加载的 pet_config.json 绝对路径；若缺失则从 pet_config.defaults.json 复制生成 */
static QString resolvePetConfigPathForStartup()
{
	const QString petCfg = findConfigFileUpward(QStringLiteral("resources/config/pet_config.json"));
	if (!petCfg.isEmpty()) {
		return petCfg;
	}
	const QString defaults = findConfigFileUpward(QStringLiteral("resources/config/pet_config.defaults.json"));
	if (defaults.isEmpty()) {
		return {};
	}
	const QString dest = QFileInfo(defaults).absolutePath() + QStringLiteral("/pet_config.json");
	if (QFile::exists(dest)) {
		return QFileInfo(dest).absoluteFilePath();
	}
	if (!QFile::copy(defaults, dest)) {
		qCritical() << "[配置] 无法从模板创建 pet_config.json:" << defaults << "->" << dest;
		return {};
	}
	qWarning() << "[配置] 未找到 pet_config.json，已从 pet_config.defaults.json 复制生成:" << dest;
	return QFileInfo(dest).absoluteFilePath();
}

// 程序入口
int main(int argc, char* argv[])
{
	QApplication a(argc, argv);
	a.setApplicationName("DesktopPet");

	// 获取应用程序目录
	QString appDir = QCoreApplication::applicationDirPath();
	qDebug() << "[main] 应用程序目录:" << appDir;

	const QString configPath = resolvePetConfigPathForStartup();
	qDebug() << "[main] 配置文件路径:" << configPath;

	// 加载配置
	PetConfig* config = PetConfig::getInstance();
	if (configPath.isEmpty() || !config->loadConfig(configPath)) {
		qCritical() << "[致命错误] 配置加载失败：需要 resources/config/pet_config.json，"
		                "或至少存在 resources/config/pet_config.defaults.json 以自动生成前者。";
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
