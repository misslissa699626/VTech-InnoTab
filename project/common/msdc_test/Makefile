TOPDIR := ../../../
include $(TOPDIR)sdk/build/core/config.mak


GOAL     = msdc


CFLAGS += -I$(SDK_DIR)/include
CFLAGS += -I$(PRODUCT_DIR)
CFLAGS += -I.
CFLAGS += -g


OBJS := \
	msdc.o

LIBS := \


.PHONY: all
all: $(GOAL)
	$(MSG_SPLIT_LINE)
	@$(MKDIR) -p $(PRODUCT_DIR)/rootfs
	@$(CP) -u $(GOAL) $(PRODUCT_DIR)/rootfs/$(GOAL)


$(GOAL): $(OBJS) $(LIBS)
	$(MSG_SPLIT_LINE)
	$(MSG_LINKING)
	$(TARGET_CC) -o $@ $(OBJS) $(LIBS)
	$(MSG_GOAL_OK)


.PHONY: clean
clean:
	$(MSG_SPLIT_LINE)
	$(RM) -rf $(GOAL) $(OBJS) $(OBJS:.o=.d)
	$(RM) -f $(PRODUCT_DIR)/rootfs/$(GOAL)


-include $(OBJS:.o=.d)
