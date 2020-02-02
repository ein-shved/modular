

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

#define BLOCK_OF(m) (uint8_t)((m) >> 8)

typedef struct {
    uint16_t size;
    module_type_t type;
    module_id_t id;
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
    event_name_t rcv_event;
    method_name_t called_method;
    uint8_t num_args;
    /* First - is positional argument of income event's argument.
     * Second - is positional number of called method's argument. */
    uint8_t args[2][];
} STRUCT_PACKED event_to_method;

#endif /* MODULAR_H */
