#include "i8042.h"
#include "term/term.h"
#include <uacpi/acpi.h>
#include <uacpi/types.h>
#include <uacpi/tables.h>
#include <uacpi/helpers.h>
#include <uacpi/utilities.h>
#include <uacpi/resources.h>
static bool foundkbd = false;
static bool secondport = true;
struct ps2controller controller = {
    .inited = false,
    .port1 = {
        .type = UNKNOWN,
        .usable = false,
    },
    .port2 = {
        .type = UNKNOWN,
        .usable = false,
    }
};
static uacpi_iteration_decision
chk_for_ps2(void *udata, uacpi_namespace_node *node, uint32_t unused) {

    foundkbd = true;
  return UACPI_ITERATION_DECISION_BREAK;
}

void i8042_init() {
    for (int i = 0; i < 25; i++) {
        if (foundkbd) {
            break;
        }
        uacpi_find_devices(kbd_pnpids[i], chk_for_ps2, NULL);
    }
    if (foundkbd) {
        kprintf_log(STATUSOK, "found ps2 controller\r\n");
        send_command(0xAD); //disable ps2 port 1
        send_command(0xA7); // disable ps2 port 2
        send_command(0x20);
        uint8_t configbyte = read_data_reg();
        kprintf_log(TRACE, "config byte before 0x%b\r\n", configbyte);
        configbyte |= (1<<1) | (1 << 0); // its okay to enable them as it isnt routed yet.
        // we still need to configure shit and route the irqs
        // give us a minute mr ps2 then we reset this shit
        configbyte &= ~(1 << 6); // disable ps2 translation
        configbyte &= ~(1 << 4); // enable kbd

        kprintf_log(TRACE, "config byte after 0x%b\r\n", configbyte);
        send_command(0x60); // tell ps2 we are writing shit back into ram
        write_data_reg(configbyte);
        send_command(0xAA); // test ps2 controllera
        uint8_t ack = read_data_reg();
        if (!(ack == 0x55)) {
            kprintf_log(ERROR, "faulty ps2 controller, ack recvived %x\r\n", ack);;
            return;
        } 
        // on some hardware tm the config byte gets reset after testing the ps2 controller
        // so we must restore the configbyte

        send_command(0x60); //yk the drill
        write_data_reg(configbyte);
        read_data_reg(); // remove any byte in the first port
        // attempt to enable the second ps2 port
        send_command(0xA8);

        if (configbyte & (1 << 5)) {
            secondport = false;
        } else {
            send_command(0xA7); // disable
        } 
        send_command(0x60);
        write_data_reg(configbyte);
        // test ports
        send_command(0xAB);
        if (!(read_data_reg() == 0)) {
            kprintf_log(ERROR, "no keyboard so exiting ps2\r\n");
            return;
        }

        if (secondport) {
            // first clear whatever is in the second port
            send_command(0xD4);
            read_data_reg();
            send_command(0xA9); // now test
            ack = read_data_reg();
            if (!(ack == 0)) {
                kprintf_log(ERROR,"ps2 port 2 isnt usable, no ps2 port 2 im afraid\r\n");
                secondport = false;
            }
        }
        send_command(0xAE);
        if (secondport) {
            
        send_command(0xA8);
        }
        // reset ps2 devices
        write_data_reg(0xFF);
        ack = read_data_reg();
        
        
        if (!(ack == 0xfa || ack == 0xaa)) {
            kprintf_log(ERROR, "failure to reset device\r\n");
            return;
        }
        ack = read_data_reg();
        if (!(ack == 0xfa || ack == 0xaa)) {
            kprintf_log(ERROR, "failure to reset device\r\n");
            return;
        }
        ack = read_data_reg();
        kprintf_log(LOG, "found ps2 device at port 1 with type %d, device is a ", ack);
        if (ack == 0x00 || ack == 0x03 || ack == 0x04) {
            kprintf("mouse\r\n");
            controller.port1.type = MOUSE;
        } else {
            kprintf("keyboard\r\n");
            controller.port1.type = KEYBOARD;
        }
        controller.port1.usable = true;
        if (secondport) {
            kprintf("doing\r\n");
            send_command(0xD4); // send the next reset to fuckin port 2
            write_data_reg(0xFF);
            ack = read_data_reg();
            
         
            if (!(ack == 0xfa || ack == 0xaa)) {
                kprintf_log(ERROR, "failure to reset device\r\n");
                return;
            }
            ack = read_data_reg();
            if (!(ack == 0xfa || ack == 0xaa)) {
                kprintf_log(ERROR, "failure to reset device\r\n");
                return;
            }
            ack = read_data_reg();
            kprintf_log(LOG, "found ps2 device at port 2 with type %d, device is a ", ack);
            if (ack == 0x00 || ack == 0x03 || ack == 0x04) {
                kprintf("mouse\r\n");
                controller.port2.type = MOUSE;
            } else {
                kprintf("keyboard\r\n");
                controller.port2.type = KEYBOARD;
            }
            controller.port2.usable = true;
        }

        kprintf_log(STATUSOK, "ps2 init done\r\n");
        
    } else {
        kprintf_log(ERROR, "no ps2 kbd found\r\n");
    }
}