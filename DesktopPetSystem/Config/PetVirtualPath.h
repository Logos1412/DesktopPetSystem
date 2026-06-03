#pragma once
#pragma execution_character_set("utf-8")

#include <QString>

/** 应用根目录下的逻辑路径（pet:/…），在运行时解析为真实绝对路径。 */
namespace PetVirtualPath {

QString schemePrefix();

bool isVirtualPath(const QString& path);

/** 将工程根下的相对路径规范为 pet:/…（正斜杠）。已是 pet:/ 则规范化。 */
QString toVirtual(const QString& relativePathFromProjectRoot);

/** 结合 projectRoot（含 resources 的目录）解析为可用于 QFile/QProcess 的绝对路径。 */
QString resolveToAbsolute(const QString& path, const QString& projectRoot);

/** 自 exe 目录向上查找含 resources/config/pet_config.json 的目录作为工程根。 */
QString findProjectRootFromExe();

/**
 * 用于配置中的脚本路径、记忆文件路径等：绝对路径原样规范化；
 * pet:/ 规范化；旧版相对路径（如 resources/…）转为 pet:/…。
 * 勿用于可执行名（如 python3）。
 */
QString normalizeConfigurablePath(const QString& raw);

} // namespace PetVirtualPath
