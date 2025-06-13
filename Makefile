PROJDIR := $(realpath $(CURDIR))

SUBDIRS := gpio magnetometer pwm serial

include $(PROJDIR)/subdirs.mk
