/*
 * File:   Message.c
 * Author: Jake
 *
 * Created on August 23, 2018, 9:28 PM
 */
#include "BOARD.h"
#include "Message.h"
#include "BattleBoats.h"

typedef enum {
    WAITING_FOR_START_DELIMITER,
    RECORDING_PAYLOAD,
    RECORDING_SUM
} DecodeStates;

DecodeStates currentDecodeState;
char incomingPayload[MESSAGE_MAX_PAYLOAD_LEN];
char incomingChecksum[MESSAGE_MAX_PAYLOAD_LEN];
int payloadLength;
int checksumLength;
BB_Event currentBBEvent;

uint8_t Message_CalculateChecksum(const char* payload) {
    uint8_t checkSum;
    if (payload == NULL) {
        return checkSum;
    } else {
        return checkSum ^ Message_CalculateChecksum(payload + 1);
    }
}

int Message_ParseMessage(const char* payload,
        const char* checksum_string, BB_Event * message_event) {
    uint8_t derivedChecksum;
    // check for errors
    if (strlen(checksum_string) > MESSAGE_CHECKSUM_LEN || message_event->type == MESSAGE_NONE){
        return STANDARD_ERROR;
    }

    // check if checksums equal
    derivedChecksum = Message_CalculateChecksum(payload);
    if (derivedChecksum == checksum_string){
        return SUCCESS;
    } else {
        return STANDARD_ERROR;
    }
}

int Message_Encode(char *message_string, Message message_to_encode) {
    // message_string is output

    // <editor-fold defaultstate="collapsed" desc="identify message type">
    char outgoingPayload[MESSAGE_MAX_PAYLOAD_LEN];
    int outgoingMessageLength;
    // identify message type
    if (message_to_encode.type == MESSAGE_CHA) {
        sprintf(outgoingPayload, PAYLOAD_TEMPLATE_CHA, message_to_encode.param0);

    } else if (message_to_encode.type == MESSAGE_ACC) {
        sprintf(outgoingPayload, PAYLOAD_TEMPLATE_ACC, message_to_encode.param0);

    } else if (message_to_encode.type == MESSAGE_REV) {
        sprintf(outgoingPayload, PAYLOAD_TEMPLATE_REV, message_to_encode.param0);

    } else if (message_to_encode.type == MESSAGE_SHO) {
        sprintf(outgoingPayload, PAYLOAD_TEMPLATE_SHO, message_to_encode.param0,
                message_to_encode.param1);

    } else if (message_to_encode.type == MESSAGE_RES) {
        sprintf(outgoingPayload, PAYLOAD_TEMPLATE_RES, message_to_encode.param0,
                message_to_encode.param1, message_to_encode.param2);
    } else if (message_to_encode.type == MESSAGE_NONE) {
        return STANDARD_ERROR;
    }
    // </editor-fold>

    // <editor-fold defaultstate="collapsed" desc="message wrap">
    // wrap payload
    uint8_t encodingChecksum;
    // get checksum
    encodingChecksum = Message_CalculateChecksum(outgoingPayload);
    // wrap payload
    outgoingMessageLength = sprintf(message_string, MESSAGE_TEMPLATE, outgoingPayload,
            encodingChecksum);
    // return message length
    return outgoingMessageLength;
    // </editor-fold>
}

int Message_Decode(unsigned char char_in, BB_Event * decoded_message_event) {

    switch (currentDecodeState) {

        case WAITING_FOR_START_DELIMITER:
            // <editor-fold defaultstate="collapsed" desc="WAITING_FOR_START_DELIMITER">
            if (char_in == '$') {
                // delimiter detected, change state to RECORDING_PAYLOAD
                currentDecodeState = RECORDING_PAYLOAD;
            }
            break;
            // </editor-fold>

        case RECORDING_PAYLOAD:
            // <editor-fold defaultstate="collapsed" desc="RECORDING_PAYLOAD">
            if (char_in == '$' || char_in == '\n') {
                // unexpected delimiter detected, return state to WAITING_FOR_START_DELIMITER
                currentDecodeState = WAITING_FOR_START_DELIMITER;
                return STANDARD_ERROR;
            }

            if (char_in == '*') {
                // checksum delimiter detected, change state to RECORDING_SUM
                currentDecodeState = RECORDING_SUM;

            } else {
                // for any other char, record and stay in current state
                char payloadBuffer = (char) char_in;
                payloadLength = sprintf(incomingPayload, "%s%c", incomingPayload, payloadBuffer);

            }

            if (payloadLength > MESSAGE_MAX_PAYLOAD_LEN) {
                // payload exceed max, return state to WAITING_FOR_START_DELIMITER
                currentDecodeState = WAITING_FOR_START_DELIMITER;
                return STANDARD_ERROR;

            }
            break;
            // </editor-fold>

        case RECORDING_SUM:
            // <editor-fold defaultstate="collapsed" desc="RECORDING_SUM">
            if (char_in == '0' || char_in == '1' || char_in == '2' || char_in == '3'
                    || char_in == '4' || char_in == '5' || char_in == '6' || char_in == '7'
                    || char_in == '8' || char_in == '9' || char_in == 'a' || char_in == 'b'
                    || char_in == 'c' || char_in == 'd' || char_in == 'e' || char_in == 'f') {
                char checksumBuffer = (char) char_in;
                checksumLength = sprintf(incomingChecksum, "%s%c", incomingChecksum, checksumBuffer);
            }
            if (char_in == '\n') {
                // end of packet detected

                //Message_ParseMessage(incomingPayload, incomingChecksum, currentBBEvent);

                if (Message_CalculateChecksum(incomingPayload)) {
                    // checksum is correct, no cheating detected
                    currentDecodeState = WAITING_FOR_START_DELIMITER;
                    return SUCCESS;
                } else {
                    // checksum is incorrect, ignoring message
                    currentDecodeState = WAITING_FOR_START_DELIMITER;
                    return STANDARD_ERROR;
                }

            } else if (checksumLength > MESSAGE_CHECKSUM_LEN) {
                // checksum exceed max, return state to WAITING_FOR_START_DELIMITER
                currentDecodeState = WAITING_FOR_START_DELIMITER;
                return STANDARD_ERROR;

            } else {
                // anything else, return state to WAITING_FOR_START_DELIMITER
                currentDecodeState = WAITING_FOR_START_DELIMITER;
                return STANDARD_ERROR;
            }
            break;
            // </editor-fold>
    }
}