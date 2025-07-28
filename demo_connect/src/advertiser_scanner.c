#include "blend.h"
#include "advertiser_scanner.h"
#include "my_lbs.h"


LOG_MODULE_REGISTER(BLEnd_CONN_ADV_SCAN, LOG_LEVEL_DBG);


// Define the k_work structs here.
// This is where the memory is allocated.
struct k_work adv_work;
struct k_work adv_stop;
struct k_work scan_work;
struct k_work scan_stop;

static void adv_work_handler(struct k_work *work);
static void adv_stop_handler(struct k_work *work);


static bool parse_adv_data_cb(struct bt_data *data, void *user_data);
static void scan_filter_match(struct bt_scan_device_info *device_info,
			      struct bt_scan_filter_match *filter_match,
			      bool connectable);
static int scan_start(void);
static void scan_work_handler(struct k_work *item);
static void scan_stop_handler(struct k_work *item);
static const struct bt_le_scan_param my_scan_param = {
    .type = BT_LE_SCAN_TYPE_PASSIVE, // Use passive scanning
    .interval = BT_GAP_SCAN_SLOW_INTERVAL_1, 
    .window = BT_GAP_SCAN_SLOW_INTERVAL_1,    
    .options = BT_LE_SCAN_OPT_NONE,        // No special options, or BT_LE_SCAN_OPT_FILTER_DUPLICATE for common usage
    .timeout = 0,                          // Scan indefinitely (0 means no timeout)
};


static int broadcast_stop = 0;  // adv cycle count
/* BLE Advertising Parameters variable */
static struct bt_le_adv_param *adv_param =
	BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE, /* No options specified */
			500, /* assign an initial value first */
			500, /* assign an initial value first */
			NULL); /* Set to NULL for undirected advertising */

/* Declare the Company identifier (Company ID) */
#define COMPANY_ID_CODE 0x0059
#define BLEND_IDENTIFIER  0xFE
typedef struct adv_mfg_data {
	uint16_t company_code; /* Company Identifier Code. */
	uint16_t blend_id; /* sequence number */
} adv_mfg_data_type;


/* Define and initialize a variable of type adv_mfg_data_type */
static adv_mfg_data_type adv_mfg_data = { COMPANY_ID_CODE, BLEND_IDENTIFIER };

/* Declare the advertising packet */
static const struct bt_data ad[] = {
	/* Set the advertising flags */
	BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR), // no BR/EDR support
	/* Set the advertising packet data: device name and manufacturer data */
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN), 
	BT_DATA(BT_DATA_MANUFACTURER_DATA, (unsigned char *)&adv_mfg_data, sizeof(adv_mfg_data)),   

};


// Define the bt_scan_manufacturer_data struct for the filter
// It holds a pointer to your filter data and its length
static struct bt_scan_manufacturer_data mfg_filter = {
    .data = (uint8_t *)&adv_mfg_data,
    .data_len = sizeof(adv_mfg_data),
};


/**
 * @brief Starts the advertising process
 *
 * @param *work Workqueue thread for starting advertising
 */

void adv_work_handler(struct k_work *work)

{
    int err_start;
    // adv date: ad, no scan response data
    err_start = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), NULL, 0);
    if (err_start) {
        LOG_ERR("Advertising failed to start (err %d)", err_start);
    } else {
        LOG_DBG("Advertising started (%d times)", broadcast_stop + 1);
    }
	dk_set_led(ADVERTISE_LED, 1); // turn on the advertising LED
}

/**
 * @brief stops the advertising process
 *
 * @param *work Workqueue thread for stoping advertising
 */
void adv_stop_handler(struct k_work *work)
{
    int err_stop;
    err_stop = bt_le_adv_stop();
    if (err_stop) {
        LOG_ERR("Advertising failed to stop (err %d)", err_stop);
    } else {
        broadcast_stop++;
       LOG_DBG("Advertising stopped (%d times)", broadcast_stop);
    }
	dk_set_led(ADVERTISE_LED, 0); // turn off the advertising LED
}
void adv_init(int adv_interval)
{
    adv_param->interval_max =adv_interval;
    adv_param->interval_min = adv_interval;
    k_work_init(&adv_work, adv_work_handler);
    k_work_init(&adv_stop, adv_stop_handler);
}

