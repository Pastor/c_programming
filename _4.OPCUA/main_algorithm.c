#include <open62541.h>
#include "main_server.h"

enum EventType {
    Success, Failure
};

struct Detector {
    union {
        struct {
            unsigned int d0: 1;
            unsigned int d1: 1;
            unsigned int d2: 1;
            unsigned int d3: 1;
            unsigned int d4: 1;
            unsigned int d5: 1;
            unsigned int d6: 1;
            unsigned int d7: 1;
            unsigned int d8: 1;
            unsigned int d9: 1;
            unsigned int d10: 1;
            unsigned int d11: 1;
            unsigned int d12: 1;
            unsigned int d13: 1;
            unsigned int d14: 1;
            unsigned int d15: 1;
        } Bits;
        UA_UInt16 origin;
    };
};

#define Bit_IsSet(value, bit)   ((value) & (1 << (bit)))

static void Detector_print(struct Detector detector) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "Показания %u%u%u%u%u%u%u%u%u%u%u%u%u%u%u%u",
                detector.Bits.d15, detector.Bits.d14, detector.Bits.d13, detector.Bits.d12, detector.Bits.d11,
                detector.Bits.d10, detector.Bits.d9, detector.Bits.d8, detector.Bits.d7, detector.Bits.d6,
                detector.Bits.d5, detector.Bits.d4, detector.Bits.d3, detector.Bits.d2, detector.Bits.d1,
                detector.Bits.d0);
}

static void Value_print(UA_UInt16 value) {
    int i, k;
    char buffer[(sizeof(value) * 8 )+ 1];

    for (i = 0, k = (sizeof(value) * 8) - 1; i < sizeof(value) * 8; ++i, --k) {
        buffer[i] = Bit_IsSet(value, k) ? '1' : '0';
    }
    buffer[i] = 0;
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Показания %s", buffer);
}

static UA_StatusCode
create_event(UA_Server *server, char *message, enum EventType type, UA_NodeId *outId) {
    UA_StatusCode ret = UA_Server_createEvent(server, EventType_id, outId);
    if (ret != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                       "createEvent failed. StatusCode %s", UA_StatusCode_name(ret));
        return ret;
    }

    UA_DateTime event_time = UA_DateTime_now();
    UA_Server_writeObjectProperty_scalar(server, *outId, UA_QUALIFIEDNAME(0, "Time"),
                                         &event_time, &UA_TYPES[UA_TYPES_DATETIME]);

    UA_LocalizedText event_message = UA_LOCALIZEDTEXT("ru-RU", message);
    UA_Server_writeObjectProperty_scalar(server, *outId, UA_QUALIFIEDNAME(0, "Message"),
                                         &event_message, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);

    UA_UInt16 event_type = type == Success ? 0 : 1;
    UA_Server_writeObjectProperty_scalar(server, *outId, UA_QUALIFIEDNAME(0, "Type"),
                                         &event_type, &UA_TYPES[UA_TYPES_UINT16]);
    UA_String event_source_name = UA_STRING("Устройство");
    UA_Server_writeObjectProperty_scalar(server, *outId, UA_QUALIFIEDNAME(0, "SourceName"),
                                         &event_source_name, &UA_TYPES[UA_TYPES_STRING]);
    UA_UInt16 event_severity = type == Failure ? 100 : 10;
    UA_Server_writeObjectProperty_scalar(server, *outId, UA_QUALIFIEDNAME(0, "Severity"),
                                         &event_severity, &UA_TYPES[UA_TYPES_UINT16]);
    return UA_STATUSCODE_GOOD;
}

//https://github.com/open62541/open62541/issues/3102
void write_detector_value(UA_Server *server, UA_UInt16 value) {
    UA_NodeId event_id = UA_NODEID_NULL;
    struct Detector detector;

    detector.origin = value;
    Detector_print(detector);
    Value_print(value);
    UA_StatusCode ret = create_event(server, "Ошибка состояния запроса", Failure, &event_id);
    if (ret == UA_STATUSCODE_GOOD) {
        ret = UA_Server_triggerEvent(server, event_id,
                                     UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER),
                                     NULL, UA_TRUE);
        if (ret != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                           "Triggering event failed. StatusCode %s", UA_StatusCode_name(ret));
        } else {
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "Создано событие %lu ", (unsigned long) event_id.identifier.numeric);
        }
    }
}

