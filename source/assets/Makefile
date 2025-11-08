#
#	assets/Makefile
#

OBJCOPY		= objcopy

# 资源文件
ASSET_FILES	= $(shell find . -type f -not -name Makefile -not -name "*.obj")
TARGETS		= $(addsuffix .obj, $(ASSET_FILES))

.PHONY : default
default : $(TARGETS)

# 资源文件转换规则
%.obj : %
	@$(OBJCOPY) -I binary -O elf32-i386 -B i386 \
		--rename-section .data=.asset,contents,alloc,load,readonly,data \
		$< $@
	@echo "\tOBJCOPY\t$@"

.PHONY : clean
clean:
	@rm -f $(TARGETS)
	@echo "\tRM\t$(TARGETS)"
