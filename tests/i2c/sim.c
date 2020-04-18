/**
 *  Schematic layout:
 *
 *   + -                             + -
 *   . .   . . . . . ___ . . . . .   . .
 *   . .   . . . . *|#U#|* . * *--R--* . i2c SCL line resistor
 *   . .   . . . . *|###|* * | *--R--* . i2c SDA line resistor
 *   . .   . . . . *|###|* | | . .   . .
 *   . .   . . . . *| m |* |i| . .   . .
 *   . .   . . . . *| a |* |2| . .   . .
 *   . .   . . . . *| s |* |c| . .   . .
 *   *-----* . . . *| t |* | | . .   . .
 *   . *---* . . . *| e |* |w| . .   . .
 *   . .   . . . . *| r |* |i| . .   . .
 *   . .   . . . . *|###|* |r| . .   . .
 *   . .   . . . . *|###|* |e| . .   . .
 *   . .   . . . . *|###|* |s| . .   . .
 *   . .   . . . . *|###|* | | . .   . .
 *   . *--R--* * . *|###|* | | *--0--R-* diod and resistor, PORTB[1]
 *   . .   . . | . . --- . | | . .   . .
 *   .Transmit | . .     . | | . .   . .
 *   . request | . .     . | | . .   . .
 *   . .  wire | . .     . | | . .   . .
 *   . .   . . | . . ___ . | | . .   . .
 *   . .   . . | . *|#U#|* | * . .   . .
 *   . .   . . | . *|###|* * . . .   . .
 *   . .   . . | . *|###|* . . . .   . .
 *   . .   . . | . *| s |* . . . .   . .
 *   . .   . . | . *| l |* . . . .   . .
 *   . .   . . | . *| a |* . . . .   . .
 *   *-----* . | . *| v |* . . . .   . .
 *   . *---* . | . *| e |* . . . .   . .
 *   . .   . . | . *|###|* . . . .   . .
 *   . .   . . | . *|###|* . . . .   . .
 *   . .   . . | . *|###|* . . . .   . .
 *   . .   . . | . *|###|* . . . .   . .
 *   . .   . . | . *|###|* . . . .   . .
 *   . *-R-* * | . *|###|* * . . .   . .
 *   . .   . | | . . --- . | . . .   . .
 *   button [\] \_________/
 *   *_______|  . . .     . . . . .   . .
 *   . .   . . . . .     . . . . .   . .
 *   + -                             + -
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include <limits.h>
#include <pthread.h>

#include <simavr/sim_avr.h>
#include <simavr/sim_elf.h>
#include <simavr/sim_gdb.h>
#include <simavr/sim_vcd_file.h>

#include <simavr/avr_twi.h>
#include <simavr/avr_ioport.h>

typedef struct {
    avr_t *master;
    avr_t *slave;
    enum {
        GDB_MASTER = 1 << 0,
        GDB_SLAVE = 1 << 1,
    } gdbs;
    struct {
        avr_irq_t *irq;
        int state;
    } button;
} board_t;

avr_t *prepare_device(const char *fname, const char *mmcu, unsigned long freq)
{
    avr_t *avr = NULL;
    elf_firmware_t  f;

    printf("Firmware pathname is %s\n", fname);
    if (elf_read_firmware(fname, &f)) {
        fprintf(stderr, "Failed to load elf firmware from `%s'\n", fname);
        return NULL;
    }
    if (f.mmcu[0] == '\0') {
        sprintf(f.mmcu, mmcu);
    }
    if (f.frequency == 0) {
        f.frequency = freq;
    }
    printf("firmware %s f=%d mmcu=%s\n", fname, (int)f.frequency, f.mmcu);

    avr = avr_make_mcu_by_name(f.mmcu);
    if (avr == NULL) {
        fprintf(stderr, "Failed make mcu `%s'\n", f.mmcu);
        return NULL;
    }
    /* TODO free allocated data? */
    if (avr_init(avr)) {
        fprintf(stderr, "Failed to init avr with mcu `%s'\n", f.mmcu);
        return NULL;
    }
    avr_load_firmware(avr, &f);
    avr->log = LOG_WARNING;
    return avr;
}

void lamp(struct avr_irq_t *irq, uint32_t value, void *param)
{
    printf("Lamp is switched %s\n", value ? "ON" : "OFF");
}

