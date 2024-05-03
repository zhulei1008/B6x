.PHONY : all clean

# lib name
DRVS_NAME  = drvs
OUTPUT_DIR = output

SDK_ROOT   = ../..
DRVS_ROOT  = ..

include $(SDK_ROOT)/core/gnu/Makefile.common
include ./src_list.txt
include ./inc_list.txt

INCLUDES   = ${INC_DIR}

# Optimization flags
OPT        = -Os -g1

# see src_list.txt
DRVS_OBJ   = $(patsubst %.c, %.o, $(C_SRC_FILES))
OBJ_FILE   = $(addprefix $(OUTPUT_DIR)/, $(notdir $(DRVS_OBJ)))

all: $(OUTPUT_DIR) $(DRVS_OBJ)
	$(info Archiving ...)
	@$(AR) -o $(OUTPUT_DIR)/lib$(DRVS_NAME).a $(OBJ_FILE)
	@cp $(OUTPUT_DIR)/lib$(DRVS_NAME).a ../lib

$(DRVS_OBJ): %.o : %.c
	$(info Compiling file: $(notdir $@))
	@$(CC) $^ -c $(INCLUDES) $(CFLAGS) -o $(OUTPUT_DIR)/$(notdir $@)

$(OUTPUT_DIR):
ifneq ($(OUTPUT_DIR), $(wildcard $(OUTPUT_DIR)))
	$(info mkdir: $(notdir $@))
	@mkdir $(OUTPUT_DIR)
endif

clean:
	-$(RM) $(OUTPUT_DIR)
	-$(RM) ../lib/lib$(DRVS_NAME).a
