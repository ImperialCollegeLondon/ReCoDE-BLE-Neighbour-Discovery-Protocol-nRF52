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
    my_lbs_c->measurement_char.handle = 0;
    my_lbs_c->measurement_char.ccc_handle = 0;
    my_lbs_c->measurement_char.indicate_cb = NULL;
    my_lbs_c->state = ATOMIC_INIT(0);
}

static int my_lbs_measurement_data_parse(struct my_lbs_client_measurement *lbs_measurement,
				      const uint8_t *data, uint16_t length)
{
	if (!data || length < sizeof(lbs_measurement->button_state)) {
		LOG_ERR("Invalid data received");
		return -EINVAL;
	}

	lbs_measurement->button_state = data[0] ? true : false;

	return 0;
}
static uint8_t my_lbs_measurement_indicate(struct bt_conn *conn,
					 struct bt_gatt_subscribe_params *params,
					 const void *data, uint16_t length)
{
	int err;
	struct my_lbs_client *my_lbs_c;
	struct my_lbs_client_measurement lbs_measurement;

	my_lbs_c = CONTAINER_OF(params, struct my_lbs_client, measurement_char.indicate_params);

	if (!data) {
		atomic_clear_bit(&my_lbs_c->state, INDICATE_ENABLED);
		LOG_DBG("[UNSUBSCRIBE] from Measurement characterictic");

		return BT_GATT_ITER_STOP;
	}

	err = my_lbs_measurement_data_parse(&lbs_measurement, data, length);

	if (my_lbs_c->measurement_char.indicate_cb) {
		my_lbs_c->measurement_char.indicate_cb(my_lbs_c, &lbs_measurement, err);
	}
	return BT_GATT_ITER_CONTINUE;
}

int my_lbs_client_measurement_subscribe(struct  my_lbs_client *my_lbs_c,
					my_lbs_client_indicate_cb indicate_cb)
{
	int err;
	struct bt_gatt_subscribe_params *params = &my_lbs_c->measurement_char.indicate_params;

	if (!my_lbs_c || !indicate_cb) {
		return -EINVAL;
	}

	if (atomic_test_and_set_bit(&my_lbs_c->state, INDICATE_ENABLED)) {
		LOG_INF("Measurement characterisic indication already enabled.");
		// return -EALREADY;
	}
    my_lbs_c->measurement_char.indicate_cb = indicate_cb;
    params->ccc_handle = my_lbs_c->measurement_char.ccc_handle;
    params->value_handle = my_lbs_c->measurement_char.handle;
    params->value= BT_GATT_CCC_INDICATE;
    params->notify = my_lbs_measurement_indicate;

	atomic_set_bit(params->flags, BT_GATT_SUBSCRIBE_FLAG_VOLATILE);

	err = bt_gatt_subscribe(my_lbs_c->conn, params);
	if (err) {
		atomic_clear_bit(&my_lbs_c->state, INDICATE_ENABLED);
		LOG_ERR("Subscribe to characteristic failed");
	} else {
		LOG_DBG("Subscribed to  Measurement characteristic");
	}

	return err;
}

int bt_hrs_client_measurement_unsubscribe(struct my_lbs_client *my_lbs_c)
{
	int err;

	if (!my_lbs_c) {
		return -EINVAL;
	}

	if (!atomic_test_bit(&my_lbs_c->state, INDICATE_ENABLED)) {
		return -EFAULT;
	}

	err = bt_gatt_unsubscribe(my_lbs_c->conn, &my_lbs_c->measurement_char.indicate_params);
	if (err) {
		LOG_ERR("Unsubscribing from Heart Rate Measurement characteristic failed, err: %d",
			err);
	} else {
		atomic_clear_bit(&my_lbs_c->state, INDICATE_ENABLED);
		LOG_DBG("Unsubscribed from Heart Rate Measurement characteristic");
	}

	return err;
}


int my_lbs_client_handles_assign(struct bt_gatt_dm *dm, struct my_lbs_client *my_lbs_c)
{
	const struct bt_gatt_dm_attr *gatt_service_attr =
			bt_gatt_dm_service_get(dm);
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
	LOG_DBG("Getting handles from Heart Rate service.");

	my_lbs_reinit(my_lbs_c);

	/* Heart Rate Measurement characteristic */
	gatt_chrc = bt_gatt_dm_char_by_uuid(dm, BT_UUID_LBS_BUTTON);
	if (!gatt_chrc) {
		LOG_ERR("No Heart Rate Measurement characteristic found.");
		return -EINVAL;
	}

	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc,
					    BT_UUID_LBS_BUTTON);
	if (!gatt_desc) {
		LOG_ERR("No Heart Rate Measurement characteristic value found.");
		return -EINVAL;
	}
	my_lbs_c->measurement_char.handle = gatt_desc->handle;

	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, BT_UUID_GATT_CCC);
	if (!gatt_desc) {
		LOG_ERR("No Heart Rate Measurement CCC descriptor found.");
		return -EINVAL;
	}

	my_lbs_c->measurement_char.ccc_handle = gatt_desc->handle;

	LOG_DBG("Heart Rate Measurement characteristic found");

	
	/* Finally - save connection object */
	my_lbs_c->conn = bt_gatt_dm_conn_get(dm);

	return 0;
}


int my_lbs_client_init(struct my_lbs_client *my_lbs_c)
{
	if (!my_lbs_c) {
		return -EINVAL;
	}

	memset(my_lbs_c, 0, sizeof(*my_lbs_c));

	return 0;
}
