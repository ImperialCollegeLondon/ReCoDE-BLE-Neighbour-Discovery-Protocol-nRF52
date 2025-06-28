<!--
This includes your top-level README as you index page i.e. homepage.

This will not be the best approach for all exemplars, so feel free to customise
your index page as you see fit.
-->

{%
include-markdown "../README.md"

%}

<!-- Add more files in the `docs/` directory for them to be automatically
included in the Mkdocs documentation -->
# Introduction to Zephyr API: Generic Access Profile (GAP)
In Zephyr OS, the `zephyr/bluetooth/gap.h` header file provides the essential APIs applications need to interact with the **Bluetooth Generic Access Profile (GAP)**. GAP defines how Bluetooth devices discover each other, connect, and communicate in a secure manner. Understanding this header is crucial for developing any Zephyr-based Bluetooth application.

---

## Core Concepts

* **Roles**: Bluetooth GAP defines various roles a device can assume, primarily including:
    * **Broadcaster**: A device that only sends advertising data (e.g., a beacon).
    * **Observer**: A device that only scans for advertising data.
    * **Central**: A device that scans for and connects to peripheral devices (e.g., a smartphone connecting to Bluetooth headphones).
    * **Peripheral**: A device that can be connected to by a central device (e.g., Bluetooth headphones).
    
* **Operations & Device Interactions**
    * **Advertising & Scanning**:
        * **Advertising**: Advertising: Devices transmit advertising packets containing information like their name and service UUIDs, making themselves discoverable. This is typically performed by **Peripheral** and **Broadcaster** roles.
        * **Scanning**: Devices listen for and parse advertising packets sent by other devices. This is a primary action for **Central** and **Observer** roles.
    * **Connection**: Once devices discover each other and aim to communicate, they establish and manage a connection. GAP handles the lifecycle of these connections. This primarily involves **Central** and **Peripheral** roles.
    <!-- 
        * **Pairing & Bonding**:
            * **Pairing**: The process of establishing shared secret keys for secure communication.
            * **Bonding**: Persisting the keys generated during pairing so that future secure connections can be established without re-pairing.
    -->
* **Address Types**: Bluetooth devices use addresses to uniquely identify themselves. Common address types include:
    * **Public Address**: A globally unique IEEE EUI-48 address. It's predominantly associated with Classic Bluetooth, also known as BR/EDR (Basic Rate/Enhanced Data Rate) connections.
    * **Random Address**: Widely used in Bluetooth Low Energy (BLE) 
    <!-- , random addresses can be:
        * **Static Random Address**: Persists across device resets but differs from a public address.
        * **Resolvable Private Address (RPA)**: A dynamic, frequently changing address that enhances privacy. It can be resolved to a device's real identity using a pre-shared Identity Resolving Key (IRK), making devices harder to track.
        * **Non-Resolvable Private Address (NRPA)**: A randomly generated address that offers no resolution mechanism, ideal for scenarios where no tracking is desired.
    -->

---

## Key API Functions (from `zephyr/bluetooth/gap.h` and related)

While `zephyr/bluetooth/gap.h` itself exposes a limited number of direct APIs, it defines structures and macros relevant to GAP. Actual GAP operations are often performed by combining APIs from `bluetooth/bluetooth.h` and implicitly using definitions from `bluetooth/gap.h`.  
**Usful links**:  
Nordic: https://docs.nordicsemi.com/bundle/zephyr-apis-latest/page/group_bt_gap.html  
Zephyr: https://docs.zephyrproject.org/apidoc/latest/group__bt__gap.html

Here are some common GAP-related operations you'll frequently encounter in Zephyr Bluetooth development:  
### 1. Initializing the Bluetooth Stack

```c
int bt_enable	(	bt_ready_cb_t	cb	)	
```
The bt_enable() function is the essential starting point for any Bluetooth application in Zephyr. You must call this function before attempting any other Bluetooth operations that require communication with the local Bluetooth hardware.
* **Parameters**  
**cb**:  
    Give it a **callback** function: Your program continues running, and Zephyr tells that function when Bluetooth is ready. This is what we call asynchronous operation – tasks happening in the background without blocking your main program flow.  
    Give it **NULL**: Your program pauses and waits until Bluetooth is ready before moving on. This is a synchronous operation. 

* **Returns**
Zero on success or (negative) error code otherwise.
* **Example**
    ```ini
    # in prj.conf file
    # Logger module
    CONFIG_LOG=y
    #  Include the Bluetooth LE stack in your project
    CONFIG_BT=y
    ```
    ```c
    # in .c file
    #include <zephyr/bluetooth/bluetooth.h>
    #include <zephyr/bluetooth/gap.h> 
    int err;

        /* Enable the Bluetooth LE stack */
        err = bt_enable(NULL);
        if (err) {
            LOG_ERR("Bluetooth init failed (err %d)\n", err);
            return; // Handle error appropriately
        }

        LOG_INF("Bluetooth initialized\n");
    ```	
### 2. Configuring Advertising Parameters
#### a. Start advertising
```c
int bt_le_adv_start	(	const struct bt_le_adv_param *	param,
                        const struct bt_data *	ad,
                        size_t	ad_len,
                        const struct bt_data *	sd,
                        size_t	sd_len )

```
In Zephyr, once your Bluetooth stack is initialized (with bt_enable()), the bt_le_adv_start() function is your primary tool for making your device discoverable to others using Bluetooth Low Energy (LE) advertising. This function set advertisement parameters, advertisement data, scan response data and start advertising.  
* **Parameters**  
**bt_le_adv_param *	param**: specifies what fields (variables) are needed to describe Bluetooth Low Energy (LE) advertising parameters.
    ```c
    /** in  zephyr/bluetooth/bluetooth.h */
    /** LE Advertising Parameters. */
    struct bt_le_adv_param {
  
        uint8_t  id;

  
        uint8_t  sid;

        uint8_t  secondary_max_skip;

        uint32_t options;

        uint32_t interval_min;

        uint32_t interval_max;

       
        const bt_addr_le_t *peer;
    };



```



> [NOTE: Scan Response]   

> * What is the Scan Response？ 
>    Scan Response Data (SD): This is a secondary, optional packet of data Your device only sends this data in response to a specific request from a scanning device. 

> * How the Scan Response Process Works?  
>    Advertising: Your BLE device (the advertiser, often a peripheral) regularly sends out its advertising data (AD).  
>    Scanning: Another BLE device (the scanner, often a central) is listening for these advertising packets.  
>    Scan Request: If the scanner receives an advertising packet and wants more information from that specific advertiser, it sends a scan request packet back to the advertiser. This only happens if the advertiser's bt_le_adv_start() parameters are set to be "scannable." **Crucially**, after each advertising event, a scannable advertiser briefly opens its receiver window to listen for these incoming scan requests.  
>    Scan Response: Upon receiving a scan request, the advertiser then sends its scan response data (SD) back to the scanner.  
>    Information Received: The scanner now has both the initial advertising data and the more detailed scan response data for that device.  

> * Why Use Scan Response?  
>    More Data: It allows a device to expose more than the initial 31 bytes of advertising data. This is crucial if you need to provide a complete device name, more service UUIDs, or specific manufacturer data that wouldn't fit in the primary advertising packet.  
>    Efficiency: You don't always need to broadcast all information. By putting less critical or larger chunks of data in the scan response, the device saves power on its regular advertisements, only sending the extra data when explicitly requested.  
>    Active Scanning: The act of a scanner sending a scan request and receiving a scan response is often referred to as "active scanning." When a scanner just listens for advertising packets without sending requests, it's called "passive scanning."  