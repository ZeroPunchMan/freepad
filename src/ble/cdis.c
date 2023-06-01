/** @file
 *  @brief GATT Device Information Service
 */

/*
 * Copyright (c) 2019 Demant
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>

#include <zephyr/settings/settings.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/sys/printk.h>


struct dis_pnp {
	uint8_t pnp_vid_src;
	uint16_t pnp_vid;
	uint16_t pnp_pid;
	uint16_t pnp_ver;
} __packed;

static struct dis_pnp dis_pnp_id = {
	.pnp_vid_src = 2,
	.pnp_vid = 1118,
	.pnp_pid = 2848,
	.pnp_ver = 1303,
};


#define BT_DIS_MANUF_REF		"Microsoft"
#define BT_DIS_SERIAL_NUMBER_STR_REF	"02600105842824"
#define BT_DIS_FW_REV_STR_REF		"5.17.3202.0"

static ssize_t read_str(struct bt_conn *conn,
			  const struct bt_gatt_attr *attr, void *buf,
			  uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, attr->user_data,
				 strlen(attr->user_data));
}

static ssize_t read_pnp_id(struct bt_conn *conn,
			   const struct bt_gatt_attr *attr, void *buf,
			   uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &dis_pnp_id,
				 sizeof(dis_pnp_id));
}

/* Device Information Service Declaration */
BT_GATT_SERVICE_DEFINE(dis_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_DIS),
	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_MANUFACTURER_NAME,
			       BT_GATT_CHRC_READ, BT_GATT_PERM_READ_ENCRYPT,
			       read_str, NULL, BT_DIS_MANUF_REF),
	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_PNP_ID,
			       BT_GATT_CHRC_READ, BT_GATT_PERM_READ_ENCRYPT,
			       read_pnp_id, NULL, &dis_pnp_id),
	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_FIRMWARE_REVISION,
			       BT_GATT_CHRC_READ, BT_GATT_PERM_READ_ENCRYPT,
			       read_str, NULL, BT_DIS_FW_REV_STR_REF),
	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_SERIAL_NUMBER,
			       BT_GATT_CHRC_READ, BT_GATT_PERM_READ_ENCRYPT,
			       read_str, NULL,
			       BT_DIS_SERIAL_NUMBER_STR_REF),
);
