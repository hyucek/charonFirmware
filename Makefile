# Simple makefile for simple example
PROGRAM=firmware
EXTRA_COMPONENTS = extras/pwm extras/rboot-ota extras/mbedtls
LIBS = hal gcc c m
include ../../common.mk
