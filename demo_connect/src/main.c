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

LOG_MODULE_REGISTER(BLEnd_CONN_MAIN, LOG_LEVEL_INF);


#define CONN_LED_PERIPHERAL DK_LED1
#define CONN_LED_CENTRAL DK_LED4
#define USER_BUTTON DK_BTN1_MSK
/* Timer for BLEnd timming */
#define EPOCH_DURATION 10000		// 10 seconds
#define ADV_INTERVAL 800		// 0.625ms 500ms

static bool app_button_state;

/* Define the application callback function for reading the state of the button */
static bool app_button_cb(void)
{
	return app_button_state;
}

/* STEP 10 - Declare a varaible app_callbacks of type my_lbs_cb and initiate its members to the applications call back functions we developed in steps 8.2 and 9.2. */
static struct my_lbs_cb app_callbacks = {
	.button_cb = app_button_cb,
};

static void button_changed(uint32_t button_state, uint32_t has_changed)
{
	if (has_changed & USER_BUTTON) {
		uint32_t user_button_state = button_state & USER_BUTTON;
		app_button_state = user_button_state ? true : false;
	}
}

static void on_connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		LOG_INF("Connection failed (err %u)\n", err);
		return;
	}
	blend_stop();
	LOG_INF("Connected\n");

	dk_set_led_on(CONN_LED_PERIPHERAL);
}

static void on_disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_INF("Disconnected (reason %u)\n", reason);

	dk_set_led_off(CONN_LED_PERIPHERAL);
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
	int blink_status = 0;
	int err;
	
	LOG_INF("bi-direct BLEnd: adv only test \n");
    
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
	
    

 

	

	// for (;;) {
	// 	dk_set_led(RUN_STATUS_LED, (++blink_status) % 2);
	// 	k_sleep(K_MSEC(RUN_LED_BLINK_INTERVAL));
	// }
}