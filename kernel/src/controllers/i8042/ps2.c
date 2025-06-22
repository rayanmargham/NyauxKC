#include "ps2.h"
#include "arch/x86_64/cpu/lapic.h"
#include "arch/x86_64/cpu/structures.h"
#include "arch/x86_64/idt/idt.h"
#include "arch/x86_64/instructions/instructions.h"
#include "arch/x86_64/interrupt_controllers/ioapic.h"
#include "fs/vfs/vfs.h"
#include "term/term.h"
#include "uacpi/namespace.h"
#include <uacpi/acpi.h>
#include <uacpi/helpers.h>
#include <uacpi/resources.h>
#include <uacpi/tables.h>
#include <uacpi/types.h>
#include <uacpi/utilities.h>
static spinlock_t ps2lock = SPINLOCK_INITIALIZER;
static inline uint8_t read_status_reg() {
  return (uint8_t)(arch_raw_io_in(0x64, 1));
};


static inline void write_data_reg(uint8_t data) {
    spinlock_lock(&ps2lock);
    size_t i = 0;
    uint8_t prevstatus = 0;
do {
    i++;
    prevstatus = read_status_reg();
} while (i < 0x2000 && (prevstatus & (1 << 1)));
    if (prevstatus & (1 << 1)) {
         // get rid of any shit we dont care abt
         // yes we should error out here but frankly who cares
         kprintf("WE FUCKING TIMED OUT\r\n");
        spinlock_unlock(&ps2lock);
        
        
    }
  arch_raw_io_write(0x60, (uint8_t)(data), 1);
  spinlock_unlock(&ps2lock);
};

static inline uint8_t read_data_reg() {
    spinlock_lock(&ps2lock);
    size_t i = 0;
    uint8_t prevstatus = 0;
do {
    i++;
    prevstatus = read_status_reg();
} while (i < 0x2000 && (prevstatus & (1)));
    if (prevstatus & (1 << 1)) {
         // get rid of any shit we dont care abt
         // yes we should error out here but frankly who cares
         kprintf("timeout\r\n");
        spinlock_unlock(&ps2lock);
        return 0xff;
        
    }
    uint8_t data = (uint8_t)(arch_raw_io_in(0x60, 1));
  spinlock_unlock(&ps2lock);
  return data;
};
static inline void send_command(uint8_t data) {
    spinlock_lock(&ps2lock);
    size_t i = 0;
    uint8_t prevstatus = 0;
do {
    i++;
    prevstatus = read_status_reg();
} while (i < 0x2000 && (prevstatus & (1 << 1)));
    if (prevstatus & (1 << 1)) {
         // get rid of any shit we dont care abt
         // yes we should error out here but frankly who cares
         kprintf("hmm with data %x\r\n", data);
        spinlock_unlock(&ps2lock);
        
    }
  arch_raw_io_write(0x64, (uint8_t)(data), 1);
  spinlock_unlock(&ps2lock);
};
static bool foundkbd = false;
static bool secondport = false;
struct ps2controller controller = {.inited = false,
                                   .port1 =
                                       {
                                           .type = UNKNOWN,
                                           .usable = false,
                                       },
                                   .port2 = {
                                       .type = UNKNOWN,
                                       .usable = false,
                                   }};
