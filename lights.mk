PRODUCT_PACKAGES += \
	android.hardware.lights-service

# TODO: LOCAL_PATH doesn't work here

# How about
ALH_PATH := $(patsubst $(CURDIR)/%,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))

BOARD_VENDOR_SEPOLICY_DIRS += $(ALH_PATH)/sepolicy
# ?
