#include <Arduino.h>
#include "main.h"

// PID variables
double setpoint = 0.0;   // Desired temperature
double input = 0.0;      // Current temperature input from the thermocouple
double output = 0.0;     // PID output to control heater
PID myPID(&input, &output, &setpoint, 2.0, 5.0, 1.0, DIRECT); // PID tuning parameters
PID_ATune aTune(&input, &output); // PID autotune instance

// System control variables
const double MAX_SAFE_TEMP = 150.0; // Max temperature for thermal runaway protection
long last_ping_time;                // Last time a ping was received
const double PING_TIMEOUT_SECONDS = 10.0; // Timeout for ping
bool is_autotuning = false;           // Flag for whether we are in autotune mode

void setup() {
    // Initialize UART and Protobuf communication
    protobuf_comm_init();
    Serial.begin(115200);

    // Initialize PID controller
    myPID.SetMode(AUTOMATIC);
    myPID.SetOutputLimits(0, 255);  // Example output limits for heater control

    // Register callbacks for message processing
    register_temp_control_callback(on_temperature_control);
    register_pin_control_callback(on_pin_control);
    register_ping_callback(on_ping);
    register_error_handler(on_error);
    register_unknown_message_handler(on_unknown_message);
    register_autotune_callback(on_autotune);

    // Initialize last ping time
    last_ping_time = millis();

    SPI.begin();
    pinMode(MAX6675_CS_PIN, OUTPUT);
    digitalWrite(MAX6675_CS_PIN, HIGH); // Ensure CS is high

    // Initialize heater and pump GPIOs (set to LOW/OFF state)
    pinMode(HEATER_PIN, OUTPUT);
    pinMode(PUMP_PIN, OUTPUT);
    control_heater(false);
    control_pump(false);

    aTune.SetOutputStep(10); // Set the output step size for autotuning
    aTune.SetControlType(1);  // Set to 1 for temperature control
    aTune.SetNoiseBand(1.0);   // Set the noise band
    aTune.SetLookbackSec(10);   // Set the lookback time
}

void loop() {
    uint8_t buffer[64];
    while (1) {
        // Read UART for incoming commands
        if (uart_receive(buffer, sizeof(buffer)) > 0) {
            process_received_message(buffer, sizeof(buffer));  // Process and invoke callback
        }

        // Check the ping timeout for safety
        long now = millis();
        if ((now - last_ping_time) / 1000 > PING_TIMEOUT_SECONDS) {
            handle_ping_timeout();
        }

        // Handle PID control if system is active and not autotuning
        if (setpoint > 0 || is_autotuning) {
            input = read_temperature();  // Read the current boiler temperature

            if (is_autotuning) {
                // Execute autotuning
                if (aTune.Runtime() < 0) { // Check if tuning is complete
                  	printf("Finished autotune: %f, %f, %f\n", aTune.GetKp(), aTune.GetKi(), aTune.GetKd());
                  	myPID.SetTunings(aTune.GetKp(), aTune.GetKi(), aTune.GetKd());
                    stop_pid_autotune(); // Stop autotuning after completion
                }
            } else {
                myPID.Compute(); // Perform PID computation
            }
            control_heater(output); // Control the heater based on the PID output
        } else {
            control_heater(0);
        }

        // Check for thermal runaway
        if (read_temperature() > MAX_SAFE_TEMP) {
            thermal_runaway_shutdown();
        }
    }
}

void on_temperature_control(const CoffeeMachine_TemperatureControl *message) {
    if (is_autotuning) {
        // Ignore temperature control commands during autotuning
        return;
    }
    setpoint = message->setpoint;
    printf("Setpoint updated to: %f\n", setpoint);
}

void on_pin_control(const CoffeeMachine_PinControl *message) {
    switch (message->pin) {
        case CoffeeMachine_PinControl_Pin_PUMP:
            control_pump(message->state);
            break;
        default:
            // Unknown pin
            break;
    }
    printf("Pin Control: Pin %d, State %d\n", message->pin, message->state);
}

void on_error(int error_code) {
  	send_error(error_code);
    printf("Error occurred: %d\n", error_code);
}

void on_unknown_message(int message_tag) {
  	send_error(ERROR_CODE_PROTO_ERR);
    printf("Unknown message received. Tag: %d\n", message_tag);
}

void on_ping(const CoffeeMachine_Ping *message) {
    // Update the last ping time
    last_ping_time = millis();
    printf("Ping received, system is alive.\n");
}

void handle_ping_timeout() {
    printf("Ping timeout detected. Turning off heater and pump for safety.\n");
    // Turn off the heater and pump as a safety measure
    control_heater(0);
    control_pump(false);
    setpoint = 0;
}

void thermal_runaway_shutdown() {
    printf("Thermal runaway detected! Turning off heater and pump!\n");
    // Turn off the heater and pump immediately
    control_heater(0);
    control_pump(false);
    setpoint = 0;
}

void control_heater(int out) {
  	analogWrite(HEATER_PIN, out);
}

void control_pump(bool state) {
    if (state) {
        // Turn on the pump
        digitalWrite(PUMP_PIN, HIGH);
        printf("Pump ON\n");
    } else {
        // Turn off the pump
        digitalWrite(PUMP_PIN, LOW);
        printf("Pump OFF\n");
    }
}

double read_temperature(void) {
    uint16_t v; // Variable to store the temperature value

    // Start communication with the MAX6675
    digitalWrite(MAX6675_CS_PIN, LOW); // Select the MAX6675
    delay(1); // Allow time for the chip to select

    // Read the 16-bit value from the MAX6675
    v = SPI.transfer(0x00) << 8; // Read the first byte
    v |= SPI.transfer(0x00); // Read the second byte
    digitalWrite(MAX6675_CS_PIN, HIGH); // Deselect the MAX6675

    // Check if the reading is valid
    if (v & 0x4) { // Check if the thermocouple is connected
    	printf("Thermocouple not connected...\n");
        return -1; // Return an error value
    }

    // Shift the value right to get the temperature
    v >>= 3; // Remove the status bits
    double temperature = v * 0.25; // Each unit corresponds to 0.25°C

    return temperature; // Return the temperature
}

void on_autotune(const CoffeeMachine_PIDAutotune *message) {
    is_autotuning = true;
    printf("Starting PID autotune...\n");
}

void stop_pid_autotune(void) {
    is_autotuning = false;
    aTune.Cancel();
    printf("PID autotune stopped.\n");
}