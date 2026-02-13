use crate::arch::x86_64::outb;

pub fn serial_init() {
    // stolen from old nyaux
    outb(0x3F8 + 1, 0x00); // Disable all interrupts
    outb(0x3F8 + 3, 0x80); // Enable DLAB (set baud rate divisor)
    outb(0x3F8 + 0, 0x0C); // Set divisor to 3 (lo byte) 9600 baud
    outb(0x3F8 + 1, 0x00); //                  (hi byte)
    outb(0x3F8 + 3, 0x03); // 8 bits, no parity, one stop bit
    outb(0x3F8 + 2, 0xC7); // Enable FIFO, clear them, with 14-byte threshold
    outb(0x3F8 + 4, 0x0B); // IRQs enabled, RTS/DSR set
    outb(0x3F8 + 4, 0x1E); // Set in loopback mode, test the serial chip
    outb(0x3F8 + 0, 0xAE); // Test serial chip (send byte 0xAE and check if serial
                           // returns same byte)
    outb(0x3F8 + 4, 0x0F);
}
pub fn serial_putc(chara: char) {
    outb(0x3F8, chara as u8);
}