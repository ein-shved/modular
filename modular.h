

#ifndef MODULAR_H
#define MODULAR_H

typedef char arg_name_t [3];

typedef char method_name_t [3];
typedef struct {
    method_name_t name;
    uint8_t num_args;
    /* Followed by
     * arg_name_t args[];
     */
} STRUCT_PACKED method_dsc_t;

typedef char event_name_t [3];
typedef struct {
    event_name_t event;
    uint8_t num_args;
    /* Followed by
     * arg_name_t args[];
     */
} STRUCT_PACKED event_dsc_t;

typedef char module_type_t [3];
typedef uint16_t module_id_t;
typedef uint8_t block_id_t;

#define BLOCK_OF(m) ((uint8_t)(((m) >> 8) & 0xff))
#define ADDR_OF(m) ((uint8_t)((m) & 0xff))

typedef struct {
    module_type_t type;
    uint8_t num_methods;
    uint8_t num_events;
    /* Followed by
     * method_dsc_t methods[];
     * then by
     * event_dsc_t events[];
     */
} STRUCT_PACKED module_dsc_t;

/* When event received, call next method */
typedef struct {
    module_id_t from_module;
    uint8_t rcv_event;
    uint8_t called_method;
    uint8_t num_args;
    /* First - is positional argument of income event's argument.
     * Second - is positional number of called method's argument. */
    uint8_t args[2][];
} STRUCT_PACKED event_to_method;

enum {
    MSG_NONE        = 0
    MSG_MODULE_DSC  = 1,
    MSG_EVENT       = 2,
    MSG_METHOD      = 3,
};

typedef struct {
    uint8_t msg;
    module_id_t id; /* Receiver or sender */
    uint16_t size; /* size of full message, including size, msg and id fields */
    /* Followed by size - 5 bytes
     * uint8_t data[]; */
} STRUCT_PACKED message_t;

typedef struct {
    message_t msg; /* id of sender module */
    uint8_t event_id; /* Event index */
    /* Followed by msg.size - 6 arguments */
    uint8_t args[];
} STRUCT_PACKED event_t;

typedef struct {
    message_t msg;  /* id of receiver module */
    uint8_t method_id; /* Event index */
    /* Followed by msg.size - 6 arguments */
    uint8_t args[];
} STRUCT_PACKED method_t;

#define MODULE_T(module_dsc_typename) struct {                                \
    message_t msg; /* id of sender module */                                  \
    module_dsc_typename module;                                               \
} STRUCT_PACKED

MODULE_T(module_dsc_t) module_t;

#define ntoh16(val) (val) /* TODO */
#define hton16(val) (val) /* TODO */

enum {
    EOK,
    EUNSUPPORTED,
    EINVALID,
    EINVALIDMETHOD,
    EINVALIDEVENT
};

#endif /* MODULAR_H */
