#ifndef PROTOBUF_COMM_H
#define PROTOBUF_COMM_H

#include "pb_encode.h"
#include "pb_decode.h"
#include "comms.pb.h"
#include "common_uart.h"
#include "codes.h"
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>

// Callback function types for each message type
typedef void (*temp_control_callback_t)(const CoffeeMachine_TemperatureControl *message);
typedef void (*pin_control_callback_t)(const CoffeeMachine_PinControl *message);
typedef void (*ping_callback_t)(const CoffeeMachine_Ping *message);
typedef void (*remote_err_callback_t)(const CoffeeMachine_Error *message);
typedef void (*autotune_callback_t)(const CoffeeMachine_PIDAutotune *message);

// Error and unknown message handlers
typedef void (*error_handler_t)(int error_code);
typedef void (*unknown_message_handler_t)(int message_tag);

// Initialize communication
void protobuf_comm_init(void);

// Register callback functions
void register_temp_control_callback(temp_control_callback_t callback);
void register_pin_control_callback(pin_control_callback_t callback);
void register_ping_callback(ping_callback_t callback);
void register_autotune_callback(autotune_callback_t callback);
void register_remote_error_callback(remote_err_callback_t callback);

// Register callback for error handling
void register_error_handler(error_handler_t callback);

// Register callback for unknown message types
void register_unknown_message_handler(unknown_message_handler_t callback);

// Send a TemperatureControl message
void send_temperature_control(float setpoint);

// Send a PinControl message
void send_pin_control(CoffeeMachine_PinControl_Pin pin, bool state);

// Send a Ping message
void send_ping(bool is_alive);

// Send an Error message
void send_error(int error_code);

// Process received Protobuf message
void process_received_message(uint8_t *buffer, size_t buffer_size);

#endif // PROTOBUF_COMM_H
