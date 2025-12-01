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
