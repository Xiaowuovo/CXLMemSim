/**
 * @file styled_dialogs.cpp
 * @brief 自定义暗色主题对话框实现
 */

#include "styled_dialogs.h"
#include <QMessageBox>
#include <QInputDialog>
#include <QPushButton>
#include <QLineEdit>

namespace StyledDialogs {

// Vercel暗色主题对话框样式
static const QString dialogStyle = R"(
    QMessageBox {
        background-color: #0A0A0A;
        color: #EDEDED;
    }
    QMessageBox QLabel {
        color: #EDEDED;
        font-size: 13px;
        background-color: transparent;
    }
    QPushButton {
        background-color: #1A1A1A;
        color: #EDEDED;
        border: 1px solid #333333;
        border-radius: 6px;
        padding: 8px 16px;
        font-size: 13px;
        font-weight: 500;
        min-width: 80px;
    }
    QPushButton:hover {
        background-color: #222222;
        border-color: #555555;
    }
    QPushButton:pressed {
        background-color: #2A2A2A;
    }
    QPushButton:default {
        background-color: #0070F3;
        border-color: #0070F3;
        color: #FFFFFF;
    }
    QPushButton:default:hover {
        background-color: #0761D1;
    }
)";

static const QString inputDialogStyle = R"(
    QInputDialog {
        background-color: #0A0A0A;
        color: #EDEDED;
    }
    QInputDialog QLabel {
        color: #EDEDED;
        font-size: 13px;
    }
    QLineEdit {
        background-color: #111111;
        color: #EDEDED;
        border: 1px solid #333333;
        border-radius: 6px;
        padding: 8px 12px;
        font-size: 13px;
    }
    QLineEdit:focus {
        border-color: #0070F3;
        background-color: #1A1A1A;
    }
    QPushButton {
        background-color: #1A1A1A;
        color: #EDEDED;
        border: 1px solid #333333;
        border-radius: 6px;
        padding: 8px 16px;
        font-size: 13px;
        font-weight: 500;
        min-width: 80px;
    }
    QPushButton:hover {
        background-color: #222222;
        border-color: #555555;
    }
    QPushButton:default {
        background-color: #0070F3;
        border-color: #0070F3;
        color: #FFFFFF;
    }
    QPushButton:default:hover {
        background-color: #0761D1;
    }
)";

void showInfo(QWidget* parent, const QString& title, const QString& message) {
    QMessageBox msgBox(parent);
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setStyleSheet(dialogStyle);
    msgBox.exec();
}

void showWarning(QWidget* parent, const QString& title, const QString& message) {
    QMessageBox msgBox(parent);
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setStyleSheet(dialogStyle);
    msgBox.exec();
}

void showError(QWidget* parent, const QString& title, const QString& message) {
    QMessageBox msgBox(parent);
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.setStyleSheet(dialogStyle);
    msgBox.exec();
}

bool confirm(QWidget* parent, const QString& title, const QString& message) {
    QMessageBox msgBox(parent);
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    msgBox.setStyleSheet(dialogStyle);
    
    // 设置按钮文本为中文
    msgBox.button(QMessageBox::Yes)->setText("确定");
    msgBox.button(QMessageBox::No)->setText("取消");
    
    return msgBox.exec() == QMessageBox::Yes;
}

QString getText(QWidget* parent, const QString& title, const QString& label, 
                const QString& defaultText, bool* ok) {
    QInputDialog inputDialog(parent);
    inputDialog.setWindowTitle(title);
    inputDialog.setLabelText(label);
    inputDialog.setTextValue(defaultText);
    inputDialog.setStyleSheet(inputDialogStyle);
    inputDialog.resize(400, 150);
    
    // 设置按钮文本为中文
    if (auto* okBtn = inputDialog.findChild<QPushButton*>("qt_inputdlg_ok")) {
        okBtn->setText("确定");
    }
    if (auto* cancelBtn = inputDialog.findChild<QPushButton*>("qt_inputdlg_cancel")) {
        cancelBtn->setText("取消");
    }
    
    bool accepted = (inputDialog.exec() == QDialog::Accepted);
    if (ok) *ok = accepted;
    
    return accepted ? inputDialog.textValue() : QString();
}

} // namespace StyledDialogs
