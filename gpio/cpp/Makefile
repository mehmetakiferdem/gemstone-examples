TARGET = gpio

PROJDIR := $(realpath $(CURDIR)/../..)
BUILDDIR := $(PROJDIR)/build/$(TARGET)/cpp

CXX_SOURCES = gpio_controller.cpp main.cpp

LDFLAGS += -lgpiod

include $(PROJDIR)/common.mk
