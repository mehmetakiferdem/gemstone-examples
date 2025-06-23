PROJDIR := $(realpath $(CURDIR))

SUBDIRS := camera gpio magnetometer mcu pwm serial

include $(PROJDIR)/subdirs.mk