static uacpi_iteration_decision
chk_for_ps2(void *udata, uacpi_namespace_node *node, uint32_t unused) {

  foundkbd = true;
  return UACPI_ITERATION_DECISION_BREAK;
}
static uacpi_iteration_decision
chk_for_mouse(void *udata, uacpi_namespace_node *node, uint32_t unused) {
  kprintf("found ps2 mouse\r\n");
  secondport = true;
  return UACPI_ITERATION_DECISION_BREAK;
}
// this function will send commands
// it will NOT read the identity, that is up to the calling function
void identify_device(bool port2) {
  if (port2)
    send_command(0xD4);
  read_data_reg(); // remove any bytes
ackornat:
  write_data_reg(0xF5); // disable scan codes
  if (!(read_data_reg() == 0xFA))
    goto ackornat;
MIKEWHEREISMYEMAILGUY:
  if (port2)
    send_command(0xD4);
  write_data_reg(0xF2);
  if (!(read_data_reg() == 0xFA))
    goto MIKEWHEREISMYEMAILGUY;
}
typedef enum KEYBOARDSTATE {
  INIT,
  F0,
  E0,
  E0_XX,
  E0_XX_E0,
  E0_XX_E0_XX,
  E0_F0,
  E0_F0_XX,
  E0_F0_XX_E0,
  E0_F0_XX_E0_F0,
  E0_F0_XX_E0_F0_XX,
  E1,
  E1_XX,
  E1_F0,
  E1_F0_XX,
  E1_F0_XX_F0,
} KBDSTATE;
uint8_t pause_buf[7];
int pause_pos;
KBDSTATE state = INIT;
void mike_rebuild_my_kids(nyauxps2kbdpacket packet) {
  sprintf("adding\r\n");
  struct vnode *node = NULL;
  vfs_lookup(NULL, "/dev/keyboard", &node);
  if (!node) 
    return;
  int res = 0;
  node->ops->rw(node, 0, sizeof(nyauxps2kbdpacket), &packet, 1, NULL, &res);
  if (res != 0) {
    panic("death");
  }
}
#ifdef __x86_64__
void *kbd_handler(struct StackFrame *frame) {
  uint8_t scan_code = arch_raw_io_in(0x60, 1);
  switch(state){
    case INIT:
      if (scan_code == 0xF0) {
        state = F0;
      } else if (scan_code == 0xE0) {
        state = E0;
      } else if (scan_code == 0xE1) {
        state = E1;
        pause_pos = 0;
      } else {
        // construct 
        enum ps2_scancode scan = ps2_scancode_from_byte(false, scan_code);
        nyauxps2kbdpacket packet = {.keycode = scan, .flags = PRESSED, .ascii = ps2_scancode_to_ascii(scan)};
        mike_rebuild_my_kids(packet);
      }
      break;
    case F0:
      state = INIT;
     enum ps2_scancode scan = ps2_scancode_from_byte(false, scan_code);
        nyauxps2kbdpacket packet = {.keycode = scan, .flags = RELEASED, .ascii = ps2_scancode_to_ascii(scan)};
        mike_rebuild_my_kids(packet); 
        break;
    case E0:
      if (scan_code == 0xF0) {
        state = E0_F0;
      } else if (scan_code == 0x12) {
        pause_pos = 0;
        state = E0_XX;
      }
      break;
    case E0_F0:
    if (scan_code == 0x7c) {
      state = E0_F0_XX;
      pause_pos = 0;
      
    } else {
      state = INIT;
      enum ps2_scancode scan2 = ps2_scancode_from_byte(true, scan_code);
        nyauxps2kbdpacket packet2 = {.keycode = scan2, .flags = EXTENDEDKEYRELEASED, .ascii = ps2_scancode_to_ascii(scan2)};
        mike_rebuild_my_kids(packet2); }
        break;
    case E0_F0_XX:
      pause_pos++;
      kprintf("scancode %x, pause pos %d\r\n", scan_code, pause_pos);
      if (pause_pos == 3) {
       enum ps2_scancode scan2 = PS2_SC_E0_PRINT_SCREEN;
        nyauxps2kbdpacket packet2 = {.keycode = scan2, .flags = EXTENDEDKEYRELEASED, .ascii = ps2_scancode_to_ascii(scan2)};
        mike_rebuild_my_kids(packet2); 
        state = INIT;
      }
      break;
    case E0_XX:
      pause_pos++;
      if (pause_pos == 2) {
 enum ps2_scancode scan3 = PS2_SC_E0_PRINT_SCREEN;
        nyauxps2kbdpacket packet3 = {.keycode = scan3, .flags = EXTENDEDKEYPRESSED, .ascii = ps2_scancode_to_ascii(scan3)};
        mike_rebuild_my_kids(packet3);
        state = INIT;
      }
      break;
    case E1:
      pause_pos++;
      if (pause_pos == 6) {
           enum ps2_scancode scan3 = PS2_SC_E1_PAUSE;
        nyauxps2kbdpacket packet3 = {.keycode = scan3, .flags = EXTENDEDKEYPRESSED, .ascii = ps2_scancode_to_ascii(scan3)};
        mike_rebuild_my_kids(packet3);
        state = INIT;
      }
     break; 
     default:
      break;
  }
  send_eoi();
  return frame;
}
#endif
int i8042_init() {
  uacpi_find_devices_at(uacpi_namespace_root(), kbd_pnpids, chk_for_ps2, NULL);
  uacpi_find_devices_at(uacpi_namespace_root(), mouse_pnpids, chk_for_mouse,
                        NULL);
  if (foundkbd) {
    kprintf_log(STATUSOK, "found ps2 controller\r\n");
    send_command(0xAD); // disable ps2 port 1
    send_command(0xA7); // disable ps2 port 2
    read_data_reg();    // hope for god something good happens
    send_command(0x20);

    uint8_t configbyte = read_data_reg();
    kprintf_log(TRACE, "config byte before 0x%b\r\n", configbyte);
    configbyte &= ~(1 | (1 << 1) | (1 << 6));
    kprintf_log(TRACE, "config byte after 0x%b\r\n", configbyte);
    send_command(0x60); // tell ps2 we are writing shit back into ram
    write_data_reg(configbyte);
    send_command(0xAA); // test ps2 controllera
    uint8_t ack = read_data_reg();
    if (!(ack == 0x55)) {
      kprintf_log(ERROR, "faulty ps2 controller, ack recvived %x\r\n", ack);
      
      return -1;
    }
    // on some hardware tm the config byte gets reset after testing the ps2
    // controller so we must restore the configbyte

    send_command(0x60); // yk the drill
    write_data_reg(configbyte);
    read_data_reg(); // remove any byte in the first port
    // attempt to enable the second ps2 port

    send_command(0x60);
    write_data_reg(configbyte);
    // test ports
    send_command(0xAB);
    if (!(read_data_reg() == 0)) {
      kprintf_log(ERROR, "no keyboard so exiting ps2\r\n");
      return -1;
    }

    if (secondport) {
      // first clear whatever is in the second port

      read_data_reg();
      send_command(0xA9); // now test
      ack = read_data_reg();
      if (!(ack == 0)) {
        kprintf_log(ERROR,
                    "ps2 port 2 isnt usable, no ps2 port 2 im afraid\r\n");
        secondport = false;
      }
    }
    // reset ps2 devices
    write_data_reg(0xFF);
    ack = read_data_reg();

    if (!(ack == 0xfa || ack == 0xaa)) {
      kprintf_log(ERROR, "failure to reset device\r\n");
      return -1;
    }
    ack = read_data_reg();
    if (!(ack == 0xfa || ack == 0xaa)) {
      kprintf_log(ERROR, "failure to reset device\r\n");
      return -1;
    }

    identify_device(false);
    ack = read_data_reg();
    uint8_t ack2 = read_data_reg();
    kprintf_log(
        LOG,
        "found ps2 device at port 1 with type %d, ack 2 is %d, device is a ",
        ack, ack2);
    if (ack == 0x00 || ack == 0x03 || ack == 0x04) {
      kprintf("mouse\r\n");
      controller.port1.type = MOUSE;
    } else {
      kprintf("keyboard\r\n");
      controller.port1.type = KEYBOARD;
    }
    controller.port1.usable = true;
    if (secondport) {
      send_command(0xD4); // send the next reset to fuckin port 2
      write_data_reg(0xFF);
      ack = read_data_reg();

      if (!(ack == 0xfa || ack == 0xaa)) {
        kprintf_log(ERROR, "failure to reset device\r\n");
        return -1;
      }
      ack = read_data_reg();
      if (!(ack == 0xfa || ack == 0xaa)) {
        kprintf_log(ERROR, "failure to reset device\r\n");
        return -1;
      }
      identify_device(true);
      ack = read_data_reg();
      ack2 = read_data_reg();
      kprintf_log(
          LOG,
          "found ps2 device at port 2 with type %d ack 2 is %d, device is a ",
          ack, ack2);
      if (ack == 0x00 || ack == 0x03 || ack == 0x04) {
        kprintf("mouse\r\n");
        controller.port2.type = MOUSE;
      } else {
        kprintf("keyboard\r\n");
        controller.port2.type = KEYBOARD;
      }
      controller.port2.usable = true;
    }
    int yea = AllocateIrq();
    int epic = -1;
    route_irq(1, yea, 0, get_lapic_id());
RegisterHandler(yea, kbd_handler);
    if (secondport) {
      epic = AllocateIrq();
      route_irq(12, epic, 0, get_lapic_id());
    }
    kprintf_log(STATUSOK, "ps2 init done\r\n");
   resend: 
    write_data_reg(0xF4);
    ack = read_data_reg();
    kprintf("got %d\r\n", ack);
    if (!(ack == 0xFA)) {
      goto resend;
    }

    if (secondport) {
      retry:
      send_command(0xD4);
      
      write_data_reg(0xF4);
      ack = read_data_reg();
      kprintf("got %d\r\n", ack);
      if (!(ack == 0xFA)) {
        goto retry;
      }
    }

    configbyte |= 1;
    configbyte |= (1 << 1);
    // configbyte &= ~((1 << 4) | (1 << 5));
    kprintf("config byte %b\r\n", configbyte);
    send_command(0x60);
    //register the keyboard
    
    
    write_data_reg(configbyte);
    // lets fucking go
    read_data_reg();
    send_command(0xAE);
    send_command(0xA8);

return 0;
  } else {
    kprintf_log(ERROR, "no ps2 kbd found\r\n");
    return -1;
  }
}
