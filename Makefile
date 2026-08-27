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
#
#  WARNING: every recipe line below (the indented g++/rm lines)
#           MUST start with a real TAB, not spaces. This is Make's
#           single most infamous gotcha. If you retype this, check
#           the indentation is a tab.
# ============================================================

# ---- Compiler & flags shared by both targets ---------------
CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2

# ---- image_capture -----------------------------------------
IC_BIN    := image_capture/image_capture
IC_SRC    := image_capture/src/main.cpp \
             image_capture/src/StereoCapture.cpp
IC_INC    := -Iimage_capture/include \
             -I/usr/local/zed/include \
             -I/usr/local/cuda/include
IC_LIBDIR := -L/usr/local/zed/lib \
             -L/usr/local/cuda/lib64

# >>> FILL THIS FROM YOUR OWN WORKING g++ COMMAND <<<
# These are the *typical* ZED + CUDA libs, but I don't have your
# exact link line, so verify these match the -l flags you used.
IC_LIBS   := -lsl_zed -lcudart

# ---- supervisor --------------------------------------------
SV_BIN    := supervisor/supervisor
SV_SRC    := supervisor/src/main.cpp
SV_INC    := -Isupervisor/include
SV_LIBS   := -lgpiod 

# ---- rules -------------------------------------------------
.PHONY: all clean

all: $(IC_BIN) $(SV_BIN)

$(IC_BIN): $(IC_SRC)
	$(CXX) $(CXXFLAGS) $(IC_SRC) $(IC_INC) $(IC_LIBDIR) $(IC_LIBS) -o $@

$(SV_BIN): $(SV_SRC)
	$(CXX) $(CXXFLAGS) $(SV_SRC) $(SV_INC) $(SV_LIBS) -o $@

clean:
	rm -f $(IC_BIN) $(SV_BIN)
