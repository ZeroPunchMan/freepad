/** @file
 *  @brief HoG Service sample
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <zephyr/drivers/gpio.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include "xos_hid_desc.h"
#include "systime.h"

enum
{
	HIDS_REMOTE_WAKE = BIT(0),
	HIDS_NORMALLY_CONNECTABLE = BIT(1),
};

struct hids_info
{
	uint16_t version; /* version number of base USB HID Specification */
	uint8_t code;	  /* country HID Device hardware is localized for. */
	uint8_t flags;
} __packed;

struct hids_report
{
	uint8_t id;	  /* report id */
	uint8_t type; /* report type */
} __packed;

static struct hids_info hidInfo = {
	.version = 0x101,
	.code = 0x00,
	.flags = HIDS_REMOTE_WAKE | HIDS_NORMALLY_CONNECTABLE,
};

enum
{
	HIDS_INPUT = 0x01,
	HIDS_OUTPUT = 0x02,
	HIDS_FEATURE = 0x03,
};

static struct hids_report inputMeta = {
	.id = 0x01,
	.type = HIDS_INPUT,
};

static struct hids_report outputMeta = {
	.id = 0x03,
	.type = HIDS_OUTPUT,
};

static bool simulate_input;
static uint8_t ctrl_point;

static uint8_t outputReportData[8] = {0};
static uint8_t inputReportData[16] = {0};

static ssize_t read_info(struct bt_conn *conn,
						 const struct bt_gatt_attr *attr, void *buf,
						 uint16_t len, uint16_t offset)
{
	printk("read hidInfo\n");
	return bt_gatt_attr_read(conn, attr, buf, len, offset, attr->user_data,
							 sizeof(struct hids_info));
}

static ssize_t read_report_map(struct bt_conn *conn,
							   const struct bt_gatt_attr *attr, void *buf,
							   uint16_t len, uint16_t offset)
{
	printk("read report map\n");
	return bt_gatt_attr_read(conn, attr, buf, len, offset, xosHidDesc,
							 sizeof(xosHidDesc));
}

static ssize_t read_report_meta(struct bt_conn *conn,
								const struct bt_gatt_attr *attr, void *buf,
								uint16_t len, uint16_t offset)
{
	if (attr->user_data == &inputMeta)
		printk("read input report meta\n");
	else if (attr->user_data == &outputMeta)
		printk("read output report meta\n");
	return bt_gatt_attr_read(conn, attr, buf, len, offset, attr->user_data,
							 sizeof(struct hids_report));
}

static void input_ccc_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	simulate_input = (value == BT_GATT_CCC_NOTIFY);
	printk("input ccc: %d\n", simulate_input);
}

static ssize_t read_input_report(struct bt_conn *conn,
								 const struct bt_gatt_attr *attr, void *buf,
								 uint16_t len, uint16_t offset)
{
	printk("read input report\n");
	return bt_gatt_attr_read(conn, attr, buf, len, offset, inputReportData, sizeof(inputReportData));
}

static ssize_t read_output_report(struct bt_conn *conn,
								  const struct bt_gatt_attr *attr, void *buf,
								  uint16_t len, uint16_t offset)
{
	printk("read output report\n");
	return bt_gatt_attr_read(conn, attr, buf, len, offset, NULL, 0);
}

ssize_t write_output_report(struct bt_conn *conn,
							const struct bt_gatt_attr *attr,
							const void *buf, uint16_t len,
							uint16_t offset, uint8_t flags)
{
	printk("write output\n");
	if (offset + len > sizeof(outputReportData))
	{
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}
	memcpy(outputReportData + offset, buf, len);
	return len;
}

static ssize_t write_ctrl_point(struct bt_conn *conn,
								const struct bt_gatt_attr *attr,
								const void *buf, uint16_t len, uint16_t offset,
								uint8_t flags)
{
	printk("write control\n");
	// 0x00--suspend; 0x01--exit suspend;
	uint8_t *value = attr->user_data;

	if (offset + len > sizeof(ctrl_point))
	{
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy(value + offset, buf, len);

	return len;
}

// #define FAKE_UUID

#if defined(FAKE_UUID)
/* HID Service Declaration */
BT_GATT_SERVICE_DEFINE(hog_svc,
					   BT_GATT_PRIMARY_SERVICE(BT_UUID_DECLARE_16(0xf812)),
					   BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_16(0xfa4a), BT_GATT_CHRC_READ,
											  BT_GATT_PERM_READ_ENCRYPT, read_info, NULL, &hidInfo),
					   BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_16(0xfa4b), BT_GATT_CHRC_READ,
											  BT_GATT_PERM_READ_ENCRYPT, read_report_map, NULL, NULL),
					   BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_16(0xfa4d),
											  BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
											  BT_GATT_PERM_READ_ENCRYPT,
											  read_input_report, NULL, NULL),
					   BT_GATT_CCC(input_ccc_changed,
								   BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT), // todo smp
					   BT_GATT_DESCRIPTOR(BT_UUID_DECLARE_16(0xf908), BT_GATT_PERM_READ_ENCRYPT,
										  read_report_meta, NULL, &inputMeta),
					   BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_16(0xfa4c),
											  BT_GATT_CHRC_WRITE_WITHOUT_RESP,
											  BT_GATT_PERM_WRITE_ENCRYPT,
											  NULL, write_ctrl_point, &ctrl_point),

					   BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_16(0xfa4d),
											  BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE_WITHOUT_RESP | BT_GATT_CHRC_WRITE,
											  BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT,
											  read_output_report, write_output_report, NULL),

					   BT_GATT_DESCRIPTOR(BT_UUID_DECLARE_16(0xf908), BT_GATT_PERM_READ_ENCRYPT,
										  read_report_meta, NULL, &outputMeta), );
