#include "my_lbs.h"

LOG_MODULE_DECLARE(MY_LBS);

static bool button_state;
static struct my_lbs_cb lbs_cb;


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
		       BT_GATT_CHARACTERISTIC(BT_UUID_LBS_BUTTON, BT_GATT_CHRC_READ,
					      BT_GATT_PERM_READ, read_button, NULL, &button_state),

);
/* A function to register application callbacks for the LED and Button characteristics  */
int my_lbs_init(struct my_lbs_cb *callbacks)
{
	if (callbacks) {
		lbs_cb.button_cb = callbacks->button_cb;
	}

	return 0;
}