// parses the advertising data to extract the device name.
bool parse_adv_data_cb(struct bt_data *data, void *user_data)
{
     char *name_buffer = (char *)user_data;

    switch (data->type) {
        case BT_DATA_NAME_SHORTENED: 
        case BT_DATA_NAME_COMPLETE:  
            // add the device name to the buffer
            if (data->data_len < MAX_DEVICE_NAME_LEN) {
                memcpy(name_buffer, data->data, data->data_len);
                name_buffer[data->data_len] = '\0'; 
            } else {
            
                memcpy(name_buffer, data->data, MAX_DEVICE_NAME_LEN - 1);
                name_buffer[MAX_DEVICE_NAME_LEN - 1] = '\0';
            }
            LOG_INF("device name: %s", name_buffer);
            return false; // stop parsing further data
        default:
            // if the data type is not name, continue parsing
            return true;
    }
}

// The callback function when a scan filter match occurs.
void scan_filter_match(struct bt_scan_device_info *device_info,
			      struct bt_scan_filter_match *filter_match,
			      bool connectable)
{
	char addr[BT_ADDR_LE_STR_LEN];
	char device_name[MAX_DEVICE_NAME_LEN] = {0};

	bt_addr_le_to_str(device_info->recv_info->addr, addr, sizeof(addr));
	bt_data_parse(device_info->adv_data, parse_adv_data_cb, device_name);

	LOG_INF("Filters matched. Address: %s connectable: %d",
		addr, connectable);
}

// Register the scan callback
BT_SCAN_CB_INIT(scan_cb, scan_filter_match, NULL,
		NULL, NULL);

//starts the scanning process.
int scan_start(void)
{
	int err;
	//make sure the scan is stopped before starting a new one
	err = bt_scan_stop();
    if (err == -EALREADY) {
        //LOG_WRN("Scan already stopped or cancelled.");
    } 
    else if (err) {
        LOG_ERR("Failed to stop scan (err %d)", err);
        return err;
    }

	

	err = bt_scan_start(BT_SCAN_TYPE_SCAN_PASSIVE);
	if (err) {
		LOG_ERR("Scanning failed to start (err %d)", err);
		return err;
	}
	dk_set_led(SCAN_LED, 1); // turn on the scan LED
	LOG_DBG("Scan started");
	return 0;
}
/**
 * @brief Starts the scanning process
 *
 * @param *work Workqueue thread for starting scanning
 */
void scan_work_handler(struct k_work *item)
{
	ARG_UNUSED(item);

	(void)scan_start();
}

/**
 * @brief Starts the scanning process
 *
 * @param *work Workqueue thread for stopping scanning
 */
void scan_stop_handler(struct k_work *item)
{
	ARG_UNUSED(item);
    int err;
    err = bt_scan_stop();
    if (err == -EALREADY) {
        //LOG_WRN("Scan already stopped or cancelled.");
    } 
    else if (err) {
        LOG_ERR("Failed to stop scan (err %d)", err);
        return;
    }
	dk_set_led(SCAN_LED, 0); // turn off the scan LED
    LOG_DBG("scan stopped");
}

// Initializes the scan module.
// Sets up the scan parameters and registers the scan callback.
void scan_init(void)
{
	int err;
	uint8_t filter_mode = 0;
	struct bt_scan_init_param scan_init = {
		.scan_param = &my_scan_param,
		.connect_if_match = true,
	};

	bt_scan_init(&scan_init);
	bt_scan_cb_register(&scan_cb);
	bt_scan_filter_remove_all();

	err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_MANUFACTURER_DATA,&mfg_filter );
	if (err) {
		LOG_ERR("filter cannot be added (err %d", err);
		return;
	}
	filter_mode |= BT_SCAN_MANUFACTURER_DATA_FILTER;

	err = bt_scan_filter_enable(filter_mode, false);
	if (err) {
		LOG_ERR("Filters cannot be turned on (err %d)", err);
		return;
	}

	k_work_init(&scan_work, scan_work_handler);
    k_work_init(&scan_stop, scan_stop_handler);
	LOG_DBG("Scan module initialized");
}

