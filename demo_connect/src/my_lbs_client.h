#ifndef MY_LBS_CLIENT_H_
#define MY_LBS_CLIENT_H_

#include <zephyr/bluetooth/conn.h>
#include <bluetooth/gatt_dm.h>
#include <zephyr/bluetooth/gatt.h>

#include <zephyr/sys/atomic.h>


/**@brief Data structure of the lbs characteristic.
 */
struct my_lbs_client_measurement {
	 bool button_state; /**< Button state */
};

/* Helper forward structure declaration representing Heart Rate Service Client instance.
 * Needed for callback declaration that are using instance structure as argument.
 */
struct my_lbs_client;

/**@brief indication callback.
 *
 * This function is called every time the client receives a indication
 * with Measurement data.
 *
 * @param[in] my_lbs_client  Service Client instance.
 * @param[in] meas  Measurement received data.
 * @param[in] err 0 if the notification is valid.
 *                Otherwise, contains a (negative) error code.
 */
typedef void (*my_lbs_client_indicate_cb)(struct my_lbs_client *my_lbs_c,
					const struct my_lbs_client_measurement *meas,
					int err);


/**@brief my lbs Measurement characteristic structure.
 */
struct my_lbs_client_meas {
	/** Value handle. */
	uint16_t handle;

	/** Handle of the characteristic CCC descriptor. */
	uint16_t ccc_handle;

	/** GATT subscribe parameters for indicate. */
	struct bt_gatt_subscribe_params indicate_params;

	/** Indicate callback. */
	my_lbs_client_indicate_cb indicate_cb;
};


/**@brief Heart Rate Service Client instance structure.
 *        This structure contains status information for the client.
 */
struct my_lbs_client {
	/** Connection object. */
	struct bt_conn *conn;

	/** Heart Rate Measurement characteristic. */
	struct my_lbs_client_meas measurement_char;


	/** Internal state. */
	atomic_t state;
};



/**@brief Function for initializing the Heart Rate Service Client.
 *
 * @param[in, out] hrs_c Heart Rate Service Client instance. This structure must be
 *                       supplied by the application. It is initialized by
 *                       this function and will later be used to identify
 *                       this particular client instance.
 *
 * @retval 0 If the client was initialized successfully.
 *           Otherwise, a (negative) error code is returned.
 */
int my_lbs_client_init(struct my_lbs_client *my_lbs_c);



/**@brief Subscribe to Heart Rate Measurement notification.
 *
 * This function writes CCC descriptor of the Heart Rate Measurement characteristic
 * to enable notification.
 *
 * @param[in] hrs_c Heart Rate Service Client instance.
 * @param[in] notify_cb   Notification callback.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int my_lbs_client_measurement_subscribe(struct  my_lbs_client *my_lbs_c,
					my_lbs_client_indicate_cb indicate_cb);


/**@brief Remove subscription to the Heart Rate Measurement notification.
 *
 * This function writes CCC descriptor of the Heart Rate Measurement characteristic
 * to disable notification.
 *
 * @param[in] hrs_c Heart Rate Service Client instance.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int my_lbs_client_measurement_unsubscribe(struct my_lbs_client *my_lbs_c);


/**@brief Function for assigning handles to Heart Rate Service Client instance.
 *
 * @details Call this function when a link has been established with a peer to
 *          associate the link to this instance of the module. This makes it
 *          possible to handle several links and associate each link to a particular
 *          instance of this module.
 *
 * @param[in]     dm     Discovery object.
 * @param[in,out] hrs_c  Heart Rate Service Client instance for associating the link.
 *
 * @retval 0 If the operation is successful.
 *           Otherwise, a (negative) error code is returned.
 */
int my_lbs_client_handles_assign(struct bt_gatt_dm *dm, struct my_lbs_client *my_lbs_c);

#endif 