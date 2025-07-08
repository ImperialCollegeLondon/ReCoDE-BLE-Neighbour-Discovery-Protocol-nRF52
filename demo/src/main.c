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

//#include <dk_buttons_and_leds.h>

LOG_MODULE_REGISTER(BLEnd_NONCONN_TEST, LOG_LEVEL_INF);

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

#define RUN_STATUS_LED DK_LED1
#define RUN_LED_BLINK_INTERVAL 1000




/* Timer for BLEnd timming */
#define EPOCH_DURATION 3000
#define ADV_DURATION 2000
static void adv_timeout_timer_handler(struct k_timer *timer_id);
static void adv_toggle_timer_handler(struct k_timer *timer_id);
K_TIMER_DEFINE(adv_toggle_timer, adv_toggle_timer_handler, NULL);
K_TIMER_DEFINE(adv_timeout_timer, adv_timeout_timer_handler, NULL);

static int adv_toggle_count = 0;  // toggle count
static int broadcast_stop = 0;  // adv cycle count

// workqueue thread for starting and stopping advertising and scanning
static struct k_work adv_work;
static struct k_work adv_stop;
static struct k_work scan_work;
static struct k_work scan_stop;

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


/* functions used for advertising:
 * adv_work_handler: starts the advertising process
 * adv_stop_handler: stops the advertising process
*/

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


/*	functions for scanning and filtering:
 *  - scan_filter_match: called when a filter match occurs during scanning
 *  - scan_start: starts the scanning process with the specified filters
 *  - scan_work_handler: handles the work item for starting the scan
 *  - scan_stop_handler: handles the work item for stopping the scan
 *  - scan_init: initializes the scanning module and registers callbacks
*/

static void scan_filter_match(struct bt_scan_device_info *device_info,
			      struct bt_scan_filter_match *filter_match,
			      bool connectable)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(device_info->recv_info->addr, addr, sizeof(addr));

	LOG_INF("Filters matched. Address: %s connectable: %d",
		addr, connectable);
}

BT_SCAN_CB_INIT(scan_cb, scan_filter_match, NULL,
		NULL, NULL);

static int scan_start(void)
{
	int err;
	uint8_t filter_mode = 0;
	//make sure the scan is stopped before starting a new one
	err = bt_scan_stop();
    if (err == -EALREADY) {
        LOG_WRN("Scan already stopped or cancelled.");
    } 
    else if (err) {
        LOG_ERR("Failed to stop scan (err %d)", err);
        return err;
    }

	bt_scan_filter_remove_all();

	err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_NAME,"mydevice" );
	if (err) {
		LOG_ERR("NAME filter cannot be added (err %d", err);
		return err;
	}
	filter_mode |= BT_SCAN_NAME_FILTER;

	err = bt_scan_filter_enable(filter_mode, false);
	if (err) {
		LOG_ERR("Filters cannot be turned on (err %d)", err);
		return err;
	}

	err = bt_scan_start(BT_SCAN_TYPE_SCAN_PASSIVE);
	if (err) {
		LOG_ERR("Scanning failed to start (err %d)", err);
		return err;
	}

	LOG_INF("Scan started");
	return 0;
}

static void scan_work_handler(struct k_work *item)
{
	ARG_UNUSED(item);

	(void)scan_start();
}
static void scan_stop_handler(struct k_work *item)
{
	ARG_UNUSED(item);
    int err;
    err = bt_scan_stop();
    if (err == -EALREADY) {
        LOG_WRN("Scan already stopped or cancelled.");
    } 
    else if (err) {
        LOG_ERR("Failed to stop scan (err %d)", err);
        return err;
    }
    LOG_INF("scan stopped");
}

static void scan_init(void)
{
	struct bt_scan_init_param scan_init = {
		.connect_if_match = false,
	};

	bt_scan_init(&scan_init);
	bt_scan_cb_register(&scan_cb);

	k_work_init(&scan_work, scan_work_handler);
    k_work_init(&scan_stop, scan_stop_handler);
	LOG_INF("Scan module initialized");
}


/* timer and workqueue handlers
 * adv_timeout_timer_handler: stops advertising after a specified duration
 * adv_toggle_timer_handler: toggles between starting advertising and scanning
*/
void adv_timeout_timer_handler(struct k_timer *timer_id)
{
    // Stop advertising after broadcasting is done
   k_work_submit(&adv_stop);
   
}

void adv_toggle_timer_handler(struct k_timer *timer_id)
{
    LOG_INF(" enter adv_toggle_timer_handler");
        if(adv_toggle_count %2 ==0){
            //start_ble_broadcast   
            adv_toggle_count++;
            k_work_submit(&scan_stop);
            k_work_submit(&adv_work);
            k_timer_start(&adv_timeout_timer, K_MSEC(ADV_DURATION), K_NO_WAIT);
      
        }
        else
        {
            adv_toggle_count++;
            k_work_submit(&scan_work);
            LOG_INF("scan for one cycle");
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
    scan_init();

    k_work_init(&adv_work, adv_work_handler);
    k_work_init(&adv_stop, adv_stop_handler);

	
    

 
    k_timer_start(&adv_toggle_timer, K_NO_WAIT, K_MSEC(EPOCH_DURATION));
    LOG_INF("BLEnd start");
	

	/*for (;;) {
		dk_set_led(RUN_STATUS_LED, (++blink_status) % 2);
		k_sleep(K_MSEC(RUN_LED_BLINK_INTERVAL));
	}*/
}