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
LOG_MODULE_REGISTER(BLEnd_NONCONN_MAIN, LOG_LEVEL_INF);


#define RUN_STATUS_LED DK_LED1
#define RUN_LED_BLINK_INTERVAL 1000
/* Timer for BLEnd timming */
#define EPOCH_DURATION 10000		// 10 seconds
#define ADV_INTERVAL 800		// 0.625ms 500ms


int main(void)
{
	int blink_status = 0;
	int err;
	
	LOG_INF("Uni-direct BLEnd: non-connectable test \n");
    
    err = dk_leds_init();
	if (err) {
		LOG_ERR("LEDs init failed (err %d)\n", err);
		return -1;
	}
    
	
	/*Enable the Bluetooth LE stack */
	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)\n", err);
		return -1;
	}

	LOG_INF("Bluetooth initialized\n");
    
	
	scan_init();
	adv_init(ADV_INTERVAL);
    
	blend_init(EPOCH_DURATION, ADV_INTERVAL);
	blend_start();
	
    

 

	

	for (;;) {
		dk_set_led(RUN_STATUS_LED, (++blink_status) % 2);
		k_sleep(K_MSEC(RUN_LED_BLINK_INTERVAL));
	}
}