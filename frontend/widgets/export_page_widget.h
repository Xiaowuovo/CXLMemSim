#pragma once

#include <QWidget>
#include <QListWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QLabel>
#include <QProgressBar>
#include <QCheckBox>
#include <QRadioButton>
#include <QSpinBox>
#include <QPushButton>
#include <QTabWidget>
#include <QDateTime>
#include <vector>
#include "analyzer/timing_analyzer.h"
#include "widgets/export_dialog.h"  // reuse SessionRecord / ExportOptions

/**
 * @brief 导出数据页面（嵌入式，非弹窗）
 *
 * 与 ExportDialog 功能完全对等，但作为 pageStack_ 中的一个常驻页面存在。
 * 通过 setEpochData / setConfigData / setSessionHistory 注入数据。
 */
class ExportPageWidget : public QWidget {
    Q_OBJECT

public:
    using SessionRecord  = ExportDialog::SessionRecord;
    using ExportOptions  = ExportDialog::ExportOptions;
    using ExportRange    = ExportDialog::ExportRange;

    explicit ExportPageWidget(QWidget* parent = nullptr);
    ~ExportPageWidget() = default;

    void setEpochData(const std::vector<cxlsim::EpochStats>& data);
    void setConfigData(const QString& configJson);
    void setSessionHistory(const QList<SessionRecord>& history);

    ExportOptions getExportOptions() const;

signals:
    void exportDone(const SessionRecord& record);

private slots:
    void onExport();
    void onRangeChanged();
    void onHistoryItemDoubleClicked(QListWidgetItem* item);
    void onDeleteHistory();

private:
    void setupUI();
    void setupExportTab(QWidget* tab);
    void setupHistoryTab(QWidget* tab);
    void setupNoteTab(QWidget* tab);
    void refreshHistoryList();

    QString generateCSV(const std::vector<cxlsim::EpochStats>& data, int start, int end) const;
    QString generateMarkdown(const std::vector<cxlsim::EpochStats>& data, int start, int end) const;
    bool saveTextFile(const QString& content, const QString& path);

    // ── 导出 Tab ──
    QRadioButton* allEpochsRadio_    = nullptr;
    QRadioButton* recentEpochsRadio_ = nullptr;
    QRadioButton* epochRangeRadio_   = nullptr;
    QSpinBox*     recentCountSpin_   = nullptr;
    QSpinBox*     rangeStartSpin_    = nullptr;
    QSpinBox*     rangeEndSpin_      = nullptr;
    QCheckBox*    csvCheckbox_       = nullptr;
    QCheckBox*    jsonCheckbox_      = nullptr;
    QCheckBox*    markdownCheckbox_  = nullptr;
    QLineEdit*    tagEdit_           = nullptr;
    QPushButton*  exportBtn_         = nullptr;
    QLabel*       statusLabel_       = nullptr;
    QProgressBar* progressBar_       = nullptr;
    QLabel*       summaryLabel_      = nullptr;

    // ── 历史 Tab ──
    QListWidget*  historyList_      = nullptr;
    QPushButton*  deleteHistoryBtn_ = nullptr;

    // ── 备注 Tab ──
    QTextEdit*    noteEdit_ = nullptr;

    // ── 数据 ──
    std::vector<cxlsim::EpochStats> epochData_;
    QString configJson_;
    QList<SessionRecord> history_;
};
