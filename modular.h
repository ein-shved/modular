

#ifndef MODULAR_H
#define MODULAR_H

typedef char name_t [3];
typedef name_t arg_name_t;

typedef name_t method_name_t;
typedef struct {
    method_name_t name;
    uint8_t num_args;
    /* Followed by
     * arg_name_t args[];
     */
} STRUCT_PACKED method_dsc_t;

typedef name_t event_name_t;
typedef struct {
    event_name_t event;
    uint8_t num_args;
    /* Followed by
     * arg_name_t args[];
     */
} STRUCT_PACKED event_dsc_t;

typedef name_t module_type_t;
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
    /* First - is positional number of income event's argument.
     * Second - is positional number of called method's argument.
     * All positional number counts from 1. 0 - is extra values */
    uint8_t args[2][];
} STRUCT_PACKED event_handler_dsc_t;

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
#define EV_NUM_ARGS(ev) ((ev)->msg.size - (uint16_t)((ev)->args - ev))

typedef struct {
    message_t msg;  /* id of receiver module */
    uint8_t method_id; /* Event index */
    /* Followed by msg.size - 6 arguments */
    uint8_t args[];
} STRUCT_PACKED method_t;

#define MT_NUM_ARGS(mt) ((mt)->msg.size - (uint16_t)((mt)->args - mt))

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
    EINVALIDEVENT,
    EOVERFLOW,
    EBUSY,
    ENOTFOUND,
    EFATAL,
};

/* When event received, call next method */
typedef struct {
    module_id_t from_module;
    uint8_t rcv_event;
    uint8_t called_method;
    uint8_t num_args;
    uint8_t ev_max_arg; /* Maximum number in args array */
    /* Index - is the positional number of  method argument
     * Value - is the positional number of event argument, starting from 1 or
     * 0 for default */
    uint8_t args[];
} STRUCT_PACKED event_handler_t;

/* Next four must be implemented by user */
event_handler_t *event_table_getter(uint16_t *out_len, uint16_t *out_maxsize);
void event_table_update_len(uint16_t len);
int process_event(event_handler_t *handler, event_t *event);
void event_table_purge_urgent(void);

int handle_event(message_t *msg);
int append_handler(uint16_t len, uint8_t *buf);
/* Zero rcv_event and called_method to match all */
int remove_handlers(module_id_t from_module, uint8_t rcv_event,
                    uint8_t called_method);

#define for_table(i, table_len, ptr, ...)                                      \
    for (__VA_ARGS__; (i)<(table_len);                                         \
         ++i, ptr =                                                            \
            (event_handler_t *)(((uint8_t *)(ptr + 1)) + handler->num_args))
#define for_each_table(i, table_len, ptr, table)                               \
    for_table(i, table_len, ptr, (i)=0, ptr=table)

#endif /* MODULAR_H */
