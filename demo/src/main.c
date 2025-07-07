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

#define REPEAT_COUNT 20  // broadcast 50 adv packets in each cycle


/* Timer for scheduling advertising on/off */
static void adv_timeout_timer_handler(struct k_timer *timer_id);
static void adv_toggle_timer_handler(struct k_timer *timer_id);
K_TIMER_DEFINE(adv_toggle_timer, adv_toggle_timer_handler, NULL);
K_TIMER_DEFINE(adv_timeout_timer, adv_timeout_timer_handler, NULL);

static int adv_toggle_count = 0;  // adv cycle count
static int broadcast_stop = 0;  // adv cycle count

// workqueue thread for starting and stopping advertising
static struct k_work adv_work;
static struct k_work adv_stop;

/* BLE Advertising Parameters variable */
static struct bt_le_adv_param *adv_param =
	BT_LE_ADV_PARAM(BT_LE_ADV_OPT_NONE, /* No options specified */
			80, /* Min Advertising Interval *0.625ms) */
			80, /* Max Advertising Interval *0.625ms) */
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




/* BLE start broadcast */
static void adv_work_handler(struct k_work *work)

{
    int err_start;
    // adv date: ad, no scan response data
    err_start = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), NULL, 0);
    if (err_start) {
        LOG_ERR("Advertising failed to start (err %d)", err_start);
    } else {
        LOG_INF("Advertising started (%d times)", broadcast_stop + 1);
    }
}


/* BLE stop broadcast */
static void adv_stop_handler(struct k_work *work)
{
    int err_stop;
    err_stop = bt_le_adv_stop();
    if (err_stop) {
        LOG_ERR("Advertising failed to stop (err %d)", err_stop);
    } else {
        broadcast_stop++;
       LOG_INF("Advertising stopped (%d times)", broadcast_stop);
    }
}


void adv_timeout_timer_handler(struct k_timer *timer_id)
{
    // Stop advertising after broadcasting is done
   k_work_submit(&adv_stop);
   
}

void adv_toggle_timer_handler(struct k_timer *timer_id)
{
    LOG_INF(" enter adv_toggle_timer_handler");
    if (adv_toggle_count /2 < 100){
        if(adv_toggle_count %2 ==0){
            //start_ble_broadcast   
             adv_toggle_count++;
            k_work_submit(&adv_work);
            k_timer_start(&adv_timeout_timer, K_MSEC(2000), K_NO_WAIT);
      
        }
        else
        {
            adv_toggle_count++;
            LOG_INF("sleep for one cycle");
        }
       

    }
    else{
        k_timer_stop(timer_id);
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
    k_work_init(&adv_work, adv_work_handler);
    k_work_init(&adv_stop, adv_stop_handler);
	
    

 
    k_timer_start(&adv_toggle_timer, K_NO_WAIT, K_MSEC(3000));
    LOG_INF("timer start");
	

	/*for (;;) {
		dk_set_led(RUN_STATUS_LED, (++blink_status) % 2);
		k_sleep(K_MSEC(RUN_LED_BLINK_INTERVAL));
	}*/
}