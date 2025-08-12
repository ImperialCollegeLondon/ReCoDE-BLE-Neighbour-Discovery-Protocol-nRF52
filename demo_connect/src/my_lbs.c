#include "my_lbs.h"

LOG_MODULE_DECLARE(MY_LBS);

static bool button_state;
static bool indicate_enabled;
static struct my_lbs_cb lbs_cb;

/* Define an indication parameter */
static struct bt_gatt_indicate_params ind_params;

/* Implement the configuration change callback function */
static void mylbsbc_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	indicate_enabled = (value == BT_GATT_CCC_INDICATE);
}

// This function is called when a remote device has acknowledged the indication at its host layer
static void indicate_cb(struct bt_conn *conn, struct bt_gatt_indicate_params *params, uint8_t err)
{
	LOG_DBG("Indication %s\n", err != 0U ? "fail" : "success");
}

/*  Implement the read callback function of the Button characteristic */
static ssize_t read_button(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			   uint16_t len, uint16_t offset)
{
	// get a pointer to button_state which is passed in the BT_GATT_CHARACTERISTIC() and stored in attr->user_data
	const char *value = attr->user_data;

	LOG_DBG("Attribute read, handle: %u, conn: %p", attr->handle, (void *)conn);

	if (lbs_cb.button_cb) {
		// Call the application callback function to update the get the current value of the button
		button_state = lbs_cb.button_cb();
		return bt_gatt_attr_read(conn, attr, buf, len, offset, value, sizeof(*value));
	}

	return 0;
}

/*  Service Declaration */
/*  Create and add the MY LBS service to the Bluetooth LE stack */
BT_GATT_SERVICE_DEFINE(my_lbs_svc, BT_GATT_PRIMARY_SERVICE(BT_UUID_LBS),
		       /*  Create and add the Button characteristic */
		       	BT_GATT_CHARACTERISTIC(BT_UUID_LBS_BUTTON,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_INDICATE,
			       BT_GATT_PERM_READ, read_button, NULL,
			       &button_state),
				BT_GATT_CCC(mylbsbc_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),

);
/* A function to register application callbacks for the LED and Button characteristics  */
int my_lbs_init(struct my_lbs_cb *callbacks)
{
	if (callbacks) {
		lbs_cb.button_cb = callbacks->button_cb;
	}

	return 0;
}

/* Define the function to send indications */
int my_lbs_send_button_state_indicate(bool button_state)
{
	if (!indicate_enabled) {
		return -EACCES;
	}
	ind_params.attr = &my_lbs_svc.attrs[2];
	ind_params.func = indicate_cb; // A remote device has ACKed at its host layer (ATT ACK)
	ind_params.destroy = NULL;
	ind_params.data = &button_state;
	ind_params.len = sizeof(button_state);
	return bt_gatt_indicate(NULL, &ind_params);
}