/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 * adv_only_test: 50 packets per cycle, 10 cycles in total
 * packet number is updated in manufacturer data
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

/*  Include the header file of the Bluetooth LE stack 
 *       Provides core Bluetooth initialization and control APIs,
 *       including enabling the Bluetooth stack and managing device states.
*/
#include <zephyr/bluetooth/bluetooth.h>

/*  Include the header file of the BLE GAP（Generic Access Profile）
 *       GAP (Generic Access Profile) defines core Bluetooth behaviors,
 *       including advertising, scanning, connection establishment,
 *       device name/role configuration, and connection parameter control. 
 */
#include <zephyr/bluetooth/gap.h>

//#include <dk_buttons_and_leds.h>

LOG_MODULE_REGISTER(ADV_ONLY_TEST, LOG_LEVEL_INF);

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

#define RUN_STATUS_LED DK_LED1
#define RUN_LED_BLINK_INTERVAL 1000

#define REPEAT_COUNT 50  // broadcast 50 adv packets in each cycle

/*  Define delayed work item*/
static struct k_work_delayable ble_broadcast_work;
static int broadcast_count = 0;  // adv cycle count

/* BLE Advertising Parameters variable */
static struct bt_le_adv_param *adv_param =
	BT_LE_ADV_PARAM(BT_LE_ADV_OPT_NONE, /* No options specified */
			800, /* Min Advertising Interval 500ms (800*0.625ms) */
			800, /* Max Advertising Interval 500ms (800*0.625ms) */
			NULL); /* Set to NULL for undirected advertising */

/* Declare the Company identifier (Company ID) */
#define COMPANY_ID_CODE 0x0059

typedef struct adv_mfg_data {
	uint16_t company_code; /* Company Identifier Code. */
	uint16_t number; /* sequence number */
} adv_mfg_data_type;

/* Define and initialize a variable of type adv_mfg_data_type */
static adv_mfg_data_type adv_mfg_data = { COMPANY_ID_CODE, 0x0000 };

/* Declare the advertising packet */
static const struct bt_data ad[] = {
	/* Set the advertising flags */
	BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR), // no BR/EDR support
	/* Set the advertising packet data: device name and manufacturer data */
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN), 
	BT_DATA(BT_DATA_MANUFACTURER_DATA, (unsigned char *)&adv_mfg_data, sizeof(adv_mfg_data)),   

};


// Function to update advertising data
static void update_advertising_data(void) {
    adv_mfg_data.number += 1;
    // Update advertising data
    int err = bt_le_adv_update_data(ad, ARRAY_SIZE(ad), NULL, 0);
    if (err) {
        LOG_ERR("Failed to update advertising data (err %d)", err);
    }
}

/* BLE start broadcast */
void start_ble_broadcast(void)
{
    int err;
    // adv date: ad, no scan response data
    err = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), NULL, 0);
    if (err) {
        LOG_ERR("Advertising failed to start (err %d)", err);
    } else {
        LOG_INF("Advertising started (%d times)", broadcast_count + 1);
    }
}


/* BLE stop broadcast */
void stop_ble_broadcast(void)
{
    int err;
    err = bt_le_adv_stop();
    if (err) {
        LOG_ERR("Advertising failed to stop (err %d)", err);
    } else {
       LOG_INF("Advertising stopped (%d times)", broadcast_count + 1);
    }
}

/* delayed work handler */
void ble_broadcast_handler(struct k_work *work)
{
    if (broadcast_count < 10) {  // 10 broadcast cycles
       start_ble_broadcast();

        for (int i = 0; i < REPEAT_COUNT; i++) {
        update_advertising_data(); // Update advertising data with new sequence number
		LOG_INF("Current sequence number: %u", adv_mfg_data.number);
        k_sleep(K_MSEC(500)); // Delay for the advertising interval 500ms
        }

        stop_ble_broadcast(); // Stop advertising after broadcasting is done
	    LOG_INF("Current sequence number: %u", adv_mfg_data.number);
        k_work_schedule(&ble_broadcast_work, K_SECONDS(30));  // Delay for the broadcast cycle interval 30s
        broadcast_count++;
    } 
    else {
        LOG_INF("Finished 10 cycles of BLE broadcast");
    }
}

int main(void)
{
	//int blink_status = 0;
	int err;

	LOG_INF("bi-direct BLEnd: adv only test \n");
    /*
    err = dk_leds_init();
	if (err) {
		LOG_ERR("LEDs init failed (err %d)\n", err);
		return -1;
	}
    */
	
	/* STEP 5 - Enable the Bluetooth LE stack */
	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)\n", err);
		return -1;
	}

	LOG_INF("Bluetooth initialized\n");

	
    LOG_INF("Advertising completed");



	/* init delay*/
    k_work_init_delayable(&ble_broadcast_work, ble_broadcast_handler); 

    /* start the first cycle, no delay） */
    k_work_schedule(&ble_broadcast_work, K_NO_WAIT);  

	/*for (;;) {
		dk_set_led(RUN_STATUS_LED, (++blink_status) % 2);
		k_sleep(K_MSEC(RUN_LED_BLINK_INTERVAL));
	}*/
}