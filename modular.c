#include "modular.h"

#include <string.h>

int handle_event(message_t *msg)
{
    event_t *event = (event_t *)msg;
    uint16_t table_len, table_max_len, i;
    event_handler_t *handler;
    event_handler_t *table = event_table_getter(&table_len, &table_max_len);

    if (table == NULL)
        return EOK;
    for_each_table(i, table_len, handler, table) {
        if(msg->id == handler->from_module &&
               event->event_id == handler->rcv_event)
            return process_event(handler, event);
    }
    return EOK;
}

int append_handler(uint16_t len, uint8_t *buf)
{
    uint16_t table_len, max_table_size, i;
    event_handler_dsc_t *dsc = (event_handler_dsc_t *)buf;
    event_handler_t *table = event_table_getter(&table_len, &max_table_size);
    event_handler_t *handler;

    if (table == NULL)
        return -EUNSUPPORTED;

    if (len < sizeof (dsc) ||
            len < sizeof(dsc) + dsc->num_args * sizeof(dsc->args[0])) {
        return -EINVALID;
    }

    for_each_table(i, table_len, handler, table) {
        if (handler - table >= max_table_size) {
            event_table_purge_urgent();
            return -EFATAL;
        }
        if (handler->from_module == dsc->from_module &&
                handler->rcv_event == dsc->rcv_event) {
            return -EBUSY;
        }
    }
    if (handler - table + sizeof(*handler) >= max_table_size) {
        return -EOVERFLOW;
    }
    handler->from_module = dsc->from_module;
    handler->rcv_event = dsc->rcv_event;
    handler->called_method = dsc->called_method;
    handler->num_args = dsc->args[0][1];
    handler->ev_max_arg = dsc->args[0][0];

    for (i = 0; i < dsc->num_args; ++i) {
        if (handler->num_args < dsc->args[i][1]) {
            handler->num_args = dsc->args[i][1];
        }
        if (handler->ev_max_arg < dsc->args[i][0]) {
            handler->ev_max_arg = dsc->args[i][0];
        }
    }
    if (handler - table + sizeof(*handler) + handler->num_args >=
            max_table_size) {
        return -EOVERFLOW;
    }
    memset (handler->args, 0, handler->num_args * sizeof(handler->args[0]));
    for (i = 0; i < dsc->num_args; ++i) {
        if (dsc->args[i][1] > 0) {
            handler->args[dsc->args[i][1] - 1] = dsc->args[i][0];
        }
    }
    event_table_update_len(table_len + 1);
    return EOK;
}
int remove_handlers(module_id_t from_module, uint8_t rcv_event,
                    uint8_t called_method)
{
    uint16_t table_len, max_table_size, i, removed = 0;
    event_handler_t *table = event_table_getter(&table_len, &max_table_size);
    event_handler_t *handler, *dst;
    uint8_t num_args;

    if (table == NULL)
        return -EUNSUPPORTED;

    for (i=0,handler=table, dst=handler; i<table_len; ++i) {
        if (handler - table >= max_table_size) {
            event_table_purge_urgent();
            return -EFATAL;
        }
        /* Temporary store this value because it may be overwrote when copying
         * to dst */
        num_args = handler->num_args;
        if (handler->from_module == from_module &&
                (rcv_event == 0 || handler->rcv_event == rcv_event) &&
                (called_method == 0 || handler->called_method == called_method)
           )
        {
            ++removed;
            /* Remove this handler = do not shift dst*/
        } else {
            memcpy(dst, handler, sizeof(handler) + num_args);
            dst = (event_handler_t *)((uint8_t *)(dst + 1) + num_args);
        }
        handler = (event_handler_t *)((uint8_t *)(handler + 1) + num_args);
    }
    if (removed) {
        event_table_update_len(table_len - removed);
    }
    return removed;
}
