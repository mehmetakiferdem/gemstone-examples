PROJDIR := $(realpath $(CURDIR))

SUBDIRS := camera gpio magnetometer pwm serial

include $(PROJDIR)/subdirs.mk
