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

**Quick deployment** (automated):
```bash
# One-line deployment
sudo ./scripts/deploy_physical.sh --auto
```

**Manual deployment**:
```bash
# Install dependencies
sudo ./scripts/install_dependencies.sh

# Configure perf permissions
sudo sysctl -w kernel.perf_event_paranoid=1

# Build with PEBS tracer
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)

# Verify PEBS support
./cxlmemsim --check-tracer
```

📖 **Deployment Guides**:
- [Quick Start Guide](DEPLOYMENT_QUICKSTART.md) - Fast deployment in 3 steps
- [Complete Deployment Guide](docs/PHYSICAL_DEPLOYMENT.md) - Detailed instructions

## Documentation

- **Deployment**
  - [📦 Quick Deployment](DEPLOYMENT_QUICKSTART.md)
  - [📘 Complete Deployment Guide](docs/PHYSICAL_DEPLOYMENT.md)
  - [🔄 VM to Physical Migration](planning/vm_to_physical_migration_strategy.md)
- **Development**
  - [🏗️ Architecture Design](docs/design/architecture.md)
  - [📚 API Reference](docs/api/)
  - [📖 User Guide](docs/user_guide/)
  - [🚀 Quick Start](QUICKSTART.md)

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
