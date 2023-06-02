#include "ble_agent.h"

#include <zephyr/settings/settings.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include "hog.h"
#include "systime.h"

typedef enum
{
    BleAgent_NotReady,
    BleAgent_Idle,
    BleAgent_Advertising,
    BleAgent_Connected,
} BleAgentStatus_t;

static BleAgentStatus_t bleAgentStatus = BleAgent_NotReady;

//***********************ble*********************************
static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID16_ALL,
                  BT_UUID_16_ENCODE(BT_UUID_HIDS_VAL),
                  BT_UUID_16_ENCODE(BT_UUID_BAS_VAL),
                  BT_UUID_16_ENCODE(BT_UUID_DIS_VAL)),
    BT_DATA_BYTES(BT_DATA_GAP_APPEARANCE, BT_UUID_16_ENCODE(CONFIG_BT_DEVICE_APPEARANCE)),
};
//
static void bt_ready(int err);
static void BleAdvStart(void);
static void BleAdvRestart(void);

static volatile bool initialized = false;
static bool dirAdvHighDuty = true;
static bool advNeedRestart = false;

static bt_addr_le_t bondAddr;
static volatile bool deviceBond = false;
void EnumOnBond(const struct bt_bond_info *info,
                void *user_data)
{
    bondAddr = info->addr;
    deviceBond = true;

    printk("bond: %02x:%02x:%02x:%02x:%02x:%02x--%d\r\n",
           info->addr.a.val[5],
           info->addr.a.val[4],
           info->addr.a.val[3],
           info->addr.a.val[2],
           info->addr.a.val[1],
           info->addr.a.val[0],
           info->addr.type);
}

static bool GetBondAddr(void)
{
    deviceBond = false;
    bt_foreach_bond(BT_ID_DEFAULT, EnumOnBond, NULL);
    return deviceBond;
}

static void OnBleConnected(struct bt_conn *conn, uint8_t err)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    if (err)
    {
        printk("Failed to connect to %s (%u)\n", addr, err);

        // directed advertising timeout
        dirAdvHighDuty = false;

        bleAgentStatus = BleAgent_Idle;
    }
    else
    {
        printk("Connected %s\n", addr);
        bleAgentStatus = BleAgent_Connected;
    }
}

static void OnBledisconnected(struct bt_conn *conn, uint8_t reason)
{
    char addr[BT_ADDR_LE_STR_LEN];

    const bt_addr_le_t *peer = bt_conn_get_dst(conn);
    bt_addr_le_to_str(peer, addr, sizeof(addr));

    printk("Disconnected from %s (reason 0x%02x)\n", addr, reason);

    bool fakeDisconn = true;
    for (int i = 0; i < 6; i++)
    {
        if (peer->a.val[i] != 0xff)
            fakeDisconn = false;
    }

    dirAdvHighDuty = true;
    if (!fakeDisconn)
    {
        bleAgentStatus = BleAgent_Advertising;
        advNeedRestart = true;
    }
    else
    {
        bleAgentStatus = BleAgent_NotReady;
    }
}

static void security_changed(struct bt_conn *conn, bt_security_t level,
                             enum bt_security_err err)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    if (!err)
    {
        printk("Security changed: %s level %u\n", addr, level);
    }
    else
    {
        printk("Security failed: %s level %u err %d\n", addr, level,
               err);
    }
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = OnBleConnected,
    .disconnected = OnBledisconnected,
    .security_changed = security_changed,
};

static void auth_cancel(struct bt_conn *conn)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    printk("Pairing cancelled\n");
}

static void pairing_complete(struct bt_conn *conn, bool bonded)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    printk("Pairing completed\n");
}

static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    printk("Pairing failed, reason %d\n", reason);
}

static struct bt_conn_auth_cb conn_auth_callbacks = {
    .passkey_display = NULL,
    .passkey_confirm = NULL,
    .cancel = auth_cancel,
    .pairing_confirm = NULL,
};

static struct bt_conn_auth_info_cb auth_info_callbacks =
    {
        .pairing_complete = pairing_complete,
        .pairing_failed = pairing_failed,
};

static void bt_ready(int err)
{
    if (err)
    {
        printk("Bluetooth init failed (err %d)\n", err);
        return;
    }

    printk("Bluetooth initialized\n");

    hog_init();

    if (IS_ENABLED(CONFIG_SETTINGS))
    {
        settings_load();
    }

    BleAdvStart();
    bleAgentStatus = BleAgent_Advertising;
}

//------------------end of ble------------------------
void BleAgent_Init(void)
{
    if (initialized)
        return;
    //****************ble*********************
    int err;

    err = bt_enable(bt_ready);
    if (err)
    {
        printk("Bluetooth init failed (err %d)\n", err);
        return;
    }

    bt_conn_auth_cb_register(&conn_auth_callbacks);
    bt_conn_auth_info_cb_register(&auth_info_callbacks);
    //------------------------------------
    printk("Bluetooth init done\n");

    initialized = true;
}

void BleAgent_Process(void)
{
    if (!initialized)
        return;

    hog_loop();

    static uint32_t lastTime = 0;
    if (SysTimeSpan(lastTime) >= 1000)
    {
        lastTime = GetSysTime();
    }

    switch (bleAgentStatus)
    {
    case BleAgent_Idle:
        BleAdvStart();
        bleAgentStatus = BleAgent_Advertising;
        break;
    case BleAgent_Advertising:
        if (advNeedRestart)
        {
            advNeedRestart = false;
            BleAdvRestart();
        }
        break;
    default:
        break;
    }
}

static void BleAdvStart(void)
{
    struct bt_le_adv_param advParam = {
        .id = BT_ID_DEFAULT,
        .sid = 0,
        .secondary_max_skip = 0,
        .options = (BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_USE_NAME),
        .interval_min = (BT_GAP_ADV_FAST_INT_MIN_1),
        .interval_max = (BT_GAP_ADV_FAST_INT_MAX_1),
        .peer = NULL,
    };

    if (GetBondAddr())
    { //	directed advertising
        advParam.options = (BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_USE_NAME);
        if (!dirAdvHighDuty)
            advParam.options |= BT_LE_ADV_OPT_DIR_MODE_LOW_DUTY;

        advParam.peer = &bondAddr;
        if (advParam.options & BT_LE_ADV_OPT_DIR_MODE_LOW_DUTY)
            printk("start low duty direct adv\n");
        else
            printk("start high duty direct adv\n");
    }
    else
    {
        printk("start normal adv\n");
    }

    int err = bt_le_adv_start(&advParam, ad, ARRAY_SIZE(ad), NULL, 0);
    if (err)
    {
        printk("Advertising failed to start (err %d)\n", err);
        return;
    }
}

static void BleAdvRestart(void)
{
    int err = bt_le_adv_stop();
    if (err)
    {
        printk("Advertising failed to start (err %d)\n", err);
        return;
    }

    BleAdvStart();
}

void BleAgent_Unbond(void)
{
    if (!initialized)
        return;

    static uint32_t lastTime = 0;
    if (SysTimeSpan(lastTime) < 1000)
        return;

    lastTime = GetSysTime();

    int ret = bt_unpair(BT_ID_DEFAULT, BT_ADDR_LE_ANY);
    printk("Attempting to unpair device %d\n", ret);

    advNeedRestart = true;
}
