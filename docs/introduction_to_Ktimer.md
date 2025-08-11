# How to Use a Timer?

In Zephyr RTOS, a `k_timer` is a fundamental kernel object that allows your application to execute a specific function (a callback) after a defined period of time has expired. It's a precise way to introduce time-based behavior into your concurrent applications.

Think of a `k_timer` as a programmable alarm clock within your embedded system. You set the alarm, and when the time comes, a pre-defined action is triggered.

Here, we'll cover the fundamental steps for using timers in Zephyr. For a more in-depth understanding and advanced configurations, please refer to the [Timer APIs](https://docs.nordicsemi.com/bundle/ncs-2.5.0/page/zephyr/kernel/services/timing/timers.html). 

## Timer Expiry Function

This function is the heart of your timer's functionality. It's a special callback function that Zephyr's kernel automatically executes when a specific timer finishes counting down or "expires."    

For example:  
  ```c
  void my_timer_expiry_handler(struct k_timer *timer_id){
    LOG_INF("My timer expired!");
    // Add your code to run when the timer expires
  }
  ```

## Define and Initialize a Timer.

The `K_TIMER_DEFINE()` macro in Zephyr is a powerful tool for statically defining and initializing a kernel timer.  
  ```c
    K_TIMER_DEFINE(name, expiry_fn, stop_fn)
  ```

  - **Parameters:**    
      - `name`: The name you give to your timer instance.  
      - `expiry_fn`: This is a function pointer to your timer's expiry handler. This function will be called by Zephyr's kernel whenever the timer finishes counting down (i.e., it "expires"). 
      - `stop_fn`: This is an optional function pointer to your timer's stop handler. This function will be called by Zephyr's kernel if the timer is stopped **before** it expires. This is useful for cleanup or specific actions if a timer is canceled early.


## Start the Timer 

This function starts a timer, and resets its status to zero. The timer begins counting down using the specified duration and period values.
    ```c
    void k_timer_start(struct k_timer *timer, k_timeout_t duration, k_timeout_t period)
    ```
  - **Parameters:**
    - `timer`: Address of timer.
    - `duration`  specifies the initial delay before the timer first expires and its `expiry_fn` (callback) is executed.
    - `period` specifies the subsequent interval at which the timer will repeatedly expire after the initial duration has passed. If period is `K_NO_WAIT` or `0`, the timer will only expire once (one-shot).

## Performing Non-Trivial Actions

When a Zephyr `k_timer` expires, its associated expiry function (callback) is executed within the context of the system clock interrupt handler (ISR context). This is a very high-priority execution environment designed for minimal latency with strict limitations.

Many application-level operations, especially those involving complex subsystems like the Bluetooth stack (e.g., `bt_le_adv_start()`, `bt_le_adv_stop()`), are **non-trivial actions**. They involve multiple steps, potentially acquire internal locks, and might even block temporarily. Such operations cannot be safely or reliably performed directly from an ISR context.

- **The Solution: Using the System Workqueue**
To overcome these limitations while still initiating work on a periodic basis, the common and recommended pattern is as follows:

    1. Timer Expiry (ISR Context): When the `k_timer` expires, its expiry function (running in the ISR) performs only one, very quick action: it submits a work item to the system workqueue. This is a safe and non-blocking operation from an ISR.

    2. Workqueue Execution (Thread Context): The system workqueue is managed by its own dedicated kernel thread. This thread runs at a lower priority than ISRs. When the work item is submitted, the workqueue thread will eventually pick it up and execute the associated work handler function.   

### Example
   

   ```c
        void my_work_handler(struct k_work *work)
        {
        /* do the processing that needs to be done*/
        }

        //define and initialize a work item.
        K_WORK_DEFINE(my_work, my_work_handler);

        void my_timer_handler(struct k_timer *timer_id)
        {
          //submit a work item to the system workqueue
          k_work_submit(&my_work);
        }
        // define a timer instance
        K_TIMER_DEFINE(my_timer, my_timer_handler, NULL);
        //start periodic timer that will expire for the first time after 100 milliseconds.
        // After the first expiry, it will repeatedly expire every 500 milliseconds.
        k_timer_start(&my_timer, K_MSEC(100), K_MSEC(500));
  ```
