<!--
This README template is designed with dual purpose.

It should help you think about and plan various aspects of your
exemplar. In this regard, the document need not be completed in
a single pass. Some sections will be relatively straightforward
to complete, others may evolve over time.

Once complete, this README will serve as the landing page for
your exemplar, providing learners with an outline of what they
can expect should they engage with the work.

Recall that you are developing a software project and learning
resource at the same time. It is important to keep this in mind
throughout the development and plan accordingly.
-->


<!-- Your exemplar title. Make it sound catchy! -->
# A BLE (Bluetooth Low Energy) Neighbour Discovery Protocol on nRF52

<!-- A brief description of your exemplar, which may include an image -->

This exemplar demonstrates the implementation of a BLE neighbor discovery protocol on the nRF52 platform using nRF Connect for VS Code. Each device alternates between advertising and scanning according to a scheduled time pattern, enabling unidirectional neighbor discovery. Once a peer is discovered, the device initiates a connection and uses a custom BLE service to exchange data. 

Although Nordic provides a BLE tutorial course, the official examples typically demonstrate a device in the **Peripheral** role (GAP, Generic Access Profile) or **Server** role (GATT, Generic Attribute Profile) interacting with a mobile app. This exemplar goes further by walking through the **Central** and **Client** as well, enabling your devices to interact directly with each other.

<!-- Author information -->
This exemplar was developed at Imperial College London by Sabrina Wang in
collaboration with Jay DesLauriers from Research Software Engineering and
Dan Cummins from Research Computing & Data Science at the Early Career
Researcher Institute.


<!-- Learning Outcomes. 
Aim for 3 - 4 points that illustrate what knowledge and
skills will be gained by studying your ReCoDE exemplar. -->
## Learning Outcomes 🎓

After completing this exemplar, students will:

- Understand basic BLE stack concepts.
- Implement BLE advertising, scanning, and connection.
- Build a simple neighbor discovery and data exchange application using BLE services.


<!-- Audience. Think broadly as to who will benefit. -->
## Target Audience 🎯

* Graduate and undergraduate students new to BLE technology who need practical experience for their research or projects.

* Students seeking a hands-on introduction to BLE neighbor discovery protocols and GATT services.

* Learners interested in wireless communication and low-power device design using nRF52 platforms.

* Anyone aiming to build foundational BLE skills for academic coursework or prototyping.


<!-- Requirements.
What skills and knowledge will students need before starting?
e.g. ECRI courses, knowledge of a programming language or library...

Is it a prerequisite skill or learning outcome?
e.g. If your project uses a niche library, you could either set it as a
requirement or make it a learning outcome above. If a learning outcome,
you must include a relevant section that helps with learning this library.
-->
## Prerequisites ✅

### Academic 📚

- Basic knowledge of C programming (variables, functions, control structures).

- No prior experience with BLE is required — this exemplar includes guidance and references to official BLE tutorials to support beginners.

### System 💻

- A Nordic BLE development board (e.g. nRF52832 DK or nRF52840 DK).

- A USB cable for programming and serial communication.

- A development environment set up by following Lesson 1 of Nordic's official nRF Connect SDK Fundamentals course:

👉 https://academy.nordicsemi.com/courses/bluetooth-low-energy-fundamentals/

    

<!-- Quick Start Guide. Tell learners how to engage with the exemplar. -->
## Getting Started 🚀

1. Before diving into this exemplar, it’s important to ensure your development environment is properly set up. We highly recommend starting with [Lesson 1, Exercise 1](https://academy.nordicsemi.com/courses/nrf-connect-sdk-fundamentals/lessons/lesson-1-nrf-connect-sdk-introduction/topic/exercise-1-1/) from Nordic Semiconductor’s official nRF Connect SDK Fundamentals tutorial. This exercise will guide you step-by-step through installing the essential development tools and configuring your environment for success.

2. Once you’re comfortable with that, take a moment to complete [Exercise 2](https://academy.nordicsemi.com/courses/nrf-connect-sdk-fundamentals/lessons/lesson-1-nrf-connect-sdk-introduction/topic/exercise-2-1/). It offers practical experience in building and flashing applications onto your development board using the nRF Connect SDK. If you’re new to Nordic’s workflow, this exercise will be especially valuable in helping you become familiar with compiling firmware and programming your device smoothly.

3. The [Notebook: demo](notebooks/demo.md) notebook walks you through the beginner-level example in the `demo` folder step by step:
    - Learn the theory: It first refers to the [What is BLEnd? Theoretical Foundations](docs/BLEnd.md) and [BLE Advertising and Scanning: What You Need to Know](docs/BLE_Background.md) documents in the `docs` folder to introduce the basics of BLE and the BLEnd protocol.   
    - Understand the code: Next, it uses the [How to Use a Timer](docs/introduction_to_Ktimer.md) and [Introduction to GAP](docs/introduction_to_GAP.md). document to explain how the example code interacts with the official BLE API (Application Programming Interface).   
    - See the results: Finally, it presents the observed output so you can compare your own results with the expected behavior. In this stage, you should see your devices scanning and advertising, allowing them to discover nearby devices.  

4. The `demo_connect` example is designed as a challenge exercise, encouraging more self-directed learning. It builds on the basics from the `demo` example and introduces additional concepts, such as creating and using a customized GATT service after establishing a connection. The [Notebook: demo_connect](notebooks/demo_connect.md) notebook provides learning resources, service explanation and observed results.



<!-- Repository structure. Explain how your code is structured. -->
## Project Structure 🗂️

Overview of code organisation and structure.

```
.
├── demo
│ ├── src
│ │ ├── advertiser_scanner.c
│ │ ├── advertiser_scanner.h
│ │ ├── blend.c
│ │ ├── blend.h
│ │ ├── main.c
│ ├── CMakeLists.txt
│ ├── prj.conf
├── demo_connect
│ ├── src
│ │ ├── advertiser_scanner.c
│ │ ├── advertiser_scanner.h
│ │ ├── blend.c
│ │ ├── blend.h
│ │ ├── main.c
│ │ ├── my_lbs.c
│ │ ├── my_lbs.h
│ │ ├── my_lbs_client.c
│ │ ├── my_lbs_client.h
│ ├── CMakeLists.txt
│ ├── prj.conf
├── docs
│ ├── BLE_Background.md
│ ├── BLEnd.md
│ ├── introduction_to_GAP.md
│ ├── introduction_to_Ktimer.md
├── notebooks
│ ├── demo.md
│ ├── demo_connect.md
└──  README.md
```

Code is organised into logical components:
- `demo` for beginner-level code, potentially divided into further modules
- `demo_connect` for challenge code, potentially divided into further modules
- `docs` for documentation
- `notebooks` for tutorials and exercises

## Estimated Time ⏳

| Task       | Time    |
| ---------- | ------- |
| Reading    | 4 hours |
| Practising | 2 hours |


<!-- Any references, or other resources. -->
## Additional Resources 🔗

- [nRF Connect SDK Fundamentals](https://academy.nordicsemi.com/courses/nrf-connect-sdk-fundamentals/)
- [Bluetooth Low Energy Fundamentals](https://academy.nordicsemi.com/courses/bluetooth-low-energy-fundamentals/)
<!-- LICENCE.
Imperial prefers BSD-3. Please update the LICENSE.md file with the current year.
-->
## Licence 📄

This project is licensed under the [BSD-3-Clause license](LICENSE.md).
