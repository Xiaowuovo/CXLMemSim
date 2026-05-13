#ifndef EXPORT_DIALOG_H
#define EXPORT_DIALOG_H

#include <QDialog>
#include <QCheckBox>
#include <QRadioButton>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include <QListWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QTabWidget>
#include <QDateTime>
#include <vector>
#include "analyzer/timing_analyzer.h"

/**
 * @brief 导出对话框 — 科研级实验数据管理
 *
 * 功能：
 *  Tab 1 - 导出当前会话：CSV / JSON / Markdown 报告多格式导出
 *  Tab 2 - 历史会话：查看并导出已保存的历史实验
 *  Tab 3 - 会话备注：为本次实验添加文字说明
 */
class ExportDialog : public QDialog {
    Q_OBJECT

public:
    // 单次导出范围
    enum ExportRange { ALL_EPOCHS, RECENT_EPOCHS, EPOCH_RANGE };

    // 导出选项
    struct ExportOptions {
        bool exportCSV    = true;
        bool exportJSON   = false;
        bool exportMarkdown = false;
        ExportRange range = ALL_EPOCHS;
        int recentCount   = 10000;
        int rangeStart    = 0;
        int rangeEnd      = -1;  // -1 表示到末尾
        QString sessionNote;
        QString sessionTag;
    };

    // 已保存的历史会话元数据
    struct SessionRecord {
        QString id;            // UUID风格的标识符
        QDateTime timestamp;
        QString tag;
        QString note;
        QString csvPath;       // 已导出的 CSV 路径（空表示未导出）
        QString jsonPath;
        int epochCount;
        double avgLatencyNs;
        double avgBandwidthGbps;
    };

    explicit ExportDialog(QWidget *parent = nullptr);
    ~ExportDialog() = default;

    void setEpochData(const std::vector<cxlsim::EpochStats>& data);
    void setConfigData(const QString& configJson);
    void setSessionHistory(const QList<SessionRecord>& history);

    ExportOptions getExportOptions() const;
    // 导出后返回当前会话元数据
    SessionRecord getSessionRecord() const { return currentSession_; }

signals:
    void exportDone(const SessionRecord& record);

private slots:
    void onExport();
    void onRangeChanged();
    void onHistoryItemDoubleClicked(QListWidgetItem* item);
    void onDeleteHistory();
    void onReExportHistory();

private:
    void setupUI();
    void setupExportTab(QWidget* tab);
    void setupHistoryTab(QWidget* tab);
    void setupNoteTab(QWidget* tab);
    void refreshHistoryList();

    QString generateCSV(const std::vector<cxlsim::EpochStats>& data, int start, int end) const;
    QString generateMarkdown(const std::vector<cxlsim::EpochStats>& data, int start, int end) const;
    bool saveTextFile(const QString& content, const QString& path);

    // ── 导出 Tab UI ──
    QRadioButton* allEpochsRadio_;
    QRadioButton* recentEpochsRadio_;
    QRadioButton* epochRangeRadio_;
    QSpinBox*     recentCountSpin_;
    QSpinBox*     rangeStartSpin_;
    QSpinBox*     rangeEndSpin_;
    QCheckBox*    csvCheckbox_;
    QCheckBox*    jsonCheckbox_;
    QCheckBox*    markdownCheckbox_;
    QLineEdit*    tagEdit_;
    QPushButton*  exportBtn_;
    QPushButton*  cancelBtn_;
    QLabel*       statusLabel_;
    QProgressBar* progressBar_;
    QLabel*       summaryLabel_;

    // ── 历史 Tab UI ──
    QListWidget*  historyList_;
    QPushButton*  deleteHistoryBtn_;
    QPushButton*  reExportBtn_;

    // ── 备注 Tab UI ──
    QTextEdit*    noteEdit_;

    // ── 数据 ──
    std::vector<cxlsim::EpochStats> epochData_;
    QString configJson_;
    QList<SessionRecord> history_;
    SessionRecord currentSession_;
};

#endif // EXPORT_DIALOG_H
