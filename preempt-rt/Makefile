TARGET = preempt-rt

PROJDIR := $(realpath $(CURDIR)/..)
BUILDDIR := $(PROJDIR)/build/examples/$(TARGET)

C_SOURCES = main.c

LDFLAGS += -lm -pthread

include $(PROJDIR)/common.mk
