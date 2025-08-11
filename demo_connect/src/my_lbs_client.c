#include "my_lbs_client.h"
#include "my_lbs.h"
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(my_lbs_client, LOG_LEVEL_INF);

#define INDICATE_ENABLED BIT(0)

static void my_lbs_reinit(struct my_lbs_client *my_lbs_c)
{
	my_lbs_c->conn = NULL;
    my_lbs_c->button_char.handle = 0;
    my_lbs_c->button_char.ccc_handle = 0;
    my_lbs_c->button_char.indicate_cb = NULL;
    my_lbs_c->state = ATOMIC_INIT(0);
}

/**
 * @brief Parse the data received from the LBS measurement characteristic.
 *
 * @param lbs_measurement Pointer to the structure where parsed data will be stored.
 * @param data Pointer to the data received from the characteristic.
 * @param length Length of the received data.
 *
 * @return 0 on success, negative error code on failure.
 */
static int my_lbs_button_data_parse(struct my_lbs_client_button_state *lbs_state,
				      const uint8_t *data, uint16_t length)
{
	if (!data || length < sizeof(lbs_state->button_state)) {
		LOG_ERR("Invalid data received");
		return -EINVAL;
	}

	lbs_state->button_state = data[0] ? true : false;

	return 0;
}

/**
 * @brief Callback function for handling indications from the LBS button characteristic.
 *
 * @param conn Pointer to the Bluetooth connection.
 * @param params Pointer to the subscription parameters.
 * @param data Pointer to the data received from the characteristic.
 * @param length Length of the received data.
 *
 * @return BT_GATT_ITER_CONTINUE to continue receiving indications, or BT_GATT_ITER_STOP to stop.
 */
static uint8_t my_lbs_button_indicate(struct bt_conn *conn,
					 struct bt_gatt_subscribe_params *params,
					 const void *data, uint16_t length)
{
	int err;
	struct my_lbs_client *my_lbs_c;
	struct my_lbs_client_button_state lbs_button_state;

	my_lbs_c = CONTAINER_OF(params, struct my_lbs_client, button_char.indicate_params);

	if (!data) {
		atomic_clear_bit(&my_lbs_c->state, INDICATE_ENABLED);
		LOG_DBG("[UNSUBSCRIBE] from LBS button characterictic");

		return BT_GATT_ITER_STOP;
	}

	err = my_lbs_button_data_parse(&lbs_button_state, data, length);


	if (my_lbs_c->button_char.indicate_cb) {
		my_lbs_c->button_char.indicate_cb(my_lbs_c, &lbs_button_state, err);
	}
	return BT_GATT_ITER_CONTINUE;
}

/**
 * @brief Subscribe to the LBS-BUTTON characteristic for indications.
 * 
 * * This function writes CCC descriptor of the LBS-BUTTON characteristic
 * to enable indication.
 * @param my_lbs_c Pointer to the LBS client structure.
 * @param indicate_cb Callback function to handle received indications.
 *
 * @return 0 on success, negative error code on failure.
 */
int my_lbs_client_button_subscribe(struct  my_lbs_client *my_lbs_c,
					my_lbs_client_indicate_cb indicate_cb)
{
	int err;
	struct bt_gatt_subscribe_params *params = &my_lbs_c->button_char.indicate_params;

	if (!my_lbs_c || !indicate_cb) {
		return -EINVAL;
	}

	if (atomic_test_and_set_bit(&my_lbs_c->state, INDICATE_ENABLED)) {
		LOG_INF("LBS-BUTTON characterisic indication already enabled.");
		// return -EALREADY;
	}
    my_lbs_c->button_char.indicate_cb = indicate_cb;
    params->ccc_handle = my_lbs_c->button_char.ccc_handle;
    params->value_handle = my_lbs_c->button_char.handle;
    params->value= BT_GATT_CCC_INDICATE;
    params->notify = my_lbs_button_indicate;

	atomic_set_bit(params->flags, BT_GATT_SUBSCRIBE_FLAG_VOLATILE);

	err = bt_gatt_subscribe(my_lbs_c->conn, params);
	if (err) {
		atomic_clear_bit(&my_lbs_c->state, INDICATE_ENABLED);
		LOG_ERR("Subscribe to characteristic failed");
	} else {
		LOG_DBG("Subscribed to  LBS-BUTTON characteristic");
	}

	return err;
}

