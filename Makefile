#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := smart-cottage-esp

pre-flash:
	cp -f $(COTTAGE_CONFIG) main/cottage_config.h

include $(IDF_PATH)/make/project.mk
