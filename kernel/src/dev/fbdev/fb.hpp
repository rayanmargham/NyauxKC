#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include "fs/vfs/fd.h"
#include <arch/x86_64/syscalls/syscall.h>
#include <fs/devfs/devfs.h>
#include <fs/vfs/vfs.h>
#include <stddef.h>
#include <term/term.h>
#include <utils/basic.h>
#include <utils/libc.h>
extern struct devfsops fbdevops;
void devfbdev_init(struct vfs *curvfs);
// stolen from linux source
#define FBIOGET_VSCREENINFO 0x4600
extern volatile struct limine_framebuffer_request framebuffer_request;
#define FBIOPUT_VSCREENINFO 0x4601
#define FBIOGET_FSCREENINFO 0x4602
#define FBIOGETCMAP 0x4604
#define FBIOPUTCMAP 0x4605
#define FBIOPAN_DISPLAY 0x4606
#define thefunny                                                               \
  void *buffer, size_t offset, size_t size, struct FileDescriptorHandle *hnd,  \
      int *res
#define alsofunny buffer, offset, size, hnd, res
struct fb_bitfield {
  uint32_t offset;    /* beginning of bitfield        */
  uint32_t length;    /* length of bitfield           */
  uint32_t msb_right; /* != 0 : Most significant bit is */
                      /* right */
};

struct fb_var_screeninfo {
  uint32_t xres; /* visible resolution           */
  uint32_t yres;
  uint32_t xres_virtual; /* virtual resolution           */
  uint32_t yres_virtual;
  uint32_t xoffset; /* offset from virtual to visible */
  uint32_t yoffset; /* resolution                   */

  uint32_t bits_per_pixel;  /* guess what                   */
  uint32_t grayscale;       /* 0 = color, 1 = grayscale,    */
                            /* >1 = FOURCC                  */
  struct fb_bitfield red;   /* bitfield in fb mem if true color, */
  struct fb_bitfield green; /* else only length is significant */
  struct fb_bitfield blue;
  struct fb_bitfield transp; /* transparency                 */

  uint32_t nonstd; /* != 0 Non standard pixel format */

  uint32_t activate; /* see FB_ACTIVATE_*            */

  uint32_t height; /* height of picture in mm    */
  uint32_t width;  /* width of picture in mm     */

  uint32_t accel_flags; /* (OBSOLETE) see fb_info.flags */

  /* Timing: All values in pixclocks, except pixclock (of course) */
  uint32_t pixclock;     /* pixel clock in ps (pico seconds) */
  uint32_t left_margin;  /* time from sync to picture    */
  uint32_t right_margin; /* time from picture to sync    */
  uint32_t upper_margin; /* time from sync to picture    */
  uint32_t lower_margin;
  uint32_t hsync_len;   /* length of horizontal sync    */
  uint32_t vsync_len;   /* length of vertical sync      */
  uint32_t sync;        /* see FB_SYNC_*                */
  uint32_t vmode;       /* see FB_VMODE_*               */
  uint32_t rotate;      /* angle we rotate counter clockwise */
  uint32_t colorspace;  /* colorspace for FOURCC-based modes */
  uint32_t reserved[4]; /* Reserved for future compatibility */
};
#ifdef __cplusplus // C should not access gfx devices directly
}

class GfxDevice {
public:
  GfxDevice() { kprintf("gfx(): initing gfx device\r\n"); };
  virtual size_t read(thefunny) { return 0; };
  virtual size_t write(thefunny) { return 0; };
  ~GfxDevice() { kprintf("gfx(): destruction of gfx device\r\n"); };
};
class LimineFrameBuffer : public GfxDevice {
public:
  LimineFrameBuffer() {
    kprintf("gfx::LimineFrameBuffer(): yogurt\r\n");
    kprintf("gfx::LimineFrameBuffer(): gurt: yo\r\n");
  }
  size_t read(thefunny) override {
    kprintf("attempted to read limine framebuffer\r\n");
    return 0;
  }
  size_t write(thefunny) override {
    kprintf("attempted to write limine framebuffer\r\n");
    return 0;
  }
};
// this class is allocated per framebuffer
// each framebuffer owns a gfxdevice to which it uses for reading and writing
class FBDev {
public:
  size_t fbnum; // the fb this is
  GfxDevice *device;
  struct fb_var_screeninfo info;
  // i know CLASSES!!!!!!
  // since im in c++. anyways all gfx hardware developed for nyaux must be done
  // with c++ because FUCK YOU what are stars can u eat them? are u a star?
  // *insert deltarune fan game song*
  FBDev(size_t num, GfxDevice *dev) : fbnum(num), device(dev) {
    fbnum = num;
    kprintf("fbdev%lu(): init\r\n", fbnum);
  };
  size_t read(thefunny) { return device->read(alsofunny); }
  size_t write(thefunny) { return device->write(alsofunny); }
  ~FBDev() {
    kprintf("fbdev%lu(): destruction\r\n", fbnum);
    delete device;
  };

private:
};
#endif