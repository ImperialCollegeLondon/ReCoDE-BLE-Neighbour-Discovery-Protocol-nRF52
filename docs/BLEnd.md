
#  What is BLEnd? Theoretical Foundations and Demo Implementation

> **Note:** If you're completely new to BLE, please first read the document [BLE Advertising and Scanning: What You Need to Know](BLE_background_1.md) to ensure a clear understanding of the BLE scanning, advertising, and discovery process.


In many common Bluetooth Low Energy (BLE) applications, a simple client-server model works perfectly well for device discovery. Typically, one device acts as a scanner (which becomes the Central after connection), while another device acts as an advertiser (becoming the Peripheral). This leads to a unidirectional discovery process, where the Central actively searches for Peripherals. A classic example is a phone scanning for a smart device advertising its presence; once found, a connection is established, and application-specific interactions follow.

However, the BLEnd (Practical Continuous Neighbor Discovery for Bluetooth Low Energy) protocol deviates from this simpler "one device scans, one device advertises" setup. Instead, BLEnd is designed to manage the switching between advertising and scanning on all participating devices. This allows for a more dynamic and flexible discovery mechanism.

This sophisticated approach makes BLEnd particularly well-suited for scenarios that lack a fixed master-slave hierarchy. Its primary application lies in environments such as Decentralized Peer-to-Peer Networks and Ad-Hoc Group Formation, where every device needs to be equally capable of **finding** and **being found** by others, fostering neighbor discovery without a predetermined central authority.

## 📄 Related Publication

> **Title**: BLEnd: Practical Continuous Neighbor Discovery for Bluetooth Low Energy  
> **Authors**: Julien, Christine and Liu, Chenguang and Murphy, Amy L and Picco, Gian Pietro    
> **Published in**: IPSN '17: The 16th International Conference on Information Processing in Sensor Networks  
> **URL**: [BLEnd: Practical Continuous Neighbor Discovery for Bluetooth Low Energy] no link first

If you use BLEnd in your work, please cite:

```bibtex
@inproceedings{julien2017blend,
  title={BLEnd: practical continuous neighbor discovery for Bluetooth low energy},
  author={Julien, Christine and Liu, Chenguang and Murphy, Amy L and Picco, Gian Pietro},
  booktitle={Proceedings of the 16th ACM/IEEE International Conference on Information Processing in Sensor Networks},
  pages={105--116},
  year={2017},
  organization={ACM}
}
```
### 💻 Source Code
> The original BLEnd source code is hosted on GitHub: [BLEnd_Nordic](https://github.com/UT-MPC/BLEnd_Nordic.git)

## 📖 BLEnd Protocol Overview
BLEnd segmente time into discrete units called epochs. Within each epoch, devices switch between advertising and scanning roles. This rapid interleaving of activities enables all participating devices to both announce their presence and discover their neighbors efficiently.  

The paper proposes three distinct algorithms for neighbor discovery:
- Uni-directional Discovery (U-BLEnd) features a small duty cycle for low power consumption, with devices active for only just over half of each epoch, but it only guarantees uni-directional discovery.   
- Full-epoch Bi-directional Discovery (F-BLEnd) keeps devices active throughout the entire epoch, utilizing a larger duty cycle to guarantee reliable bi-directional discovery.   
- Efficient Bi-directional Discovery (B-BLEnd) builds upon U-BLEnd by dynamically adjusting its parameters to achieve efficient bi-directional discovery.

Furthermore, the original paper offers detailed methods to calculate the ideal epoch length (E) and advertising interval (A). These calculations are tailored to specific application scenarios, taking into account crucial factors like:

- Device density: How many BLE devices are expected to be in close proximity?
- Power consumption requirements: How critical is battery life for the devices?
- Latency requirements: How quickly do devices need to discover each other?

While the specifics of these optimization algorithms are complex and beyond the scope of this discussion, it's important to understand that epoch length (`E`) and advertising interval (`A`) are quantifiable parameters that can be derived using the proposed methods.

We'll focus on U-BLEnd, as it provides the foundational understanding for BLEnd's approach to discovery. For those interested in the more optimized B-BLEnd, please refer to the original paper.

### U-BLEnd
Within the U-BLEnd protocol, a device's operational cycle is structured into repeating epochs. As depicted in Figure 1, for the initial half of each epoch, the device actively engages its BLE radio for discovery. It first scans for a period, then transitions to advertising. During the latter half of the epoch, the radio transitions to a low-power stand-by state, conserving energy. 

Drawing from the optimization algorithms mentioned earlier, we can determine the system's required epoch length (`E`) and advertising interval (`A`). The scan duration is critically dependent on the advertising interval (`A`) to ensure reliable beacon reception. Since BLE broadcasts have a non-negligible duration (let's denote the beacon duration as `b`), a listener might only receive a partial beacon if its scan window isn't long enough. To counter this, the scan duration must be at least `A+b`. Furthermore, because BLE introduces a random slack to the application-specified advertising interval to avoid persistent collisions, the scan duration needs to be extended to account for this variability. Therefore, the scan window duration must be at least `A+b+s`, where `s` represents the maximum random slack added.

