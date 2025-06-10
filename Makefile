PROJDIR := $(realpath $(CURDIR))

SUBDIRS := magnetometer serial

include $(PROJDIR)/subdirs.mk
