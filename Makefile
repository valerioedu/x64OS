all: check_qemu submake

submake:
	$(MAKE) -C boot

check_qemu:
	if ! command -v qemu-system-x86_64 >/dev/null 2>&1; then \
		echo "qemu is not installed. Please install qemu."; \
		exit 1; \
	fi

clean:
	$(MAKE) -C boot clean