# Simple makefile for simple example
PROGRAM=firmware
EXTRA_COMPONENTS = extras/pwm extras/rboot-ota extras/mbedtls extras/paho_mqtt_c
LIBS = hal gcc c m
include ../../common.mk
