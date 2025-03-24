// SPDX-License-Identifier: GPL-2.0+
/*
 * Dasharo ACPI Driver
 *
 * Copyright (C) 2025 3mdeb Sp. z o.o.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/acpi.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/kernel.h>
#include <linux/leds.h>
#include <linux/module.h>
#include <linux/pci_ids.h>
#include <linux/sysfs.h>
#include <linux/types.h>

enum dasharo_feature {
	DASHARO_FEATURE_TEMPERATURE = 0,
	DASHARO_FEATURE_FAN_PWM,
	DASHARO_FEATURE_FAN_TACH,
	DASHARO_FEATURE_FAN_POINTS,
	DASHARO_FEATURE_MAX,
};

enum dasharo_temperature {
	DASHARO_TEMPERATURE_CPU_PACKAGE = 0,
	DASHARO_TEMPERATURE_CPU_CORE,
	DASHARO_TEMPERATURE_GPU,
	DASHARO_TEMPERATURE_BOARD,
	DASHARO_TEMPERATURE_CHASSIS,
	DASHARO_TEMPERATURE_MAX,
};

enum dasharo_fan {
	DASHARO_FAN_CPU = 0,
	DASHARO_FAN_GPU,
	DASHARO_FAN_CHASSIS,
	DASHARO_FAN_MAX,
};


struct dasharo_capability {
	enum dasharo_feature feature;
	union cap {
		enum dasharo_temperature temp;
		enum dasharo_fan fan;
	};
	int index;
};

struct dasharo_data {
	struct acpi_device *acpi_dev;

};

static const struct acpi_device_id device_ids[] = {
	{"DSHR0001", 0},
	{"", 0},
};
MODULE_DEVICE_TABLE(acpi, device_ids);

static int dasharo_get_feature_cap(struct dasharo_data *data, char *method, int feat, int cap)
{
	union acpi_object obj[2];
	struct acpi_object_list obj_list;
	acpi_handle handle;
	acpi_status status;
	unsigned long long ret = 0;

	obj[0].type = ACPI_TYPE_INTEGER;
	obj[0].integer.value = feat;
	obj[1].type = ACPI_TYPE_INTEGER;
	obj[1].integer.value = cap;
	obj_list.count = 2;
	obj_list.pointer = &obj[0];

	handle = acpi_device_handle(data->acpi_dev);
	status = acpi_evaluate_integer(handle, method, &obj_list, &ret);
	if (ACPI_SUCCESS(status))
		return ret;
	return -ENODEV;
}

static int dasharo_add(struct acpi_device *acpi_dev)
{
	struct dasharo_data *data;
	int err;

	data = devm_kzalloc(&acpi_dev->dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;
	acpi_dev->driver_data = data;
	data->acpi_dev = acpi_dev;

	int count;
	pr_info("Dasharo driver capabilities:\n");
	pr_info("  Temperatures");
	count = dasharo_get_feature_cap(data, "GFCP", DASHARO_FEATURE_TEMPERATURE, DASHARO_TEMPERATURE_CPU_PACKAGE);
	if (count) pr_info("    CPU package: %d\n");
	count = dasharo_get_feature_cap(data, "GFCP", DASHARO_FEATURE_TEMPERATURE, DASHARO_TEMPERATURE_CPU_CORE);
	if (count) pr_info("    CPU core: %d\n");
	count = dasharo_get_feature_cap(data, "GFCP", DASHARO_FEATURE_TEMPERATURE, DASHARO_TEMPERATURE_GPU);
	if (count) pr_info("    GPU: %d\n");
	count = dasharo_get_feature_cap(data, "GFCP", DASHARO_FEATURE_TEMPERATURE, DASHARO_TEMPERATURE_BOARD);
	if (count) pr_info("    Board: %d\n");
	count = dasharo_get_feature_cap(data, "GFCP", DASHARO_FEATURE_TEMPERATURE, DASHARO_TEMPERATURE_CHASSIS);
	if (count) pr_info("    Chassis: %d\n");

	pr_info("  Fan PWM control");
	count = dasharo_get_feature_cap(data, "GFCP", DASHARO_FEATURE_FAN_PWM, DASHARO_FAN_CPU);
	if (count) pr_info("    CPU: %d\n");
	count = dasharo_get_feature_cap(data, "GFCP", DASHARO_FEATURE_FAN_PWM, DASHARO_FAN_GPU);
	if (count) pr_info("    GPU: %d\n");
	count = dasharo_get_feature_cap(data, "GFCP", DASHARO_FEATURE_FAN_PWM, DASHARO_FAN_CHASSIS);
	if (count) pr_info("    Chassis: %d\n");

	return 0;

error:
	return err;
}

static void dasharo_remove(struct acpi_device *acpi_dev)
{
}

static struct acpi_driver system76_driver = {
	.name = "Dasharo ACPI Driver",
	.class = "hotkey",
	.ids = device_ids,
	.ops = {
		.add = dasharo_add,
		.remove = dasharo_remove,
	},
};
module_acpi_driver(system76_driver);

MODULE_DESCRIPTION("Dasharo ACPI Driver");
MODULE_AUTHOR("Michał Kopeć <michal.kopec@3mdeb.com>");
MODULE_LICENSE("GPL");
