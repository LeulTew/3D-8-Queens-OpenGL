TOP_DIR        := /mnt/c/Users/Leul/Downloads/Telegram Desktop/Senior/Sem 1/CS489 - CG/Proj/8 Queen 6.0/freetype-2.10.1
OBJ_DIR        := /mnt/c/Users/Leul/Downloads/Telegram Desktop/Senior/Sem 1/CS489 - CG/Proj/8 Queen 6.0/freetype-2.10.1
OBJ_BUILD      := $(OBJ_DIR)
DOC_DIR        := $(OBJ_DIR)/reference
FT_LIBTOOL_DIR := $(OBJ_DIR)
ifndef FT2DEMOS
  include $(TOP_DIR)/Makefile
else
  TOP_DIR_2 := $(TOP_DIR)/../ft2demos
  PROJECT   := freetype
  CONFIG_MK := $(OBJ_DIR)/config.mk
  include $(TOP_DIR_2)/Makefile
endif