/**
 * @brief Unsubscribe from the LBS-BUTTON characteristic.
 *
 * @param my_lbs_c Pointer to the LBS client structure.
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_hrs_client_button_unsubscribe(struct my_lbs_client *my_lbs_c)
{
	int err;

	if (!my_lbs_c) {
		return -EINVAL;
	}

	if (!atomic_test_bit(&my_lbs_c->state, INDICATE_ENABLED)) {
		return -EFAULT;
	}

	err = bt_gatt_unsubscribe(my_lbs_c->conn, &my_lbs_c->button_char.indicate_params);
	if (err) {
		LOG_ERR("Unsubscribing from LBS-BUTTON characteristic failed, err: %d",
			err);
	} else {
		atomic_clear_bit(&my_lbs_c->state, INDICATE_ENABLED);
		LOG_DBG("Unsubscribed from LBS-BUTTON characteristic");
	}

	return err;
}

/**
 * @brief Assign GATT handles for the LBS client from the discovered GATT database.
 *
 * @param dm Pointer to the GATT discovery manager.
 * @param my_lbs_c Pointer to the LBS client structure.
 *
 * @return 0 on success, negative error code on failure.
 */
int my_lbs_client_handles_assign(struct bt_gatt_dm *dm, struct my_lbs_client *my_lbs_c)
{
	// Get the service and characteristic attributes from the GATT discovery manager
	const struct bt_gatt_dm_attr *gatt_service_attr =
			bt_gatt_dm_service_get(dm);
	// Get the service value from the attribute
	const struct bt_gatt_service_val *gatt_service =
			bt_gatt_dm_attr_service_val(gatt_service_attr);
	const struct bt_gatt_dm_attr *gatt_chrc;
	const struct bt_gatt_dm_attr *gatt_desc;

	if (!dm || !my_lbs_c) {
		return -EINVAL;
	}

	if (bt_uuid_cmp(gatt_service->uuid, BT_UUID_LBS)) {
		return -ENOTSUP;
	}
	LOG_DBG("Getting handles from my_lbs service.");

	my_lbs_reinit(my_lbs_c);

	// find LBS-BUTTON characteristic 
	gatt_chrc = bt_gatt_dm_char_by_uuid(dm, BT_UUID_LBS_BUTTON);
	if (!gatt_chrc) {
		LOG_ERR("No LBS characteristic found.");
		return -EINVAL;
	}

	
	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc,
					    BT_UUID_LBS_BUTTON);
	if (!gatt_desc) {
		LOG_ERR("No LBS-BUTTON characteristic value found.");
		return -EINVAL;
	}
	
	my_lbs_c->button_char.handle = gatt_desc->handle;

	
	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, BT_UUID_GATT_CCC);
	if (!gatt_desc) {
		LOG_ERR("No LBS-BUTTON CCC descriptor found.");
		return -EINVAL;
	}

	my_lbs_c->button_char.ccc_handle = gatt_desc->handle;

	LOG_DBG("LBS-BUTTON characteristic found");

	
	/* Finally - save connection object */
	my_lbs_c->conn = bt_gatt_dm_conn_get(dm);

	return 0;
}

/*
 * @brief Initialize the LBS client.	
*/
int my_lbs_client_init(struct my_lbs_client *my_lbs_c)
{
	if (!my_lbs_c) {
		return -EINVAL;
	}

	memset(my_lbs_c, 0, sizeof(*my_lbs_c));

	return 0;
}
