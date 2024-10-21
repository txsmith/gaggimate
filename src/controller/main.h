#ifndef MAIN_H
#define MAIN_H

#include "ProtobufComm.h"
#include "PID_v1.h"
#include "PID_AutoTune_v0.h"
#include "common_uart.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <SPI.h>

// GPIO Pin Definitions
#define HEATER_PIN       PB0    // Assuming the heater is controlled via pin PA0
#define PUMP_PIN         PB1    // Assuming the pump is controlled via pin PA1
#define MAX6675_SCK_PIN  PA5    // Replace with your actual SCK pin
#define MAX6675_CS_PIN   PA3    // Replace with your actual CS pin
#define MAX6675_MISO_PIN PA6    // Replace with your actual MISO pin

// Function prototypes for initialization
void setup(void);
void loop(void);

// Function Prototypes
void control_heater(bool state);
void control_pump(bool state);
double read_temperature(void);
void on_temperature_control(const CoffeeMachine_TemperatureControl *message);
void on_pin_control(const CoffeeMachine_PinControl *message);
void on_ping(const CoffeeMachine_Ping *message);
void on_autotune(const CoffeeMachine_PIDAutotune *message);
void on_error(int error_code);
void on_unknown_message(int message_tag);
void handle_ping_timeout(void);
void thermal_runaway_shutdown(void);
void start_pid_autotune(void);
void stop_pid_autotune(void);

#endif //MAIN_H