#else
BT_GATT_SERVICE_DEFINE(hog_svc,
					   BT_GATT_PRIMARY_SERVICE(BT_UUID_DECLARE_16(0x1812)),
					   BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_16(0x2a4a), BT_GATT_CHRC_READ,
											  BT_GATT_PERM_READ_ENCRYPT, read_info, NULL, &hidInfo),
					   BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_16(0x2a4b), BT_GATT_CHRC_READ,
											  BT_GATT_PERM_READ_ENCRYPT, read_report_map, NULL, NULL),
					   BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_16(0x2a4d),
											  BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
											  BT_GATT_PERM_READ_ENCRYPT,
											  read_input_report, NULL, NULL),
					   BT_GATT_CCC(input_ccc_changed,
								   BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT), // todo smp
					   BT_GATT_DESCRIPTOR(BT_UUID_DECLARE_16(0x2908), BT_GATT_PERM_READ_ENCRYPT,
										  read_report_meta, NULL, &inputMeta),
					   BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_16(0x2a4c),
											  BT_GATT_CHRC_WRITE_WITHOUT_RESP,
											  BT_GATT_PERM_WRITE_ENCRYPT,
											  NULL, write_ctrl_point, &ctrl_point),

					   BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_16(0x2a4d),
											  BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE_WITHOUT_RESP | BT_GATT_CHRC_WRITE,
											  BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT,
											  read_output_report, write_output_report, NULL),

					   BT_GATT_DESCRIPTOR(BT_UUID_DECLARE_16(0x2908), BT_GATT_PERM_READ_ENCRYPT,
										  read_report_meta, NULL, &outputMeta), );
#endif

void hog_init(void)
{
	inputReportData[0] = 1;
}

typedef struct
{
	uint16_t leftX, leftY, rightX, rightY, leftTrigger, rightTrigger;
	uint8_t dPad;
	uint8_t button[2];
	uint8_t reserved;
} XosHidReport_t;

void XosHidReportSerialize(uint8_t *buff, const XosHidReport_t *report)
{
	buff[0] = report->leftX & 0xff;
	buff[1] = report->leftX >> 8;
	buff[2] = report->leftY & 0xff;
	buff[3] = report->leftY >> 8;

	buff[4] = report->rightX & 0xff;
	buff[5] = report->rightX >> 8;
	buff[6] = report->rightY & 0xff;
	buff[7] = report->rightY >> 8;

	buff[8] = report->leftTrigger & 0xff;
	buff[9] = report->leftTrigger >> 8;
	buff[10] = report->rightTrigger & 0xff;
	buff[11] = report->rightTrigger >> 8;

	buff[12] = report->dPad;
	buff[13] = report->button[0];
	buff[14] = report->button[1];
	buff[15] = report->reserved;
}

static XosHidReport_t xosReport = 
{
	.leftX = UINT16_MAX / 2, //max 65535
	.leftY = UINT16_MAX / 2,

	.rightX = UINT16_MAX / 2,
	.rightY = UINT16_MAX / 2,

	.leftTrigger = 0, //max 0x3ff
	.rightTrigger = 0,

	.dPad = 0, //1~8
	.button[0] = 0,
	.button[1] = 0,
	.reserved = 0,
};

void hog_loop(void)
{
	
	static uint32_t lastTime = 0;
	if (SysTimeSpan(lastTime) >= 1000)
	{
		lastTime = GetSysTime();
		// if (simulate_input)
		{
			XosHidReportSerialize(inputReportData, &xosReport);
			if (xosReport.button[0])
				xosReport.button[0] = 0;
			else
				xosReport.button[0] = 0x08;

			printk("notify %u\n", GetSysTime() / 1000);
			bt_gatt_notify(NULL, &hog_svc.attrs[5],
						   inputReportData, sizeof(inputReportData));
		}
		// else
		// {
		// 	printk("no notify %u\n", GetSysTime() / 1000);
		// }
	}
}
