# CXLMemSim Qt Frontend

Qt6-based graphical user interface for CXLMemSim.

## Features

### 🎨 Visual Topology Editor
- Drag-and-drop component placement
- Visual representation of:
  - Root Complex (blue)
  - Switches (green)
  - CXL Devices (orange)
- Connection visualization with bandwidth/latency labels
- Interactive node selection

### ⚙️ Configuration Manager
- Tree view of configuration
- Add/remove devices and switches
- Edit parameters inline
- Load/save JSON configurations

### 📊 Performance Metrics Panel
- Real-time statistics display:
  - Current epoch number
  - Total memory accesses
  - L3 miss count and rate
  - CXL access count
  - Average latency
  - Total injected delay
- Color-coded miss rate indicator:
  - Green: <20% (good)
  - Yellow: 20-50% (moderate)
  - Red: >50% (high)

### 📝 Log Viewer
- Real-time event logging
- Simulation status messages
- Error reporting

## Building

### Prerequisites

Install Qt6:
```bash
sudo ./scripts/install_qt6.sh
```

Or manually:
```bash
sudo apt install qt6-base-dev qt6-tools-dev
```

### Compile

```bash
cd build
cmake .. -DBUILD_GUI=ON
make cxlmemsim_gui
```

### Run

```bash
./cxlmemsim_gui
```

## Architecture

### Main Window (`mainwindow.h/cpp`)
- Application entry point
- Menu bar and toolbar
- Dock widget management
- Backend integration (TimingAnalyzer)

### Topology Editor (`widgets/topology_editor_widget.h/cpp`)
- QGraphicsView-based editor
- ComponentItem: visual representation of nodes
- LinkItem: visual representation of connections
- Auto-layout for simple topologies

### Config Tree (`widgets/config_tree_widget.h/cpp`)
- QTreeWidget-based configuration view
- Hierarchical display of:
  - Root Complex
  - Switches
  - CXL Devices
  - Connections
  - Simulation parameters
- Add/remove functionality

### Metrics Panel (`widgets/metrics_panel.h/cpp`)
- Real-time performance display
- LCD display for epoch number
- Styled labels for metrics
- Progress bar for miss rate

## Usage

### Create New Configuration
1. File → New Configuration
2. Add devices using "Add Device" button
3. (Optional) Add switches
4. Save: File → Save Configuration

### Load Existing Configuration
1. File → Open Configuration
2. Select JSON file
3. View in topology editor and config tree

### Run Simulation
1. Configure or load a configuration
2. Click "Start Simulation" or F5
3. View real-time metrics
4. Stop: Click "Stop" or F6

### Edit Topology
1. Double-click components to select
2. Drag to reposition (visual only)
3. View connections and labels

## File Structure

```
frontend/
├── main.cpp                    # Qt application entry
├── mainwindow.h/cpp            # Main window
├── widgets/
│   ├── topology_editor_widget.h/cpp   # Visual editor
│   ├── config_tree_widget.h/cpp       # Config tree
│   └── metrics_panel.h/cpp            # Metrics display
└── CMakeLists.txt              # Qt build configuration
```

## Keyboard Shortcuts

- `Ctrl+N` - New Configuration
- `Ctrl+O` - Open Configuration
- `Ctrl+S` - Save Configuration
- `Ctrl+Shift+S` - Save As
- `F5` - Start Simulation
- `F6` - Stop Simulation
- `Ctrl+Q` - Quit

## Future Enhancements

### Planned Features
- [ ] Real-time latency charts (QCustomPlot)
- [ ] Bandwidth utilization graphs
- [ ] Trace file visualization
- [ ] Advanced topology editing (add/remove connections)
- [ ] Export topology as image
- [ ] Configuration templates
- [ ] Multi-window support
- [ ] Dark mode theme

### Integration Improvements
- [ ] Live tracer control
- [ ] Pause/resume simulation
- [ ] Step-by-step execution
- [ ] Breakpoints on specific events
- [ ] Export statistics to CSV

## Development Notes

### Qt Version
Requires Qt6 (tested with 6.2+)

### MOC (Meta-Object Compiler)
CMake automatically handles MOC for:
- Q_OBJECT classes
- Signals/slots

### Styling
Uses native Qt widgets with minimal custom styling for cross-platform compatibility.

### Thread Safety
- GUI updates from simulation thread use Qt signals/slots
- TimingAnalyzer runs in separate thread
- Metrics updated via QTimer (1s interval)

## Troubleshooting

### Qt6 not found
```bash
sudo apt install qt6-base-dev
```

### MOC errors
```bash
# Clean and rebuild
cd build
rm -rf *
cmake .. -DBUILD_GUI=ON
make cxlmemsim_gui
```

### Linking errors
Ensure backend and common libraries are built first:
```bash
make backend_core common
make cxlmemsim_gui
```

## License

Part of the CXLMemSim project.
