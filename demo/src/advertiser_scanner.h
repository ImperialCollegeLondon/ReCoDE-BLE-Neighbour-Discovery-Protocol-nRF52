#ifndef ADV_SCAN_NONCONN
#define ADV_SCAN_NONCONN

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

/*  Include the header file of the Bluetooth LE stack */
#include <zephyr/bluetooth/bluetooth.h>

/*  Include the header file of the BLE GAP（Generic Access Profile）*/
#include <zephyr/bluetooth/gap.h>
#include <bluetooth/scan.h>

#include <dk_buttons_and_leds.h>

#define MAX_DEVICE_NAME_LEN 30
#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)
// Declare the k_work structs as 'extern'.
// This tells the compiler that these variables exist, but their
// memory is allocated (defined) in another .c file.
extern struct k_work adv_work;
extern struct k_work adv_stop;
extern struct k_work scan_work;
extern struct k_work scan_stop;

void adv_init(int adv_interval);
void scan_init(void);



#define SCAN_LED DK_LED2
#define ADVERTISE_LED DK_LED3


#endif