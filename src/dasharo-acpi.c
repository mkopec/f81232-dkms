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

/****************************************************************************
 ****************************************************************************
 *
 * ACPI Helpers and device model
 *
 ****************************************************************************
 ****************************************************************************/

/*************************************************************************
 * ACPI basic handles
 */

static acpi_handle root_handle;
static acpi_handle ec_handle;

#define DSHRACPI_HANDLE(object, parent, paths...)			\
	static acpi_handle  object##_handle;			\
	static const acpi_handle * const object##_parent __initconst =	\
						&parent##_handle; \
	static char *object##_paths[] __initdata = { paths }

DSHRACPI_HANDLE(gftr, ec, "GFTR");
DSHRACPI_HANDLE(gfcp, ec, "GFCP");

#define TPACPI_MAX_ACPI_ARGS 3
/*************************************************************************
 * ACPI helpers
 */

static int acpi_evalf(acpi_handle handle,
		      int *res, char *method, char *fmt, ...)
{
	char *fmt0 = fmt;
	struct acpi_object_list params;
	union acpi_object in_objs[TPACPI_MAX_ACPI_ARGS];
	struct acpi_buffer result, *resultp;
	union acpi_object out_obj;
	acpi_status status;
	va_list ap;
	char res_type;
	int success;
	int quiet;

	if (!*fmt) {
		pr_err("acpi_evalf() called with empty format\n");
		return 0;
	}

	if (*fmt == 'q') {
		quiet = 1;
		fmt++;
	} else
		quiet = 0;

	res_type = *(fmt++);

	params.count = 0;
	params.pointer = &in_objs[0];

	va_start(ap, fmt);
	while (*fmt) {
		char c = *(fmt++);
		switch (c) {
		case 'd':	/* int */
			in_objs[params.count].integer.value = va_arg(ap, int);
			in_objs[params.count++].type = ACPI_TYPE_INTEGER;
			break;
			/* add more types as needed */
		default:
			pr_err("acpi_evalf() called with invalid format character '%c'\n",
			       c);
			va_end(ap);
			return 0;
		}
	}
	va_end(ap);

	if (res_type != 'v') {
		result.length = sizeof(out_obj);
		result.pointer = &out_obj;
		resultp = &result;
	} else
		resultp = NULL;

	status = acpi_evaluate_object(handle, method, &params, resultp);

	switch (res_type) {
	case 'd':		/* int */
		success = (status == AE_OK &&
			   out_obj.type == ACPI_TYPE_INTEGER);
		if (success && res)
			*res = out_obj.integer.value;
		break;
	case 'v':		/* void */
		success = status == AE_OK;
		break;
		/* add more types as needed */
	default:
		pr_err("acpi_evalf() called with invalid format character '%c'\n",
		       res_type);
		return 0;
	}

	if (!success && !quiet)
		pr_err("acpi_evalf(%s, %s, ...) failed: %s\n",
		       method, fmt0, acpi_format_exception(status));

	return success;
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
	for (int i = 0; i < DASHARO_TEMPERATURE_MAX; ++i)
		if (!acpi_evalf(gfcp_handle, &count, NULL, "dd", 0, i))
			pr_info("Dasharo temperature type %d, count %d\n", i, count);

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
