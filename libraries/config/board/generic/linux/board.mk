export ATMOSPHERE_DEFINES  += -DATMOSPHERE_BOARD_GENERIC_LINUX
export ATMOSPHERE_SETTINGS +=
export ATMOSPHERE_CFLAGS   +=
export ATMOSPHERE_CXXFLAGS +=
export ATMOSPHERE_ASFLAGS  +=

ifeq ($(strip $(ATMOSPHERE_COMPILER_NAME)),clang)
export ATMOSPHERE_CXXFLAGS += -stdlib=libc++

ifeq ($(strip $(ATMOSPHERE_ARCH_NAME)),x64)
export ATMOSPHERE_SETTINGS += -target x86_64-pc-linux-gnu
else ifeq ($(strip $(ATMOSPHERE_ARCH_NAME)),arm64)
export ATMOSPHERE_SETTINGS += -target aarch64-linux-gnu
endif

endif

# TODO: Better way of doing this?
export ATMOSPHERE_CXXFLAGS += -I/opt/libjpeg-turbo/include