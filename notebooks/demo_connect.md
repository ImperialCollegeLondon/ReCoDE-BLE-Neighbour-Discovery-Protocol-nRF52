# Demo_connect Notebook
## Introduction   
In the previous example from the `demo` folder, you learned how to configure advertising and scanning to enable device discovery using BLE. In this example, we take it a step further: once devices discover each other, they proceed to establish a connection and demonstrate basic data exchange through a custom service.

Unlike the previous demo, this example is presented as a challenge, requiring some self-guided learning. To prepare, we highly recommend reading [Lesson 4](https://academy.nordicsemi.com/courses/bluetooth-low-energy-fundamentals/lessons/lesson-4-bluetooth-le-data-exchange/) from the Nordic Bluetooth Low Energy Fundamentals course. It provides essential background on how GATT (Generic Attribute Profile) services and characteristics are used to structure and transfer data in a BLE connection.

## LED Button Service (LBS)
While Lesson 4, Exercises 1 and 2 in the Nordic tutorial provide a detailed walkthrough of configuring an LBS server, including characteristic read/write operations and sending indications, their example focuses on interaction between a BLE device (as the server) and a mobile app (as the client).

In contrast, this `demo_connect` is designed to demonstrate device-to-device communication, where both the **client** and **server** are embedded BLE devices. To simplify the setup, the server only provides a basic Button Characteristic, and the emphasis is placed on client-side configuration, including service discovery, characteristic subscription, and handling received indications.

The goal is to show how two BLE-enabled devices can autonomously discover each other, establish a connection, and exchange data using GATT, without relying on external apps or user interaction via smartphones.

In this setup:

- The server provides a Button Characteristic, which reflects the state of a physical button on the device.

- The client first performs GATT service discovery to locate the LBS on the server. Once the service is found, the client subscribes to the Button Characteristic.

- From that point on, whenever the server detects a change in the button state, it sends an indication to the client. 

- This allows the client to receive real-time updates whenever the server’s button is pressed or released, illustrating how BLE GATT indications can be used for efficient event-driven communication.

Due to the higher memory requirements of the **central** role, please make sure to use two BLE development boards that support **central** functionality.

Build and flash the `demo_connect` application onto both devices. Once running, each device will first execute the BLEnd protocol to discover nearby peers through advertising and scanning. After discovery, one device will initiate a connection to the other and proceed with GATT-based service interaction, completing the full discovery-and-connect flow.

### LED indicators 
As in the previous demo, LED2 and LED3 are used to indicate the scanning and advertising phases, respectively. In this example, we also utilize **LED1** and **LED4** to indicate the device’s role after a connection is established:

**LED1** turns on if the device is acting as a **peripheral**.

**LED4** turns on if the device is acting as a **central**.

In the BLE GATT service, the peripheral functions as the **server**, while the central acts as the **client**.

For the **central/client** device, **LED1** serves an additional purpose: it reflects the button state received from the server via indications. When a button pressed indication is received, LED1 turns on. When a button released message is received, LED1 turns off. This provides real-time, visible feedback that the central device is successfully receiving and interpreting the server's characteristic updates.

### Server

The files `my_lbs.c` and `my_lbs.h` contain the code for the LBS server, adapted from Lesson 4, Exercises 1 and 2 in the Nordic tutorial. Since the implementation closely follows the original example, we won’t go into detail here.

As a quick test, you can build and flash the server code onto a development board and interact with it using a mobile app (e.g., nRF Connect for Mobile). This allows you to verify that the Button Characteristic is functioning correctly before proceeding with full device-to-device interaction.

To demonstrate the server functionality, you can use the nRF Connect for Mobile app  as the client.   

- Scan for nearby BLE devices in the app. Look for the device named `my_LBS`. Once found, connect to the device.    
    ![server1](assets/demo_connect/server1.png)  

- The app will automatically discover the services provided by the server. Locate the Button Characteristic, and tap the button outlined in red to subscribe to indications.  

    ![server2](assets/demo_connect/server2.png)

- Now, when you press the physical button (Button 1) on the development board, the server sends a Button State indication, which is received and displayed by the mobile app. 

    ![server3](assets/demo_connect/server3.png)
    ![server4](assets/demo_connect/server4.png)

### Client
From the interaction between the server and the mobile app, we can clearly see the key responsibilities of a central device in this context:

1. Scan for the target peripheral device and initiate a connection once the device is discovered.

2. Perform service discovery to locate the desired GATT service and characteristics.

3. Subscribe to indications from the Button State characteristic to receive updates.

These same steps will be implemented in the client-side firmware when setting up communication between two development boards.

#### Initiate a connection
- **Advertiser:**
    First, we configure the device to broadcast **connectable** advertisements instead of non-connectable beacons, allowing other devices to initiate a connection once discovered.   

    in `demo_connect/src/advertiser_scanner.c`   
    ```c
        /* BLE Advertising Parameters variable */
        static struct bt_le_adv_param *adv_param =
            BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE, /* connectable */
                    500, /* assign an initial value first */
                    500, /* assign an initial value first */
                    NULL); /* Set to NULL for undirected advertising */
    ```
- **Scanner:**
    On the scanning side, we continue to use a Manufacturer Data filter within the scan module. When a device detects an advertisement that matches this filter, it immediately initiates a connection to the advertising peer. This enables automatic pairing between BLEnd-enabled devices without user interaction.   

    in `demo_connect/src/advertiser_scanner.c`     
    ```c
        struct bt_scan_init_param scan_init = {
        .scan_param = &my_scan_param,
        .connect_if_match = true,
        };
    ```
- **Connection:**
    The functions and structures used for connection management are detailed in the documentation [Introduction to GAP](../docs/introduction_to_GAP.md).   

    In `main.c`, we declare a static pointer to hold the active connection. This variable is used throughout the application to track the current BLE connection and interact with the connected peer. We also register connection callbacks using a `bt_conn_cb` structure. These callbacks handle key connection events such as connection established, disconnected. Registering these callbacks allows the application to respond to connection state changes appropriately.

    ```c
       static struct bt_conn *default_conn = NULL;
       struct bt_conn_cb connection_callbacks = {
            .connected = on_connected,
	        .disconnected = on_disconnected,
        };
        int main(void){
            ...
            // Register the connection callbacks 
	        bt_conn_cb_register(&connection_callbacks);
            ...
        }
    ```
    Next, we implement the connection and disconnection callback functions to manage the BLE connection lifecycle.

    - Inside the **connection** callback function, we first stop the BLEnd protocol because `CONFIG_BT_MAX_CONN` is set to 1 in the `prj.conf` file. Then, we increase the reference count to safely keep track of the active connection, followed by retrieving detailed connection information. The `info.role` field indicates whether the device is currently acting as a central or peripheral. Based on this role, we turn on the corresponding LED to visually indicate the device’s role after the connection is established.
    ```c
    #define CONN_LED_PERIPHERAL DK_LED1
    #define CONN_LED_CENTRAL DK_LED4
    static void on_connected(struct bt_conn *conn, uint8_t err)
    {
        int err_dm;
        struct bt_conn_info info = {0};
        /*  check errors ... */

        blend_stop();

        default_conn = bt_conn_ref(conn);
        err_dm = bt_conn_get_info(default_conn, &info);
        if (err_dm) {
            LOG_ERR("Failed to get connection info %d\n", err);
            return;
        }
        if (info.role == BT_CONN_ROLE_PERIPHERAL) {
                LOG_INF("Connected: BT_CONN_ROLE_PERIPHERAL\n");
            dk_set_led_on(CONN_LED_PERIPHERAL);
        }
        if (info.role == BT_CONN_ROLE_CENTRAL) {
            LOG_INF("Connected: BT_CONN_ROLE_CENTRAL\n");
            dk_set_led_on(CONN_LED_CENTRAL);
            /*  service discovery (next step) ... */
        }      
    }
    ```
    - In the **disconnect** callback function, we log the reason for disconnection, unreference the connection object and set `default_conn` back to `NULL` since there is no active connection anymore. After that, we restart the BLEnd protocol to resume neighbor discovery following the disconnection.
    ```c
    static void on_disconnected(struct bt_conn *conn, uint8_t reason)
    {
        LOG_INF("Disconnected (reason %u)\n", reason);

        dk_set_led_off( CONN_LED_CENTRAL);
        dk_set_led_off(CONN_LED_PERIPHERAL);

        bt_conn_unref(default_conn);
        default_conn = NULL;

        blend_start();
    }
    ```
####  Service Discovery

### Demo Results 📡
Now, build and flash the `demo_connect` application onto both boards. After a short time, BLEnd will connect the two devices automatically. The peripheral device will turn on LED1, while the central device will turn on LED4 to indicate their respective roles.   

![service_results1](assets/demo_connect/service_results1.png)

When you press **Button 1** on the **peripheral** board, LED1 on the central board will turn on. Releasing Button 1 on the peripheral will turn LED1 off on the central, reflecting the button state change via BLE indications.   

![service_results2](assets/demo_connect/service_results2.png)

We can also open the RTT terminals on both devices to observe more detailed logs. For example, if we reset one device, the other will log the disconnection reason 8 (The supervision timeout has expired). Then, both devices automatically start the BLEnd protocol. Once a neighbor is discovered, the scanner initiates a connection, performs service discovery, and starts receiving indications after the connection is established.  

![RTT_service](assets/demo_connect/RTT_service.png)