To ensure reliable neighbor detection, the active portion of the epoch, defined as the time from the beginning of scanning to the end of the last beacon, is greater than half of the epoch. This design guarantees a temporal overlap in the active phases of independent devices, resulting in detection. It may also result in bi-directional discovery, as shown in Figure 1 for `Device A` and `Device C`.

  ![U-BLEnd](assets/blend/ublend.png)  
  **Figure 1: U-BLEnd Epoch Structure.**

## ⚙️ Demo Implementation
If you're unsure how to start and manage a timer, the document [How to Use a Timer](introduction_to_Ktimer.md) provides a quick start guide.

In the demo_1, our application's timing and BLE operations are precisely managed by three distinct timers, each serving a specific role: 

- `epoch_timer` defines the fixed duration of each epoch and controlling the repetition of these epochs. Crucially, this timer also initiates scanning at the beginning of every new epoch, ensuring the device periodically attempts to discover nearby nodes.  

- `scan_timeout_timer` determines the duration of each scan operation and, upon its expiration, triggers the transition from scanning to advertising mode.

- `adv_timeout_timer` controls the duration of our device's advertising periods.

### Initialization: Setting Up Timing Parameters
The timing parameters for the BLEnd application are configured during its initialization phase, specifically within the `blend_init()` function. This function takes the desired epoch length `E` and advertising interval `A` as inputs, from which the durations for scanning and advertising are derived.
- Scan Duration:
  
  Formula: Scan Duration = `A+s+b`

  - max_random_delay `s`: Represents the maximum random delay that might be applied to an advertising event.  `s` is assigned a fixed value of 10 milliseconds for the calculation since this random delay typically varies between 0 and 10 milliseconds.

  - beacon_duration `b`: Represents the typical duration of a single BLE beacon transmission. For this implementation, `b` is assigned a fixed value of 5 milliseconds.

- Advertising Duration:

  This is the crucial calculation that determines the overall length of the active part of the epoch (which includes both scanning and subsequent advertising). The goal is to ensure the last beacon transmission effectively finishes after the midpoint of the epoch (`E/2`).

  Logic:

  - First, we determine how many average advertising intervals (`A+5ms`) can fit within the portion of the first half-epoch remaining after accounting for an average random delay of 5ms.

  - We then add 1 to this count to ensure at least one "incomplete" interval is considered.

  - Multiply this advertising interval count" by `A+5ms` to get a base active duration.

  - Finally, we add 15ms (max_random_delay `s` + beacon_duration `b`) to this duration. This explicit addition ensures the end of the last beacon's transmission extends past the `E/2` mark.
    ```c
    static int epoch_period,adv_duration, scan_duration;
    /**
    * @brief Initializes the BLEnd module
    *
    * @param epoch_duration Duration of the epoch in milliseconds
    * @param adv_interval Advertising interval in 0.625 milliseconds
    */
    void blend_init(int epoch_duration, int adv_interval)
    {
        int adv_interval_count;
        epoch_period = epoch_duration;
	      scan_duration = adv_interval* 0.625 +10 + 5;	//one adv_interval + 10ms random delay + 5ms for one advertising packet length
        adv_interval_count = (epoch_duration/2 - scan_duration)/(adv_interval* 0.625 +5);   //an average random delay of 5ms
        adv_interval_count+=1;    //one "incomplete" interval
	      adv_duration =adv_interval_count*(adv_interval* 0.625 +5)+15 ; // 15 ms for the last beacon's transmission 
        LOG_INF("BLEnd init: epoch_period %d ms, adv_duration %d ms, scan_duration %d ms", epoch_period, adv_duration, scan_duration);
    }
    ```

### Workflow: Timer-Driven State Transitions  

The workflow unfolds as follows:  
- Application Start and Epoch Initialization:  

  Upon the BLEnd application's initiation, the `epoch_timer` is started. This timer is configured as a periodic timer with a cycle corresponding to the full epoch length (`E`). Crucially, its initial delay is set to `K_NO_WAIT`, which means the `epoch_timer_handler` is executed immediately after the epoch_timer is started.

  When the `epoch_timer_handler` is invoked (signaling the start of a new epoch):

    - It immediately submits `scan_work` to the system workqueue. This work item, executed in a thread context, is responsible for starting the BLE scanning process.

    - Concurrently, it starts the `scan_timeout_timer`. This is configured as a one-shot timer with an initial delay equal to the predefined scan duration.

- Scanning Timeout and Transition to Advertising:

  When the `scan_timeout_timer` expires (indicating the scan duration has passed), its handler is executed. 
  
  Within this handler:

    - It submits `scan_stop` to the workqueue to terminate the current scanning operation.

    - Immediately following, it submits `adv_work` to the workqueue, which then start the BLE advertising process.

    - Simultaneously, it starts the `adv_timeout_timer`. This is also a one-shot timer, whose initial delay is set to the intended advertising duration.

- Advertising Timeout and Return to Stand-by:

  Upon the `adv_timeout_timer`'s expiration (signaling the end of the advertising duration), its handler is executed. In this handler:

    - It performs the necessary actions to stop the BLE advertising.

  Typically, at this point, the radio enters a low-power stand-by state until the next active period.

- Workflow Repetition:

  Since the `epoch_timer` is a periodic timer, it will continue to expire and trigger its handler at the completion of each epoch length (`E`). This inherent periodicity ensures that the entire sequence — scanning, transitioning to advertising, and then returning to an idle state — repeats seamlessly for every subsequent epoch, maintaining the defined operational cycle of the BLEnd node.
