# ============================================================
#  fish_quality_control — top-level Makefile   (lives at repo root)
#
#  Builds two executables on the Jetson:
#    - image_capture : links the ZED SDK + CUDA
#    - supervisor    : plain C++ (poll / fork / exec), no ZED/CUDA
#
#  Usage:
#    make                 # build everything
#    make image_capture   # build just the capture binary
#    make supervisor      # build just the supervisor
#    make clean           # delete the built binaries
# ============================================================

# ---- Compiler & flags shared by both targets ---------------
CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2
CXXFLAGS += -Icommon/include

# ---- image_capture -----------------------------------------
IC_BIN    := image_capture/image_capture
IC_SRC    := image_capture/src/main.cpp \
             image_capture/src/StereoCapture.cpp
IC_INC    := -Iimage_capture/include \
             -I/usr/local/zed/include \
             -I/usr/local/cuda/include
IC_LIBDIR := -L/usr/local/zed/lib \
             -L/usr/local/cuda/lib64
IC_LIBS   := -lsl_zed -lcudart

# ---- push_capture -----------------------------------------
PC_BIN    := push_captures/push_captures
PC_SRC    := push_captures/src/main.cpp
PC_LIBS   := -lcurl

# ---- supervisor --------------------------------------------
SV_BIN    := supervisor/supervisor
SV_SRC    := supervisor/src/main.cpp \
             supervisor/src/CameraManager.cpp \
             supervisor/src/WaterTank_Manager.cpp \
             supervisor/src/TankProtocol.cpp \
             lib/COM_protocol/core/FrameParser.cpp \
             lib/COM_protocol/linux/LinuxSerialTransport.cpp
SV_INC    := -Isupervisor/include \
             -Ilib/COM_protocol/core \
             -Ilib/COM_protocol/linux
SV_LIBS   := -lgpiod

# ---- rules -------------------------------------------------
.PHONY: all clean

all: $(IC_BIN) $(SV_BIN) $(PC_BIN)

$(IC_BIN): $(IC_SRC)
	$(CXX) $(CXXFLAGS) $(IC_SRC) $(IC_INC) $(IC_LIBDIR) $(IC_LIBS) -o $@

$(SV_BIN): $(SV_SRC)
	$(CXX) $(CXXFLAGS) $(SV_SRC) $(SV_INC) $(SV_LIBS) -o $@

$(PC_BIN): $(PC_SRC)
	$(CXX) $(CXXFLAGS) $(PC_SRC) $(PC_LIBS) -o $@

clean:
	rm -f $(IC_BIN) $(SV_BIN) $(PC_BIN)