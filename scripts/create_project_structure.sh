#!/bin/bash

echo "================================"
echo "Creating CXLMemSim Project Structure"
echo "================================"

PROJECT_ROOT="/home/xiaowu/work/CXLMemSim"
cd "$PROJECT_ROOT" || exit 1

# Backend directories
mkdir -p backend/{ebpf,include,src}
mkdir -p backend/include/{tracer,analyzer,injector,common,topology}
mkdir -p backend/src/{tracer,analyzer,injector,topology}

# Frontend directories
mkdir -p frontend/{widgets,dialogs,models,resources}
mkdir -p frontend/resources/{icons,styles,configs}
mkdir -p frontend/ui

# Common directories
mkdir -p common/{include,src}

# Tests directories
mkdir -p tests/{unit,integration,benchmarks,data}

# Docs directories
mkdir -p docs/{design,api,user_guide,images}

# Config and scripts
mkdir -p configs/examples
mkdir -p scripts

# Third-party
mkdir -p third_party

echo "✓ Directory structure created"

# Create .gitignore if not exists
if [ ! -f .gitignore ]; then
    cat > .gitignore << 'EOF'
# Build artifacts
build/
build-*/
cmake-build-*/
*.o
*.a
*.so
*.so.*
*.dylib

# IDE
.vscode/
.idea/
*.swp
*.swo
*~
.vs/

# BPF
*.bpf.o
vmlinux.h

# Test outputs
test_results/
*.log
Testing/

# Perf data
perf.data
perf.data.old

# Qt
moc_*.cpp
ui_*.h
qrc_*.cpp
*.qm

# User config
local_config.json
user_settings.ini

# OS
.DS_Store
Thumbs.db
EOF
    echo "✓ .gitignore created"
fi

# Create example trace file
cat > tests/data/sample_trace.json << 'EOF'
{
  "metadata": {
    "source": "mock_generator",
    "duration_ms": 1000,
    "sample_rate_hz": 10000
  },
  "events": [
    {
      "timestamp": 1000000000,
      "addr": 16777216,
      "tid": 1234,
      "cpu": 0,
      "l3_miss": true,
      "latency_ns": 150
    },
    {
      "timestamp": 1000001000,
      "addr": 16781312,
      "tid": 1234,
      "cpu": 0,
      "l3_miss": false,
      "latency_ns": 10
    }
  ]
}
EOF
echo "✓ Sample trace data created"

# Create example topology config
cat > configs/examples/simple_cxl.json << 'EOF'
{
  "topology": {
    "name": "Simple CXL Setup",
    "description": "One CXL device directly connected to host",
    "root_complex": {
      "id": "RC0",
      "local_dram_size_gb": 64,
      "local_dram_latency_ns": 90
    },
    "cxl_devices": [
      {
        "id": "CXL0",
        "type": "Type3",
        "capacity_gb": 128,
        "link_gen": "Gen5",
        "link_width": "x16",
        "bandwidth_gbps": 64,
        "base_latency_ns": 170
      }
    ],
    "connections": [
      {
        "from": "RC0",
        "to": "CXL0",
        "link": "Gen5x16"
      }
    ]
  },
  "memory_policy": {
    "type": "first_touch",
    "local_first_gb": 32
  },
  "simulation": {
    "epoch_ms": 10,
    "enable_congestion_model": true,
    "enable_mlp_optimization": false
  }
}
EOF
echo "✓ Example configuration created"

# Create README
if [ ! -f README.md ]; then
    cat > README.md << 'EOF'
# CXLMemSim - CXL Memory Simulator

A high-fidelity, execution-driven CXL memory simulator using hardware performance counters and eBPF.

## Features

- 🔬 **Precise Tracking**: PEBS-based memory access sampling on physical hardware
- 🖥️ **VM Development**: Mock tracer for development in virtual machines
- 🎨 **Qt GUI**: Professional graphical interface for topology editing and visualization
- 📊 **Multi-topology**: Support for complex CXL fabrics with switches
- ⚡ **High Performance**: Only 3-6x slowdown vs native execution

## Architecture

```
Qt Frontend → Simulator Core → Tracer (PEBS/Mock)
                ↓
          Analyzer + Injector
```

## Quick Start

### Virtual Machine (Development)
```bash
# Install dependencies
./scripts/install_dependencies.sh

# Build with Mock tracer
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)

# Run GUI
./cxlmemsim
```

### Physical Server (Production)
```bash
# Build with PEBS tracer
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)

# Should output: "Using PEBSTracer"
./cxlmemsim --check-tracer
```

## Documentation

- [Architecture Design](docs/design/architecture.md)
- [VM to Physical Migration](planning/vm_to_physical_migration_strategy.md)
- [API Reference](docs/api/)
- [User Guide](docs/user_guide/)

## Development Status

🚧 **Under Active Development**

- [x] Project structure
- [x] Tracer abstraction layer
- [ ] Mock tracer implementation
- [ ] PEBS tracer implementation
- [ ] Qt frontend
- [ ] Timing analyzer
- [ ] Bandwidth model

## Requirements

- Linux Kernel 5.10+
- GCC 9+ or Clang 10+
- CMake 3.20+
- Qt 6.2+
- Intel CPU with PEBS support (for physical hardware)

## License

TBD

## Authors

Jiang Tao - CXL Memory Simulation Platform
EOF
    echo "✓ README.md created"
fi

echo ""
echo "================================"
echo "Project Structure Created!"
echo "================================"
echo ""
echo "Next steps:"
echo "1. Run: tree -L 2  (to see the structure)"
echo "2. Initialize Git: git init"
echo "3. Start coding!"
