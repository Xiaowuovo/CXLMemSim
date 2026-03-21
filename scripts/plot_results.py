#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
CXL 内存模拟器 - 实验数据可视化脚本

功能：
  - 读取CSV实验结果
  - 生成论文所需的各类图表
  - 输出高质量PDF/PNG图片

用法：
  python3 scripts/plot_results.py <csv_file> [--output-dir DIR]

示例：
  python3 scripts/plot_results.py results/experiment_results.csv
"""

import sys
import argparse
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib
import numpy as np
from pathlib import Path

# 设置中文字体
matplotlib.rcParams['font.sans-serif'] = ['SimHei', 'DejaVu Sans']
matplotlib.rcParams['axes.unicode_minus'] = False

# 设置论文级别的绘图风格
plt.style.use('seaborn-v0_8-paper')
matplotlib.rcParams['figure.dpi'] = 300
matplotlib.rcParams['savefig.dpi'] = 300
matplotlib.rcParams['font.size'] = 10

def load_data(csv_file):
    """加载实验数据"""
    print(f"加载数据: {csv_file}")
    df = pd.read_csv(csv_file)
    print(f"  共 {len(df)} 条实验记录")
    return df

def plot_latency_comparison(df, output_dir):
    """图5-1: 延迟敏感性分析（柱状图）"""
    print("生成图表: 延迟敏感性分析...")
    
    # 筛选延迟相关实验
    latency_exp = df[df['实验名称'].str.contains('延迟|基准')]
    
    fig, ax = plt.subplots(figsize=(10, 6))
    
    x = np.arange(len(latency_exp))
    bars = ax.bar(x, latency_exp['平均延迟(ns)'], color='steelblue', alpha=0.8)
    
    # 设置标签
    ax.set_xlabel('内存配置', fontsize=12, fontweight='bold')
    ax.set_ylabel('平均访问延迟 (ns)', fontsize=12, fontweight='bold')
    ax.set_title('CXL 延迟敏感性分析', fontsize=14, fontweight='bold')
    ax.set_xticks(x)
    ax.set_xticklabels(latency_exp['实验名称'], rotation=15, ha='right')
    ax.grid(axis='y', alpha=0.3, linestyle='--')
    
    # 添加数值标签
    for i, (idx, row) in enumerate(latency_exp.iterrows()):
        value = row['平均延迟(ns)']
        ax.text(i, value + 5, f'{value:.1f}', ha='center', va='bottom', fontweight='bold')
    
    plt.tight_layout()
    output_file = output_dir / 'fig5_1_latency_comparison.pdf'
    plt.savefig(output_file, bbox_inches='tight')
    plt.savefig(output_dir / 'fig5_1_latency_comparison.png', bbox_inches='tight')
    print(f"  已保存: {output_file}")
    plt.close()

def plot_bandwidth_impact(df, output_dir):
    """图5-2: 带宽影响分析（折线图）"""
    print("生成图表: 带宽影响分析...")
    
    # 筛选带宽相关实验
    bandwidth_exp = df[df['实验名称'].str.contains('带宽')].sort_values('带宽(GB/s)', ascending=False)
    
    if len(bandwidth_exp) == 0:
        print("  警告: 未找到带宽实验数据")
        return
    
    fig, ax1 = plt.subplots(figsize=(10, 6))
    
    x = bandwidth_exp['带宽(GB/s)'].values
    y1 = bandwidth_exp['平均延迟(ns)'].values
    
    # 主Y轴：延迟
    color1 = 'tab:blue'
    ax1.set_xlabel('CXL 链路带宽 (GB/s)', fontsize=12, fontweight='bold')
    ax1.set_ylabel('平均访问延迟 (ns)', color=color1, fontsize=12, fontweight='bold')
    line1 = ax1.plot(x, y1, 'o-', color=color1, linewidth=2, markersize=8, label='平均延迟')
    ax1.tick_params(axis='y', labelcolor=color1)
    ax1.grid(alpha=0.3, linestyle='--')
    
    # 次Y轴：性能下降百分比
    ax2 = ax1.twinx()
    baseline_latency = bandwidth_exp.iloc[0]['平均延迟(ns)']
    y2 = (y1 - baseline_latency) / baseline_latency * 100
    
    color2 = 'tab:red'
    ax2.set_ylabel('性能下降 (%)', color=color2, fontsize=12, fontweight='bold')
    line2 = ax2.plot(x, y2, 's--', color=color2, linewidth=2, markersize=8, label='性能下降')
    ax2.tick_params(axis='y', labelcolor=color2)
    
    # 添加图例
    lines = line1 + line2
    labels = [l.get_label() for l in lines]
    ax1.legend(lines, labels, loc='upper left')
    
    plt.title('CXL 带宽对延迟的影响', fontsize=14, fontweight='bold')
    fig.tight_layout()
    
    output_file = output_dir / 'fig5_2_bandwidth_impact.pdf'
    plt.savefig(output_file, bbox_inches='tight')
    plt.savefig(output_dir / 'fig5_2_bandwidth_impact.png', bbox_inches='tight')
    print(f"  已保存: {output_file}")
    plt.close()

def plot_congestion_comparison(df, output_dir):
    """图5-3: 拥塞模型对比（堆叠柱状图）"""
    print("生成图表: 拥塞模型对比...")
    
    # 筛选拥塞模型实验
    congestion_exp = df[df['实验名称'].str.contains('拥塞模型')]
    
    if len(congestion_exp) < 2:
        print("  警告: 拥塞模型实验数据不足")
        return
    
    fig, ax = plt.subplots(figsize=(8, 6))
    
    # 假设基础延迟和拥塞延迟（实际应从数据中计算）
    disabled = congestion_exp[congestion_exp['拥塞模型'] == '禁用']
    enabled = congestion_exp[congestion_exp['拥塞模型'] == '启用']
    
    categories = ['拥塞模型禁用', '拥塞模型启用']
    base_latency = [disabled.iloc[0]['延迟(ns)'], enabled.iloc[0]['延迟(ns)']]
    congestion_latency = [
        disabled.iloc[0]['平均延迟(ns)'] - disabled.iloc[0]['延迟(ns)'],
        enabled.iloc[0]['平均延迟(ns)'] - enabled.iloc[0]['延迟(ns)']
    ]
    
    x = np.arange(len(categories))
    width = 0.5
    
    p1 = ax.bar(x, base_latency, width, label='基础延迟', color='steelblue')
    p2 = ax.bar(x, congestion_latency, width, bottom=base_latency, label='拥塞延迟', color='coral')
    
    ax.set_ylabel('延迟 (ns)', fontsize=12, fontweight='bold')
    ax.set_title('拥塞模型效果对比', fontsize=14, fontweight='bold')
    ax.set_xticks(x)
    ax.set_xticklabels(categories)
    ax.legend()
    ax.grid(axis='y', alpha=0.3, linestyle='--')
    
    # 添加总延迟标注
    for i in range(len(categories)):
        total = base_latency[i] + congestion_latency[i]
        ax.text(i, total + 2, f'{total:.1f}', ha='center', va='bottom', fontweight='bold')
    
    plt.tight_layout()
    output_file = output_dir / 'fig5_3_congestion_comparison.pdf'
    plt.savefig(output_file, bbox_inches='tight')
    plt.savefig(output_dir / 'fig5_3_congestion_comparison.png', bbox_inches='tight')
    print(f"  已保存: {output_file}")
    plt.close()

def plot_mlp_comparison(df, output_dir):
    """图5-4: MLP优化效果对比"""
    print("生成图表: MLP优化效果对比...")
    
    mlp_exp = df[df['实验名称'].str.contains('MLP优化')]
    
    if len(mlp_exp) < 2:
        print("  警告: MLP实验数据不足")
        return
    
    fig, ax = plt.subplots(figsize=(8, 6))
    
    disabled = mlp_exp[mlp_exp['MLP优化'] == '禁用']
    enabled = mlp_exp[mlp_exp['MLP优化'] == '启用']
    
    categories = ['MLP禁用', 'MLP启用']
    values = [disabled.iloc[0]['平均延迟(ns)'], enabled.iloc[0]['平均延迟(ns)']]
    colors = ['lightcoral', 'lightgreen']
    
    bars = ax.bar(categories, values, color=colors, alpha=0.8)
    
    ax.set_ylabel('平均延迟 (ns)', fontsize=12, fontweight='bold')
    ax.set_title('内存级并行（MLP）优化效果', fontsize=14, fontweight='bold')
    ax.grid(axis='y', alpha=0.3, linestyle='--')
    
    # 添加数值和百分比标签
    improvement = (values[0] - values[1]) / values[0] * 100
    for i, v in enumerate(values):
        label = f'{v:.1f}' if i == 0 else f'{v:.1f}\n(-{improvement:.1f}%)'
        ax.text(i, v + 2, label, ha='center', va='bottom', fontweight='bold')
    
    plt.tight_layout()
    output_file = output_dir / 'fig5_4_mlp_comparison.pdf'
    plt.savefig(output_file, bbox_inches='tight')
    plt.savefig(output_dir / 'fig5_4_mlp_comparison.png', bbox_inches='tight')
    print(f"  已保存: {output_file}")
    plt.close()

def plot_cxl_access_ratio(df, output_dir):
    """图5-5: CXL访问比例饼图"""
    print("生成图表: CXL访问比例...")
    
    # 选择一个典型实验
    exp = df[df['实验名称'].str.contains('Gen5')].iloc[0]
    
    fig, ax = plt.subplots(figsize=(8, 8))
    
    local_accesses = exp['总访问次数'] - exp['CXL访问次数']
    cxl_accesses = exp['CXL访问次数']
    
    sizes = [local_accesses, cxl_accesses]
    labels = [f'本地内存访问\n{local_accesses:,} ({100-exp["CXL访问比例(%)"]:.1f}%)',
              f'CXL内存访问\n{cxl_accesses:,} ({exp["CXL访问比例(%)"]:.1f}%)']
    colors = ['lightblue', 'lightcoral']
    explode = (0.05, 0.05)
    
    ax.pie(sizes, explode=explode, labels=labels, colors=colors,
           autopct='', startangle=90, textprops={'fontsize': 11, 'fontweight': 'bold'})
    ax.set_title(f'内存访问分布 ({exp["实验名称"]})', fontsize=14, fontweight='bold')
    
    plt.tight_layout()
    output_file = output_dir / 'fig5_5_access_ratio.pdf'
    plt.savefig(output_file, bbox_inches='tight')
    plt.savefig(output_dir / 'fig5_5_access_ratio.png', bbox_inches='tight')
    print(f"  已保存: {output_file}")
    plt.close()

def generate_summary_table(df, output_dir):
    """生成实验结果汇总表（LaTeX格式）"""
    print("生成实验结果汇总表...")
    
    # 选择关键列
    summary = df[['实验名称', 'CXL容量(GB)', '带宽(GB/s)', '延迟(ns)', 
                  '平均延迟(ns)', 'CXL访问比例(%)', '总注入延迟(ms)']]
    
    # 导出为LaTeX表格
    latex_table = summary.to_latex(index=False, float_format='%.2f', 
                                    caption='实验配置与结果汇总',
                                    label='tab:experiment_results')
    
    output_file = output_dir / 'table_results.tex'
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write(latex_table)
    print(f"  已保存: {output_file}")
    
    # 同时保存为Markdown表格
    md_table = summary.to_markdown(index=False, floatfmt='.2f')
    output_file = output_dir / 'table_results.md'
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write("# 实验结果汇总表\n\n")
        f.write(md_table)
    print(f"  已保存: {output_file}")

def main():
    parser = argparse.ArgumentParser(description='CXL模拟器实验数据可视化')
    parser.add_argument('csv_file', help='实验结果CSV文件')
    parser.add_argument('--output-dir', default='./figures', help='图片输出目录')
    
    args = parser.parse_args()
    
    # 创建输出目录
    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    
    print("=" * 60)
    print("  CXL 内存模拟器 - 实验数据可视化")
    print("=" * 60)
    print()
    
    # 加载数据
    df = load_data(args.csv_file)
    print()
    
    # 生成各类图表
    plot_latency_comparison(df, output_dir)
    plot_bandwidth_impact(df, output_dir)
    plot_congestion_comparison(df, output_dir)
    plot_mlp_comparison(df, output_dir)
    plot_cxl_access_ratio(df, output_dir)
    
    # 生成汇总表
    generate_summary_table(df, output_dir)
    
    print()
    print("=" * 60)
    print("  所有图表生成完成！")
    print("=" * 60)
    print(f"\n图片保存位置: {output_dir.absolute()}")
    print("\n可用于论文的图表:")
    print("  - fig5_1_latency_comparison.pdf")
    print("  - fig5_2_bandwidth_impact.pdf")
    print("  - fig5_3_congestion_comparison.pdf")
    print("  - fig5_4_mlp_comparison.pdf")
    print("  - fig5_5_access_ratio.pdf")
    print("  - table_results.tex (LaTeX表格)")
    print("  - table_results.md (Markdown表格)")
    print()

if __name__ == '__main__':
    main()
