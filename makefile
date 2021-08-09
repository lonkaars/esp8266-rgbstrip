#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := esp8266-rgbstrip

EXTRA_COMPONENT_DIRS = $(IDF_PATH)/examples/common_components/protocol_examples_common

include $(IDF_PATH)/make/project.mk

compile_commands:
	bear -- make

