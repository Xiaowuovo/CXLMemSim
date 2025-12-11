#!/bin/bash
export QT_QPA_PLATFORM=xcb
export LD_LIBRARY_PATH=/usr/lib/x86_64-linux-gnu:$LD_LIBRARY_PATH
./build/frontend/cxlmemsim_gui
