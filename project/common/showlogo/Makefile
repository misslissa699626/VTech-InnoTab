TOPDIR := ../../../
include $(TOPDIR)sdk/build/core/config.mak


GOAL     = showlogo


CFLAGS += -I$(SDK_DIR)/include
CFLAGS += -I$(PRODUCT_DIR)
CFLAGS += -I.
CFLAGS += -g

OBJS := \
	showlogo.o

LIBS := \
	$(SDK_DIR)/lib/libboard.a \
	$(SDK_DIR)/lib/libdiag.a


.PHONY: all
all: $(GOAL)
	$(MSG_SPLIT_LINE)
	@$(MKDIR) -p $(PRODUCT_DIR)/rootfs
	@$(CP) -u $(GOAL) $(PRODUCT_DIR)/rootfs/$(GOAL)


$(GOAL): $(OBJS) $(LIBS)
	$(MSG_SPLIT_LINE)
	$(MSG_LINKING)
	$(TARGET_CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)
	$(MSG_GOAL_OK)


.PHONY: clean
clean:
	$(MSG_SPLIT_LINE)
	$(RM) -rf $(GOAL) $(OBJS) $(OBJS:.o=.d)
	$(RM) -f $(PRODUCT_DIR)/rootfs/$(GOAL)


-include $(OBJS:.o=.d)
