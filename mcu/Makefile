TARGET = mcu

PROJDIR := $(realpath $(CURDIR)/..)
BUILDDIR := $(PROJDIR)/build/examples/$(TARGET)

TI_REQUIRED_TOOLS := MCU_PLUS_SDK_PATH SYSCFG_PATH CGT_TI_C7000_PATH CGT_TI_ARM_CLANG_PATH

TI_OUTNAME = ipc_rpmsg_echo_linux

TI_TARGETS := \
	j722s-evm/c75ss0-0_freertos/ti-c7000 \
	j722s-evm/c75ss1-0_freertos/ti-c7000 \
	j722s-evm/main-r5fss0-0_freertos/ti-arm-clang \
	j722s-evm/mcu-r5fss0-0_freertos/ti-arm-clang \
	j722s-evm/wkup-r5fss0-0_freertos/ti-arm-clang

all: check-tools
	@$(foreach ti_target,$(TI_TARGETS), \
		$(MAKE) -C "$(ti_target)" -f makefile all; \
		mkdir -p "$(BUILDDIR)/$(ti_target)"; \
		cp "$(ti_target)/$(TI_OUTNAME).$(PROFILE).out" "$(BUILDDIR)/$(ti_target)";)

clean: check-tools
	@$(foreach ti_target,$(TI_TARGETS), \
		$(MAKE) -C "$(ti_target)" -f makefile clean; \
		rm -rf "$(BUILDDIR)/$(ti_target)";)

run-ti: check-tools
	$(MAKE) -C "$(TI_TARGET)" -f makefile $(TI_COMMAND);

check-tools:
	@$(foreach tool,$(TI_REQUIRED_TOOLS), \
		if [ ! -d "$($(tool))" ] || [ -z "$(shell ls -A "$($(tool))")" ]; then \
			echo "ERROR: $($(tool)) directory does not exist or it is empty"; \
			exit 1; \
		fi;)

.PHONY: all clean check-tools
