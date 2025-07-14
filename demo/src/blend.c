#include "blend.h"
#include "advertiser_scanner.h"

LOG_MODULE_REGISTER(BLEnd_NONCONN_BLEND, LOG_LEVEL_INF);
/* timer and workqueue handlers
    * These handlers are used to manage the timing of advertising and scanning operations.
    * The epoch timer triggers the start of a new epoch, while the adv and scan timers handle
    * the duration of advertising and scanning respectively.
*/
static void epoch_timer_handler(struct k_timer *timer_id);
static void adv_timeout_timer_handler(struct k_timer *timer_id);
static void scan_timeout_timer_handler(struct k_timer *timer_id);
K_TIMER_DEFINE(epoch_timer, epoch_timer_handler, NULL);
K_TIMER_DEFINE(adv_timeout_timer, adv_timeout_timer_handler, NULL);
K_TIMER_DEFINE(scan_timeout_timer, scan_timeout_timer_handler, NULL);


static int epoch_period,adv_duration, scan_duration;

/**
 * @brief Handler for the advertising timeout timer
 *
 * This function is called when the advertising timeout timer expires. It stops the advertising process
 * and starts the scan process.
 *
 * @param timer_id Pointer to the timer that triggered this handler
 */
void adv_timeout_timer_handler(struct k_timer *timer_id)
{
    LOG_INF(" enter adv_timeout_timer_handler");
	// Stop advertising after broadcasting is done
   k_work_submit(&adv_stop);
   
}

/**
 * @brief Handler for the scan timeout timer
 *
 * This function is called when the scan timeout timer expires. It stops the scanning process
 * and starts the advertising process.
 *
 * @param timer_id Pointer to the timer that triggered this handler
 */
void scan_timeout_timer_handler(struct k_timer *timer_id)
{
	LOG_INF(" enter scan_timeout_timer_handler");
	k_work_submit(&scan_stop);
	k_work_submit(&adv_work);
	k_timer_start(&adv_timeout_timer, K_MSEC(adv_duration), K_NO_WAIT);
    LOG_INF("adv timeout timer started");
}

/**
 * @brief Handler for the epoch timer
 *
 * This function is called when the epoch timer expires. It starts the scanning process
 * and sets up the scan timeout timer.
 *
 * @param timer_id Pointer to the timer that triggered this handler
 */
void epoch_timer_handler(struct k_timer *timer_id)
{
    LOG_INF(" enter epoch_timer_handler");
       k_work_submit(&scan_work);
	   k_timer_start(&scan_timeout_timer, K_MSEC(scan_duration), K_NO_WAIT);
       LOG_INF("scan timeout timer started");
}

/**
 * @brief Initializes the BLEnd module
 *
 * @param epoch_duration Duration of the epoch in milliseconds
 * @param adv_interval Advertising interval in 0.625 milliseconds
 */
void blend_init(int epoch_duration, int adv_interval)
{
    epoch_period = epoch_duration;
	scan_duration = adv_interval* 0.625 +10 + 5;	//one adv_interval + 10ms random delay + 5ms for one advertising packet length
	adv_duration =epoch_duration - scan_duration - 10 ; // avoid the last adv packet to be sent after the epoch timer expires
    LOG_INF("BLEnd init: epoch_period %d ms, adv_duration %d ms, scan_duration %d ms", epoch_period, adv_duration, scan_duration);
}
/**
 * @brief Starts the BLEnd module
 *
 * This function starts the epoch timer, which triggers the start of the BLEnd process.
 */
void blend_start(void)
{
        k_timer_start(&epoch_timer, K_NO_WAIT, K_MSEC(epoch_period));
        LOG_INF("BLEnd start");
}
