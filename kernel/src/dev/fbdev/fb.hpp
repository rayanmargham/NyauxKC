#pragma once
#include "limine.h"

#include <fs/vfs/fd.h>
#include <arch/x86_64/syscalls/syscall.h>
#include <fs/devfs/devfs.h>
#include <fs/vfs/vfs.h>
#include <stddef.h>
#include <term/term.h>
#include <utils/basic.h>
#include <utils/libc.h>
#ifdef __cplusplus
extern "C" {
#endif
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
  struct fb_fix_screeninfo {
    char id[16];              /* identification string eg "TT Builtin" */
    unsigned long smem_start; /* Start of frame buffer mem */
                              /* (physical address) */
    uint32_t smem_len;        /* Length of frame buffer mem */
    uint32_t type;            /* see FB_TYPE_*                */
    uint32_t type_aux;        /* Interleave for interleaved Planes */
    uint32_t visual;          /* see FB_VISUAL_*              */
    uint16_t xpanstep;        /* zero if no hardware panning  */
    uint16_t ypanstep;        /* zero if no hardware panning  */
    uint16_t ywrapstep;       /* zero if no hardware ywrap    */
    uint32_t line_length;     /* length of a line in bytes    */
    unsigned long mmio_start; /* Start of Memory Mapped I/O   */
                              /* (physical address) */
    uint32_t mmio_len;        /* Length of Memory Mapped I/O  */
    uint32_t accel;           /* Indicate to driver which     */
                              /*  specific chip/card we have  */
    uint16_t capabilities;    /* see FB_CAP_*                 */
    uint16_t reserved[2];     /* Reserved for future compatibility */
  };
#ifdef __cplusplus // C should not access gfx devices directly
}

class GfxDevice {
public:
  struct fb_fix_screeninfo fixedfbinfo;
  struct fb_var_screeninfo variablefbinfo;
  GfxDevice() { kprintf("gfx(): initing gfx device\r\n"); };
  virtual size_t read(thefunny) { return 0; };
  virtual size_t write(thefunny) { return 0; };
  virtual int ioctl(void *data, unsigned long request, void *arg,
                    void *result) {
    return ENOSYS;
  }
  ~GfxDevice() { kprintf("gfx(): destruction of gfx device\r\n"); };
};
class LimineFrameBuffer : public GfxDevice {
private:
public:
  limine_framebuffer *epicfb;
  LimineFrameBuffer(limine_framebuffer *fb) {
    epicfb = fb;
    kprintf("epicfb: redmaskshift %d\r\n", epicfb->red_mask_size);
    // thats all i should rlly care about right
    variablefbinfo.red.msb_right = 0;
    variablefbinfo.red.offset = epicfb->red_mask_shift;
    variablefbinfo.red.length = epicfb->red_mask_size;
    variablefbinfo.blue.msb_right = 0;
    variablefbinfo.blue.offset = epicfb->blue_mask_shift;
    variablefbinfo.blue.length = epicfb->blue_mask_size;
    variablefbinfo.green.msb_right = 0;
    variablefbinfo.green.offset = epicfb->green_mask_shift;
    variablefbinfo.green.length = epicfb->green_mask_size;
    variablefbinfo.xres = epicfb->width;
    variablefbinfo.yres = epicfb->height;
    variablefbinfo.bits_per_pixel = epicfb->bpp;
    variablefbinfo.xres_virtual = variablefbinfo.xres;
    variablefbinfo.yres_virtual = variablefbinfo.yres;
    variablefbinfo.transp.length = 0;
    variablefbinfo.transp.offset = 0;
	fixedfbinfo.line_length = epicfb->pitch;
    kprintf_log(TRACE, "fbdev shit: red offset: %u, red length : %u, blue offset: %u, blue length: %u, green offset: %u, green length: %u\r\n"
      "xres: %u, yres: %u, bpp: %u, xres_virtual: %u, yres_virtual: %u, line_length: %u\r\n", variablefbinfo.red.offset,
    variablefbinfo.red.length,
  variablefbinfo.blue.offset,
variablefbinfo.blue.length,
variablefbinfo.green.offset,
variablefbinfo.green.length,
variablefbinfo.xres,
variablefbinfo.yres,
variablefbinfo.bits_per_pixel,
variablefbinfo.xres_virtual,
variablefbinfo.yres_virtual,
fixedfbinfo.line_length);


    kprintf("gfx::LimineFrameBuffer(): yogurt\r\n");
    kprintf("gfx::LimineFrameBuffer(): gurt: yo\r\n");
  }
  size_t read(thefunny) override {
    return EINVAL;
    size_t end = epicfb->pitch * epicfb->height;
    if (offset >= end) {
      *res = EINVAL;
      return -1;
    }
    if (size + offset > end) {
      size = end - offset;
    }
    void *calculated_dest = reinterpret_cast<void *>(
        reinterpret_cast<uint64_t>(buffer) + static_cast<uint64_t>(offset));
    memcpy(
        calculated_dest,
        epicfb->address, size);
		*res = 0;
    return size;
  }
  size_t write(thefunny) override {
    size_t end = epicfb->pitch * epicfb->height;
    if (offset >= end) {
      *res = EINVAL;
      return -1;
    }

    if (size + offset > end) {
      size = end - offset;
    }
    void *calculated_dest = reinterpret_cast<void *>(
        reinterpret_cast<uint64_t>(epicfb->address) + static_cast<uint64_t>(offset));
    memcpy(
        calculated_dest,
        buffer, size);

		*res = 0;
    return size;
  }
  int ioctl(void *data, unsigned long request, void *arg,
            void *result) override {
    switch (request) {
    case FBIOGET_VSCREENINFO: {
      *static_cast<fb_var_screeninfo*>(arg) = variablefbinfo;
	  *(void**)result = 0;
      return 0;
      break;
    }
    case FBIOGET_FSCREENINFO: {
      *static_cast<fb_fix_screeninfo*>(arg) = fixedfbinfo;
	  *(void**)result = 0;
      return 0;
      break;
    }
    default:
      kprintf_log(ERROR, "fbdev(): unknown ioctl\r\n");
	  break;
    }
    return ENOSYS;
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
  int ioctl(void *data, unsigned long request, void *arg, void *result) {
    return device->ioctl(data, request, arg, result);
  }
  ~FBDev() {
    kprintf("fbdev%lu(): destruction\r\n", fbnum);
    delete device;
  };

private:
};
#endif
