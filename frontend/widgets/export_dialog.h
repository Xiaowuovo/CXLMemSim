#ifndef EXPORT_DIALOG_H
#define EXPORT_DIALOG_H

#include <QDialog>
#include <QCheckBox>
#include <QRadioButton>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include <vector>
#include "analyzer/timing_analyzer.h"

/**
 * @brief 导出报告对话框
 * 
 * 功能：
 * 1. 实验数据流导出 (CSV) - 包含完整数据字典
 * 2. 拓扑与配置快照 (JSON)
 * 3. 可视化图表导出 (PNG)
 */
class ExportDialog : public QDialog {
    Q_OBJECT

public:
    enum ExportRange {
        ALL_EPOCHS,         // 全部记录
        RECENT_EPOCHS       // 最近N个Epoch
    };

    struct ExportOptions {
        bool exportCSV;
        bool exportJSON;
        bool exportPNG;
        ExportRange range;
        int recentCount;    // 当range=RECENT_EPOCHS时有效
        
        ExportOptions() 
            : exportCSV(true), exportJSON(false), exportPNG(false)
            , range(ALL_EPOCHS), recentCount(10000) {}
    };

    explicit ExportDialog(QWidget *parent = nullptr);
    ~ExportDialog() = default;

    // 设置导出数据源
    void setEpochData(const std::vector<cxlsim::EpochStats>& data);
    void setConfigData(const QString& configJson);
    
    // 获取导出选项
    ExportOptions getExportOptions() const;

signals:
    void exportRequested(const ExportOptions& options);

private slots:
    void onExport();
    void onRangeChanged();

private:
    void setupUI();
    QString generateCSV(const std::vector<cxlsim::EpochStats>& data, int startIdx) const;
    void saveFile(const QString& content, const QString& filter, const QString& defaultName);
    void createZipAndDownload(const QStringList& filePaths);

    // UI组件
    QRadioButton* allEpochsRadio_;
    QRadioButton* recentEpochsRadio_;
    QSpinBox* recentCountSpin_;
    
    QCheckBox* csvCheckbox_;
    QCheckBox* jsonCheckbox_;
    QCheckBox* pngCheckbox_;
    
    QPushButton* exportBtn_;
    QPushButton* cancelBtn_;
    QLabel* statusLabel_;
    QProgressBar* progressBar_;
    
    // 数据
    std::vector<cxlsim::EpochStats> epochData_;
    QString configJson_;
};

#endif // EXPORT_DIALOG_H
