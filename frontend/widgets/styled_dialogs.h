/**
 * @file styled_dialogs.h
 * @brief 自定义暗色主题对话框工具函数
 */

#ifndef STYLED_DIALOGS_H
#define STYLED_DIALOGS_H

#include <QString>
#include <QWidget>

namespace StyledDialogs {

/**
 * @brief 显示Vercel风格的信息提示框
 */
void showInfo(QWidget* parent, const QString& title, const QString& message);

/**
 * @brief 显示Vercel风格的警告提示框
 */
void showWarning(QWidget* parent, const QString& title, const QString& message);

/**
 * @brief 显示Vercel风格的错误提示框
 */
void showError(QWidget* parent, const QString& title, const QString& message);

/**
 * @brief 显示Vercel风格的确认对话框
 * @return true表示用户点击确认，false表示取消
 */
bool confirm(QWidget* parent, const QString& title, const QString& message);

/**
 * @brief 显示Vercel风格的文本输入对话框
 * @param ok 输出参数，用户点击确定为true，取消为false
 * @return 用户输入的文本
 */
QString getText(QWidget* parent, const QString& title, const QString& label, 
                const QString& defaultText = QString(), bool* ok = nullptr);

} // namespace StyledDialogs

#endif // STYLED_DIALOGS_H
