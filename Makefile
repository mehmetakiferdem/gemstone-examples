PROJDIR := $(realpath $(CURDIR))

SUBDIRS := gpio magnetometer serial

include $(PROJDIR)/subdirs.mk
