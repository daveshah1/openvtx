#include "dma.hpp"
#include "mmu.hpp"
#include "util.hpp"
#include <cassert>
#include <iostream>
namespace VTxx {

DMACtrl::DMACtrl() {
  for (int i = 0; i < 7; i++)
    dma_regs[i] = 0;
}

bool DMACtrl::is_vram_xfer() {
  return get_dst_addr() == 0x2004 || get_dst_addr() == 0x2007;
};

void DMACtrl::write(uint8_t addr, uint8_t data) {
  // cout << "dma write " << (int)addr << " " << (int)data << endl;
  assert(addr <= 6);
  dma_regs[addr] = data;
  if ((addr == 5) && (!is_busy()) /*&& (data != 0)*/) {
    if (is_vram_xfer()) {
      waiting_vblank = true;
    } else {
      do_xfer();
    }
  }
}

uint8_t DMACtrl::read(uint8_t addr) {
  assert(addr <= 6);
  // cout << "dma read " << int(addr) << endl;
  if (addr == 5) {
    return 0x00 | is_busy();
  } else {
    return dma_regs[addr];
  }
}

void DMACtrl::do_xfer() {
  bool is_extsrc = get_bit(get_src_addr(), 15);
  uint32_t srcaddr_c = get_src_addr() & 0x7FFF;
  if (is_extsrc) {
    srcaddr_c |= (dma_regs[4] << 15UL);
    srcaddr_c |= ((dma_regs[6] & 0x03) << 23UL);
  }
  //  srcaddr_c &= ~0x01;
  bool is_extdst = get_bit(get_dst_addr(), 15);
  uint32_t dstaddr_c = get_dst_addr() & 0x7FFF;
  if (is_extdst) {
    dstaddr_c |= (dma_regs[4] << 15UL);
    dstaddr_c |= ((dma_regs[6] & 0x03) << 23UL);
  }

  bool vram_dest = is_vram_xfer();
  if (!vram_dest)
    dstaddr_c &= ~0x01;
  int len = unsigned(dma_regs[5]) * 2;
  if (len == 0)
    len = 512;
  /*if (vram_dest)
    cout << "VDMA " << len << " " << get_dst_addr() << endl;*/
  for (int i = 0; i < len; i++) {
    uint8_t dat =
        is_extsrc ? read_mem_physical(srcaddr_c) : cpu_ram[srcaddr_c & 0x1FFF];

    // TODO: accelerate VRAM writes?
    if (is_extdst)
      write_mem_physical(dstaddr_c, dat);
    else
      write_mem_virtual(dstaddr_c, dat);

    if (!vram_dest)
      dstaddr_c++;
    srcaddr_c++;
  }
  dma_regs[3] = (dma_regs[3] & 0x80) | ((srcaddr_c >> 8) & 0x7F);
  dma_regs[2] = (srcaddr_c & 0xFF);
  if (is_extsrc) {
    // dma_regs[4] = (srcaddr_c >> 15) & 0xFF;
  }
  dma_regs[1] = (dma_regs[1] & 0x80) | ((dstaddr_c >> 8) & 0x7F);
  dma_regs[0] = (dstaddr_c & 0xFF);
}

void DMACtrl::vblank_notify() {
  if (waiting_vblank) {
    waiting_vblank = false;
    do_xfer();
  }
}

void DMACtrl::reset() {
  for (int i = 0; i < 7; i++) {
    dma_regs[i] = 0;
  }
  waiting_vblank = false;
}

} // namespace VTxx
