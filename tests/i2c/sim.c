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
#include <libgen.h>
#include <limits.h>
#include <pthread.h>

#include <simavr/sim_avr.h>
#include <simavr/avr_twi.h>
#include <simavr/sim_elf.h>
#include <simavr/sim_gdb.h>
#include <simavr/sim_vcd_file.h>



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
    return avr;
}

int main(int argc, char *argv[])
{
    avr_t *master, *slave, *cur_avr;
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

    if (master_gdb > 0) {
        master->gdb_port = master_gdb;
        avr_gdb_init(master);
    }

    if (slave_gdb > 0) {
        slave->gdb_port = slave_gdb;
        avr_gdb_init(slave);
    }

    printf( "\nDemo launching:\n");

    cur_avr = master;
    state = cpu_Running;
    while ((state != cpu_Done) && (state != cpu_Crashed)) {
        state = avr_run(cur_avr);
        cur_avr = cur_avr == master ? slave : master;
    }
}
