TARGET = mcu

PROJDIR := $(realpath $(CURDIR)/..)
BUILDDIR := $(PROJDIR)/build/examples/$(TARGET)

TI_REQUIRED_TOOLS := MCU_PLUS_SDK_PATH SYSCFG_PATH CGT_TI_C7000_PATH CGT_TI_ARM_CLANG_PATH

TI_PROJECT_DIRS := \
	ipc_rpmsg_echo_linux/j722s-evm/c75ss0-0_freertos/ti-c7000 \
	ipc_rpmsg_echo_linux/j722s-evm/c75ss1-0_freertos/ti-c7000 \
	ipc_rpmsg_echo_linux/j722s-evm/main-r5fss0-0_freertos/ti-arm-clang \
	ipc_rpmsg_echo_linux/j722s-evm/mcu-r5fss0-0_freertos/ti-arm-clang \
	ipc_rpmsg_echo_linux/j722s-evm/wkup-r5fss0-0_freertos/ti-arm-clang \

all: check-tools
	@$(foreach ti_project_dir,$(TI_PROJECT_DIRS), \
		$(eval ti_project_name := $(firstword $(subst /, ,$(ti_project_dir)))) \
		$(eval ti_out_name := $(subst /,.,$(ti_project_dir))) \
		$(MAKE) -C "$(ti_project_dir)" -f makefile all; \
		mkdir -p "$(BUILDDIR)/$(ti_project_name)"; \
		cp "$(ti_project_dir)/$(ti_project_name).$(PROFILE).out" "$(BUILDDIR)/$(ti_project_name)/$(ti_out_name).$(PROFILE).out";)

clean: check-tools
	@$(foreach ti_project_dir,$(TI_PROJECT_DIRS), \
		$(eval ti_project_name := $(firstword $(subst /, ,$(ti_project_dir)))) \
		$(MAKE) -C "$(ti_project_dir)" -f makefile clean; \
		rm -rf "$(BUILDDIR)/$(ti_project_name)";)

run-ti: check-tools
	$(MAKE) -C "$(TI_TARGET)" -f makefile $(TI_COMMAND);

check-tools:
	@$(foreach tool,$(TI_REQUIRED_TOOLS), \
		if [ ! -d "$($(tool))" ] || [ -z "$(shell ls -A "$($(tool))")" ]; then \
			echo "ERROR: $($(tool)) directory does not exist or it is empty"; \
			exit 1; \
		fi;)

.PHONY: all clean run-ti check-tools
