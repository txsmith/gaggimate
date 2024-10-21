#include "ProtobufComm.h"

#define BUFFER_SIZE 64
uint8_t buffer[BUFFER_SIZE];

// Function pointers to store the registered callbacks
static temp_control_callback_t temp_control_callback = NULL;
static pin_control_callback_t pin_control_callback = NULL;
static ping_callback_t ping_callback = NULL;
static remote_err_callback_t remote_err_callback = NULL;
static autotune_callback_t autotune_callback = NULL;

// Error handler and unknown message handler function pointers
static error_handler_t error_handler = NULL;
static unknown_message_handler_t unknown_message_handler = NULL;

void protobuf_comm_init(void) {
    uart_init();  // Initialize UART (platform-specific)
}

// Register a callback for TemperatureControl messages
void register_temp_control_callback(temp_control_callback_t callback) {
    temp_control_callback = callback;
}

// Register a callback for PinControl messages
void register_pin_control_callback(pin_control_callback_t callback) {
    pin_control_callback = callback;
}

// Register a callback for Ping messages
void register_ping_callback(ping_callback_t callback) {
    ping_callback = callback;
}

// Register a callback for Error messages
void register_remote_error_callback(remote_err_callback_t callback) {
    remote_err_callback = callback;
}

// Register a callback for PIDAutotune messages
void register_autotune_callback(autotune_callback_t callback) {
    autotune_callback_t = callback;
}

// Register a callback for error handling
void register_error_handler(error_handler_t callback) {
    error_handler = callback;
}

// Register a callback for unknown message handling
void register_unknown_message_handler(unknown_message_handler_t callback) {
    unknown_message_handler = callback;
}

void send_temperature_control(float setpoint) {
    CoffeeMachine_TemperatureControl temp_control = CoffeeMachine_TemperatureControl_init_zero;
    temp_control.setpoint = setpoint;

    pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
    if (!pb_encode(&stream, CoffeeMachine_TemperatureControl_fields, &temp_control)) {
        if (error_handler) {
            error_handler(ERROR_CODE_COMM_SEND);
        }
        return;
    }

    uart_send(buffer, stream.bytes_written);  // Send over UART
}

void send_pin_control(CoffeeMachine_PinControl_Pin pin, bool state) {
    CoffeeMachine_PinControl pin_control = CoffeeMachine_PinControl_init_zero;
    pin_control.pin = pin;
    pin_control.state = state;

    pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
    if (!pb_encode(&stream, CoffeeMachine_PinControl_fields, &pin_control)) {
        if (error_handler) {
            error_handler(ERROR_CODE_COMM_SEND);
        }
        return;
    }

    uart_send(buffer, stream.bytes_written);  // Send over UART
}

void send_ping(bool is_alive) {
    CoffeeMachine_Ping ping = CoffeeMachine_Ping_init_zero;
    ping.is_alive = is_alive;

    pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
    if (!pb_encode(&stream, CoffeeMachine_Ping_fields, &ping)) {
        if (error_handler) {
            error_handler(ERROR_CODE_COMM_SEND);
        }
        return;
    }

    uart_send(buffer, stream.bytes_written);  // Send over UART
}

void send_error(int error_code) {
    CoffeeMachine_Error error = CoffeeMachine_Error_init_zero;
    error.error_code = error_code;

    pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
    if (!pb_encode(&stream, CoffeeMachine_Error_fields, &error)) {
        if (error_handler) {
            error_handler(ERROR_CODE_COMM_SEND);
        }
        return;
    }

    uart_send(buffer, stream.bytes_written);  // Send over UART
}

// Process received messages and invoke appropriate callbacks
void process_received_message(uint8_t *buffer, size_t buffer_size) {
    CoffeeMachine_Command cmd = CoffeeMachine_Command_init_zero;
    pb_istream_t stream = pb_istream_from_buffer(buffer, buffer_size);

    if (pb_decode(&stream, CoffeeMachine_Command_fields, &cmd)) {
        switch (cmd.which_cmd) {
            case CoffeeMachine_Command_temp_control_tag:
                if (temp_control_callback) {
                    temp_control_callback(&cmd.cmd.temp_control);
                }
                break;
            case CoffeeMachine_Command_pin_control_tag:
                if (pin_control_callback) {
                    pin_control_callback(&cmd.cmd.pin_control);
                }
                break;
            case CoffeeMachine_Command_ping_tag:
                if (ping_callback) {
                    ping_callback(&cmd.cmd.ping);
                }
            break;
            case CoffeeMachine_Command_error_tag:
                if (remote_err_callback) {
                    remote_err_callback(&cmd.cmd.error);
                }
            break;
            case CoffeeMachine_Command_pid_autotune_tag:
                if (autotune_callback) {
                    autotune_callback(&cmd.cmd.pid_autotune);
                }
            break;
            default:
                // Unknown or unsupported message type
                if (unknown_message_handler) {
                    unknown_message_handler(cmd.which_cmd);
                }
                break;
        }
    } else {
        // Decoding failed, invoke error handler
        if (error_handler) {
            error_handler(ERROR_CODE_COMM_RCV);
        }
    }
}
