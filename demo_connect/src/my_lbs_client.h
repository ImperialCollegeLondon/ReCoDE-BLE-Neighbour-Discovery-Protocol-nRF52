#ifndef MY_LBS_CLIENT_H_
#define MY_LBS_CLIENT_H_

#include <zephyr/bluetooth/conn.h>
#include <bluetooth/gatt_dm.h>
#include <zephyr/bluetooth/gatt.h>

#include <zephyr/sys/atomic.h>


/** @brief Data structure of the lbs characteristic.
 */
struct my_lbs_client_button_state {
	 bool button_state; /**< Button state */
};

/* Helper forward structure declaration representing Heart Rate Service Client instance.
 * Needed for callback declaration that are using instance structure as argument.
 */
struct my_lbs_client;

/** @brief indication callback.
 *
 * This function is called every time the client receives a indication
 *
 * @param[in] my_lbs_client  Service Client instance.
 * @param[in] meas  received data.
 * @param[in] err 0 if the notification is valid.
 *                Otherwise, contains a (negative) error code.
 */
typedef void (*my_lbs_client_indicate_cb)(struct my_lbs_client *my_lbs_c,
					const struct my_lbs_client_button_state *meas,
					int err);


/** @brief my lbs button characteristic structure.
 */
struct my_lbs_client_button {
	/** Value handle. */
	uint16_t handle;

	/** Handle of the characteristic CCC descriptor. */
	uint16_t ccc_handle;

	/** GATT subscribe parameters for indicate. */
	struct bt_gatt_subscribe_params indicate_params;

	/** Indicate callback. */
	my_lbs_client_indicate_cb indicate_cb;
};


/** @brief LBS Client instance structure.
 *        This structure contains status information for the client.
 */
struct my_lbs_client {
	/** Connection object. */
	struct bt_conn *conn;

	/** LBS button characteristic. */
	struct my_lbs_client_button button_char;


	/** Internal state. */
	atomic_t state;
};



/** @brief Function for initializing the LBS Client.
 *
 * @param[in, out] my_lbs_c LBS Client instance. This structure must be
 *                       supplied by the application. It is initialized by
 *                       this function and will later be used to identify
 *                       this particular client instance.
 *
 * @retval 0 If the client was initialized successfully.
 *           Otherwise, a (negative) error code is returned.
 */
int my_lbs_client_init(struct my_lbs_client *my_lbs_c);




 /** @brief Subscribe to the LBS measurement characteristic for indications.
 *
 * @param my_lbs_c Pointer to the LBS client structure.
 * @param indicate_cb Callback function to handle received indications.
 *
 * @return 0 on success, negative error code on failure.
 */
int my_lbs_client_button_subscribe(struct  my_lbs_client *my_lbs_c,
					my_lbs_client_indicate_cb indicate_cb);

/**
 * @brief Unsubscribe from the LBS-BUTTON characteristic.
 *
 * @param my_lbs_c Pointer to the LBS client structure.
 *
 * @return 0 on success, negative error code on failure.
 */
int my_lbs_client_button_unsubscribe(struct my_lbs_client *my_lbs_c);


/**
 * @brief Assign GATT handles for the LBS client from the discovered GATT database.
 *
 * @param dm Pointer to the GATT discovery manager.
 * @param my_lbs_c Pointer to the LBS client structure.
 *
 * @return 0 on success, negative error code on failure.
 */
int my_lbs_client_handles_assign(struct bt_gatt_dm *dm, struct my_lbs_client *my_lbs_c);

#endif 