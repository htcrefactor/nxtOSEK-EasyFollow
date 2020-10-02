# Target specific macros
TARGET = EasyFollow
TARGET_SOURCES := \
	EasyFollow.c
TOPPERS_OSEK_OIL_SOURCE := ./EasyFollow.oil

O_PATH ?= build

include ~/nxtOSEK/ecrobot/ecrobot.mak
