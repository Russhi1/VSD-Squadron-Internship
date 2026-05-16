# VSD Squadron Mini Internship – Firmware Foundations

## Overview

This project demonstrates firmware development using a simulated GPIO firmware library. The objective was to understand how firmware libraries, APIs, and modular code structure simplify hardware interaction in embedded systems.

---

# 1. What is a Firmware Library?

A firmware library is a collection of reusable functions that allow software to communicate with hardware devices such as GPIO pins, timers, UART modules, and sensors.

Instead of writing direct register-level instructions every time, firmware libraries provide higher-level functions that simplify development and improve code readability.

### Key Benefits

* Code reusability
* Faster development
* Better abstraction
* Easier debugging
* Improved modularity


The firmware library acts as a bridge between application logic and hardware registers.

---

# 2. Why APIs are Important in Embedded Systems?

An API (Application Programming Interface) provides predefined functions that allow application code to interact with hardware through a clean and structured interface.

For example:

```c
gpio_write(LED_PIN, 1);
```

This is easier to understand than directly manipulating hardware registers.

### Importance of APIs

* Simplifies hardware control
* Makes code readable
* Reduces programming errors
* Improves portability
* Encourages modular firmware design



APIs create a software layer between application code and hardware-specific implementation.

---

# 3. What Was Understood from the Lab Code?

The lab code demonstrated modular embedded firmware design using three files: `gpio.h`, `gpio.c`, and `main.c`.

## GPIO Header File (`gpio.h`)

The header file defines the GPIO interface and provides constants and function declarations.

### Include Guard

```c
#ifndef GPIO_H
#define GPIO_H
...
#endif
```

This prevents duplicate inclusion during compilation.

### GPIO Direction Definitions

```c
#define GPIO_OUTPUT 1
#define GPIO_INPUT 0
```

These improve readability and reduce errors.

### Function Prototypes

```c
void gpio_init(int pin, int direction);
void gpio_write(int pin, int value);
int gpio_read(int pin);
```

These declare the GPIO API functions available to the application.

---

## GPIO Source File (`gpio.c`)

This file contains the implementation of the GPIO firmware library.

### GPIO Initialization

```c
void gpio_init(int pin, int direction)
```

This configures a pin as input or output.

If configured as output, it prints:

```c
GPIO 5 initialized as OUTPUT
```

If configured as input, it prints:

```c
GPIO 3 initialized as INPUT
```

In real hardware this would configure GPIO control registers.

---

### GPIO Write Function

```c
void gpio_write(int pin, int value)
```

This sends HIGH or LOW signals to a GPIO pin.

Example:

```c
gpio_write(LED_PIN, 1);
```

This turns the LED ON.

---

### GPIO Read Function

```c
int gpio_read(int pin)
```

This reads the logic state of an input pin.

The simulation returns:

```c
return 1;
```

This represents a button press.

In real hardware this would read from an input register.

---

## Main Application File (`main.c`)

This file contains the application logic that uses the GPIO API.

### Pin Definitions

```c
#define LED_PIN 5
#define BTN_PIN 3
```

These define logical pin assignments.

---


### Example Execution Output

```text
Starting firmware application
GPIO 5 initialized as OUTPUT
GPIO 3 initialized as INPUT
GPIO 5 write value: 1
GPIO 3 read value
Button state: 1
GPIO 5 write value: 0
Firmware application finished
```

---

## Key Concepts Learned

From this lab, the following concepts were understood:

* Firmware modularity
* Hardware abstraction
* GPIO initialization and control
* Input/output handling
* API-based firmware design
* Separation between application and driver layers



---

## Conclusion

This lab demonstrated how firmware libraries simplify hardware interaction by abstracting low-level register operations through reusable APIs.

This structured approach improves readability, maintainability, and development efficiency in embedded systems.
