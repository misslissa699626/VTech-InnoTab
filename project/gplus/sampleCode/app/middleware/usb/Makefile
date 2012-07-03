TOPDIR := ../../../../../../
include $(TOPDIR)sdk/build/core/config.mak


GOAL     = usb


CFLAGS += -I$(SDK_DIR)/include
CFLAGS += -I$(PRODUCT_DIR)
CFLAGS += -I.
CFLAGS += -g



all: $(GOAL)
	$(MSG_SPLIT_LINE)
	@echo Copy usb to $(PRODUCT_DIR)/system/app...
	@$(MKDIR) -p $(PRODUCT_DIR)/system/app
	@$(CP) $(GOAL) $(PRODUCT_DIR)/system/app/$(GOAL)

$(GOAL): $(OBJS) $(LIBS)
	$(MSG_GOAL_OK)


.PHONY: clean
clean:
	$(MSG_SPLIT_LINE)


-include $(OBJS:.o=.d)
