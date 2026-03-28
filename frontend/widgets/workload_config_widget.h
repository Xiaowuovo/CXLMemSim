/**
 * @file workload_config_widget.h
 * @brief 负载配置面板 (科研关键：Trace-Driven + Synthetic Traffic)
 */

#pragma once

#include <QWidget>
#include <QGroupBox>
#include <QRadioButton>
#include <QLineEdit>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include "config_parser.h"

class WorkloadConfigWidget : public QWidget {
    Q_OBJECT

public:
    explicit WorkloadConfigWidget(QWidget *parent = nullptr);
    ~WorkloadConfigWidget();

    // Get/Set workload configuration
    cxlsim::WorkloadConfig getWorkloadConfig() const;
    void setWorkloadConfig(const cxlsim::WorkloadConfig& config);

    // Validation
    bool isWorkloadValid() const;
    QString getValidationError() const;

signals:
    void workloadChanged();
    void validationStatusChanged(bool valid);

private slots:
    void onModeChanged();
    void onBrowseTraceFile();
    void onParameterChanged();
    void validateAndEmit();

private:
    void setupUI();
    void updateUIState();

    // Mode selection
    QRadioButton* syntheticModeBtn_;
    QRadioButton* traceModeBtn_;

    // Trace-driven controls
    QGroupBox* traceGroup_;
    QLineEdit* tracePathEdit_;
    QPushButton* browsBtn_;
    QLabel* traceStatusLabel_;

    // Synthetic traffic controls
    QGroupBox* syntheticGroup_;
    QComboBox* patternCombo_;
    QDoubleSpinBox* readRatioSpin_;
    QDoubleSpinBox* injectionRateSpin_;
    QSpinBox* workingSetSpin_;
    QSpinBox* strideSpin_;
    QDoubleSpinBox* durationSpin_;
    QSpinBox* threadsSpin_;

    // Validation state
    mutable QString lastError_;
};
