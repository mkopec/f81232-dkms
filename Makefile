install:
	cp -r src/* /usr/src/dasharo-acpi-0.0.1 && dkms install -m dasharo-acpi -v 0.0.1

