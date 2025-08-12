/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 * adv_only_test: 50 packets per cycle, 10 cycles in total
 * packet number is updated in manufacturer data
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

/*  Include the header file of the Bluetooth LE stack */
#include <zephyr/bluetooth/bluetooth.h>

/*  Include the header file of the BLE GAP（Generic Access Profile）*/
#include <zephyr/bluetooth/gap.h>
#include <bluetooth/scan.h>

#include <dk_buttons_and_leds.h>
#include "blend.h"
#include "advertiser_scanner.h"
#include "my_lbs.h"
#include "my_lbs_client.h"
#include <bluetooth/gatt_dm.h>
LOG_MODULE_REGISTER(BLEnd_CONN_MAIN, LOG_LEVEL_INF);



#define CONN_LED_PERIPHERAL DK_LED1
#define CONN_LED_CENTRAL DK_LED4
#define USER_BUTTON DK_BTN1_MSK
/* Timer for BLEnd timming */
#define EPOCH_DURATION 10000		// 10 seconds
#define ADV_INTERVAL 800		// 0.625ms 500ms

static bool app_button_state;
static struct bt_conn *default_conn = NULL;
static struct bt_my_lbs bt_my_lbs;
static struct my_lbs_client bt_my_client;

/* Define the application callback function for reading the state of the button */
static bool app_button_cb(void)
{
	return app_button_state;
}

/* Declare a varaible app_callbacks of type my_lbs_cb and initiate its members to the applications call back functions  */
static struct my_lbs_cb app_callbacks = {
	.button_cb = app_button_cb,
};

static void button_changed(uint32_t button_state, uint32_t has_changed)
{
	if (has_changed & USER_BUTTON) {
		uint32_t user_button_state = button_state & USER_BUTTON;
		/*  Send indication on a button press */
		my_lbs_send_button_state_indicate(user_button_state);
		app_button_state = user_button_state ? true : false;
	}
}


static void my_lbs_indicate_cb(struct my_lbs_client *my_lbs_c,
				      const struct my_lbs_client_button_state *meas,
				      int err)
{
	if (err) {
		LOG_ERR("Error during receiving LBS button indication, err: %d\n",
			err);
		return;
	}
	LOG_INF("Received button state indication: %s",
		meas->button_state ? "Pressed" : "Released");
	if (meas->button_state) 
		dk_set_led_on(CONN_LED_PERIPHERAL);
	else 
	dk_set_led_off(CONN_LED_PERIPHERAL);
}

static void discovery_complete(struct bt_gatt_dm *dm,
			       void *context)
{
	
	int err;
	LOG_INF("Service found");

	my_lbs_client_handles_assign(dm, &bt_my_client);

	err = my_lbs_client_button_subscribe(&bt_my_client, my_lbs_indicate_cb);
	if (err) {
		printk("Could not subscribe to LBS button characteristic (err %d)\n",
		       err);
	}

	err = bt_gatt_dm_data_release(dm);
	if (err) {
		LOG_ERR("Could not release the discovery data (err %d)\n", err);
	}
}

static void discovery_service_not_found(struct bt_conn *conn,
					void *context)
{
	LOG_INF("Service not found\n");
}

static void discovery_error(struct bt_conn *conn,
			    int err,
			    void *context)
{
	LOG_ERR("Error while discovering GATT database: (%d)\n", err);
}

struct bt_gatt_dm_cb discovery_cb = {
	.completed         = discovery_complete,
	.service_not_found = discovery_service_not_found,
	.error_found       = discovery_error,
};

static void on_connected(struct bt_conn *conn, uint8_t err)
{
	int err_dm;
	struct bt_conn_info info = {0};
	if (err) {
		LOG_ERR("Connection failed (err %u)\n", err);
		bt_conn_unref(default_conn);	// Always unref if connection fails

		return;
	}
	   // This check is important for a single-connection system:
    if (default_conn) {
        LOG_WRN("Already tracking a connection, disconnecting new one: %p", (void *)conn);
        bt_conn_disconnect(conn, BT_HCI_ERR_CONN_LIMIT_EXCEEDED);
        bt_conn_unref(conn); // Unref the connection we are not taking
        return;
    }
	
	blend_stop();

	default_conn = bt_conn_ref(conn);
	err_dm = bt_conn_get_info(default_conn, &info);
	if (err_dm) {
		LOG_ERR("Failed to get connection info %d\n", err);
		return;
	}
	if (info.role == BT_CONN_ROLE_PERIPHERAL) {
			LOG_INF("Connected: BT_CONN_ROLE_PERIPHERAL\n");
		dk_set_led_on(CONN_LED_PERIPHERAL);
	}
	if (info.role == BT_CONN_ROLE_CENTRAL) {
		LOG_INF("Connected: BT_CONN_ROLE_CENTRAL\n");
		dk_set_led_on(CONN_LED_CENTRAL);
		err_dm = bt_gatt_dm_start(default_conn,
				       BT_UUID_LBS,
				       &discovery_cb,
						NULL);
		if (err_dm) {
			LOG_ERR("Discover failed (err %d)\n", err_dm);
		}
	}
	

	
}

static void on_disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_INF("Disconnected (reason %u)\n", reason);

	dk_set_led_off( CONN_LED_CENTRAL);
	dk_set_led_off(CONN_LED_PERIPHERAL);
	bt_conn_unref(default_conn);
	default_conn = NULL;
	blend_start();
}

struct bt_conn_cb connection_callbacks = {
	.connected = on_connected,
	.disconnected = on_disconnected,
};

static int init_button(void)
{
	int err;

	err = dk_buttons_init(button_changed);
	if (err) {
		LOG_INF("Cannot init buttons (err: %d)\n", err);
	}

	return err;
}

int main(void)
{

	int err;
	
	LOG_INF("Uni-direct BLEnd: Connection + Service \n");
    
    err = dk_leds_init();
	if (err) {
		LOG_ERR("LEDs init failed (err %d)\n", err);
		return -1;
	}
	
	err = init_button();
	if (err) {
		printk("Button init failed (err %d)\n", err);
		return -1;
	}
    
	
	/*Enable the Bluetooth LE stack */
	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)\n", err);
		return -1;
	}
	// Register the connection callbacks （advertiser）
	bt_conn_cb_register(&connection_callbacks);

	LOG_INF("Bluetooth initialized\n");
	/* Pass your application callback functions stored in app_callbacks to the MY LBS service */
	err = my_lbs_init(&app_callbacks);
	if (err) {
		printk("Failed to init LBS (err:%d)\n", err);
		return -1;
	}
    
	
	scan_init();
	adv_init(ADV_INTERVAL);
    
	blend_init(EPOCH_DURATION, ADV_INTERVAL);
	blend_start();
	
}