#include <Wire.h>
#include <XBee.h>
#include "ccsds_xbee.h"

#define ACTUATOR_PIN 2
#define XBEE_ADDRESS 05
#define LINK_XBEE_ADDRESS 02
#define XBEE_PAN_ID 0x0B0B
#define CYCLE_DELAY 100 // time between execution cycles [ms]

/* function codes */
#define RETRACT_COMMAND 0xF1
#define EXTEND_COMMAND 0xF2
#define STATUS_REQUEST 0x0E

/* behavioral constants */
#define CYCLE_DELAY 100 // time between execution cycles [ms]

/* response definitions */
#define INIT_RESPONSE 0xAC
#define READ_FAIL_RESPONSE 0xAF
#define BAD_COMMAND_RESPONSE 0xBB
#define RETRACT_RESPONSE 0xE1
#define EXTEND_RESPONSE 0xE2

// APIDs
#define COMMAND_RESPONSE_APID 500 // not used yet
#define STATUS_RESPONSE_APID 510

bool extended = false;

void setup()
{
    Serial.begin(9600);

    if (!InitXBee(XBEE_ADDRESS, XBEE_PAN_ID, Serial))
    {
        // it initialized
        sendCommandResponse(INIT_RESPONSE);
    }
    else
    {
        // you're fucked
    }

    pinMode(ACTUATOR_PIN, OUTPUT);
}

void loop()
{
    // look for any new messages
    executeIncomingCommands();
    delay(CYCLE_DELAY);
}

void executeIncomingCommands()
{
    int pkt_type;
    int bytes_read;
    uint8_t command_byte;
    uint8_t incoming_bytes[100];

    if ((pkt_type = readMsg(1)) == 0)
    {
        // Read something else, try again
    }

    // if we didn't have a read error, process it
    if (pkt_type > -1)
    {
        if (pkt_type)
        {
            delay(20000);
            bytes_read = readCmdMsg(incoming_bytes, command_byte);
            if (command_byte == EXTEND_COMMAND)
            {
                extend(6);
            }
            else if (command_byte == RETRACT_COMMAND)
            {
                retract(15);
            }
            else if (command_byte == STATUS_REQUEST)
            {
                if (extended)
                {
                    sendStatusResponse(EXTEND_COMMAND);
                }
                else
                {
                    sendStatusResponse(RETRACT_COMMAND);
                }
            }
            else
            {
                sendCommandResponse(BAD_COMMAND_RESPONSE);
            }
        }
        else
        {
            // unknown packet type?
            sendStatusResponse(READ_FAIL_RESPONSE);
        }
    }
}

void sendCommandResponse(uint8_t msg)
{
    uint8_t tlm_data;
    addIntToTlm<uint8_t>(msg, &tlm_data, (uint16_t) 0);
    sendTlmMsg(LINK_XBEE_ADDRESS, COMMAND_RESPONSE_APID, &tlm_data, (uint16_t) 1);
}

void sendStatusResponse(uint8_t msg)
{
    uint8_t tlm_data;
    addIntToTlm<uint8_t>(msg, &tlm_data, (uint16_t) 0);
    sendTlmMsg(LINK_XBEE_ADDRESS, STATUS_RESPONSE_APID, &tlm_data, (uint16_t) 1);
}

void extend(int pulse_seconds)
{
    controlActuator("extend", pulse_seconds, EXTEND_RESPONSE);
    extended = true;
}

void retract(int pulse_seconds)
{
    controlActuator("retract", pulse_seconds, RETRACT_RESPONSE);
    extended = false;
}

void controlActuator(String direction, int pulse_seconds, uint8_t message)
{
    // actuator with built-in polarity switch
    int frequency = 0;
    if (direction == "retract")
    {
        frequency = 1;
    }
    else if (direction == "extend")
    {
        frequency = 2;
    }
    else
    {
        sendCommandResponse(BAD_COMMAND_RESPONSE);
        return;
    }

    // send signal: 1 ms retract, 2 ms extend
    unsigned long start_milliseconds = millis();
    while (millis() <= start_milliseconds + (pulse_seconds * 1000))
    {
        digitalWrite(ACTUATOR_PIN, HIGH);
        delay(frequency);
        digitalWrite(ACTUATOR_PIN, LOW);
        delay(frequency);
    }
    delay(500);

    sendCommandResponse(message);
}
