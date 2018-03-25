# Simple makefile for simple example
PROGRAM=firmware
SOURCES = ./include/mqtt_client.c ./include/taskMaster.c ./firmware.c
LIBS = hal gcc c m
EXTRA_COMPONENTS = extras/pwm extras/rboot-ota extras/mbedtls extras/paho_mqtt_c
include ../../common.mk