int prepare_peripherals(board_t *board)
{
    /* Create and attach button to slave */
    const char *names[] = { "button" };
    board->button.irq = avr_alloc_irq(&board->slave->irq_pool, 0, 1, names);
    avr_connect_irq(board->button.irq,
            avr_io_getirq(board->slave, AVR_IOCTL_IOPORT_GETIRQ('B'), 0));
    board->button.state = 0;

    /* Attach transmit request wire from slave to master */
    avr_connect_irq(
        avr_io_getirq(board->slave, AVR_IOCTL_IOPORT_GETIRQ('B'), 1),
        avr_io_getirq(board->master, AVR_IOCTL_IOPORT_GETIRQ('B'), 0));

    /* Connect diod to PORTB[0] of master */
    avr_irq_register_notify(
        avr_io_getirq(board->master, AVR_IOCTL_IOPORT_GETIRQ('B'), 1),
        lamp, board);

    /* Connect i2c lines */
    avr_connect_irq(
        avr_io_getirq(board->master, AVR_IOCTL_TWI_GETIRQ(0), TWI_IRQ_OUTPUT),
        avr_io_getirq(board->slave, AVR_IOCTL_TWI_GETIRQ(0), TWI_IRQ_INPUT));
    avr_connect_irq(
        avr_io_getirq(board->slave, AVR_IOCTL_TWI_GETIRQ(0), TWI_IRQ_OUTPUT),
        avr_io_getirq(board->master, AVR_IOCTL_TWI_GETIRQ(0), TWI_IRQ_INPUT));
}
void set_button(board_t *board, int state)
{
    board->button.state = state;
    printf("Setting button state to %d\n", state);
    avr_raise_irq(board->button.irq, state);
}

static avr_t *master, *slave;
static void
avr_logger(avr_t * avr, const int level, const char * format, va_list ap)
{
#define LOG_TYPE_STR(n) [LOG_ ## n] = #n
    static const char *lvl_str[] = {
        LOG_TYPE_STR(NONE),
        LOG_TYPE_STR(OUTPUT),
        LOG_TYPE_STR(ERROR),
        LOG_TYPE_STR(WARNING),
        LOG_TYPE_STR(TRACE),
        LOG_TYPE_STR(DEBUG),
    };
#undef LOG_TYPE_STR
#define FMT_PREFIX "  %s %s: %s"
    if (level == LOG_OUTPUT) {
        vprintf(format, ap);
        return;
    }
    if (level >= LOG_WARNING) {
        return;
    }
    char _fmt[strlen(format) + sizeof(FMT_PREFIX) + sizeof("WARNING") +
              strlen("MASTER")];
    const char *name = "AVR";
    if (avr == master) {
        name = "[MASTER]";
    }
    if (avr == slave) {
        name = "[SLAVE] ";
    }
    sprintf(_fmt, FMT_PREFIX, name, lvl_str[level], format);
    vprintf(_fmt, ap);
#undef FMT_PREFIX
}
int main(int argc, char *argv[])
{
    board_t board;
    avr_t *cur_avr;
    const char *mmcu = "atmega8";
    unsigned long frequency = 1000000UL;
    unsigned long master_gdb=0, slave_gdb=0;
    int state;

    if (argc < 3) {
        fprintf(stderr, "usage: %s master_fw slave_fw [MCU] [FREQUENCY] "
                "[GDB_MASTER_PORT] [GDB_SLAVE_PORT]\n",
                argv[0]);
        return 1;
    }
    if (argc > 3) {
        mmcu = argv[3];
    }
    if (argc > 4) {
        frequency = strtoul(argv[4], NULL, 0);
        if (frequency == ULONG_MAX) {
            fprintf (stderr, "invalid frequency value: `%s'\n", argv[4]);
        }
    }
    if (argc > 5) {
        master_gdb = strtoul(argv[5], NULL, 0);
        if (master_gdb > 0xffff) {
            fprintf (stderr, "invalid master port value: `%s'\n", argv[5]);
        }
    }
    if (argc > 6) {
        slave_gdb = strtoul(argv[6], NULL, 0);
        if (slave_gdb > 0xffff) {
            fprintf (stderr, "invalid slave port value: `%s'\n", argv[6]);
        }
    }
    master = prepare_device(argv[1], mmcu, frequency);
    slave = prepare_device(argv[2], mmcu, frequency);
    if (master == NULL || slave == NULL) {
        return 1;
    }
    board.master = master;
    board.slave = slave;
    prepare_peripherals(&board);

    avr_global_logger_set(avr_logger);

    if (master_gdb > 0) {
        master->gdb_port = master_gdb;
        avr_gdb_init(master);
        /* Stop cpu to wait for gdb */
        master->state = cpu_Stopped;
    }

    if (slave_gdb > 0) {
        slave->gdb_port = slave_gdb;
        avr_gdb_init(slave);
        /* Stop cpu to wait for gdb */
        slave->state = cpu_Stopped;
    }

    printf( "\nDemo launching:\n");

    cur_avr = master;
    state = cpu_Running;
    uint64_t counter = 0;
    while ((state != cpu_Done) && (state != cpu_Crashed)) {
        state = avr_run(cur_avr);
        if (cur_avr->state != cpu_Stopped) {
            counter = (counter + 1) % 0x1000000;
            if (!counter) {
                set_button(&board, !board.button.state);
            }
        }
        cur_avr = cur_avr == master ? slave : master;
    }
}
