.section .text.pagetablesetup
.global page_table_setup
page_table_setup:
   set PAGE_TABLE_BASE, %g1
   !PTD: context=0, index=0, level=0, child_p_addr=0x800, p_addr=0xc00
   ! *(PAGE_TABLE_BASE + 0xc00) = ptd(PAGE_TABLE_BASE + 0x800)
   ! make PTD from 0x800
   set 0x800, %g4
   add %g1, %g4, %g4
   srl %g4, 0x4, %g4
   or  %g4, 0x1, %g4
   ! g4 contains PTD 
   set 0xc00, %g5
   add %g5, %g1, %g3
   st %g4, [%g3]
   ! g4 stored into [g3] 
   !PTE: context=0, index=0, level=1,  ppnr=0x0, p_addr=0x800, cacheable=0x1, acc=0x3
   ! *(PAGE_TABLE_BASE + 0x800) = 0x8e (pte)
   set 0x8e, %g2
   set 0x800, %g5
   add %g5, %g1, %g3
   st %g2, [%g3]
   !PTD: context=0, index=64, level=1, child_p_addr=0x100, p_addr=0x900
   ! *(PAGE_TABLE_BASE + 0x900) = ptd(PAGE_TABLE_BASE + 0x100)
   ! make PTD from 0x100
   set 0x100, %g4
   add %g1, %g4, %g4
   srl %g4, 0x4, %g4
   or  %g4, 0x1, %g4
   ! g4 contains PTD 
   set 0x900, %g5
   add %g5, %g1, %g3
   st %g4, [%g3]
   ! g4 stored into [g3] 
   !PTE: context=0, index=0, level=2,  ppnr=0x40000000, p_addr=0x100, cacheable=0x1, acc=0x3
   ! *(PAGE_TABLE_BASE + 0x100) = 0x400008e (pte)
   set 0x400008e, %g2
   set 0x100, %g5
   add %g5, %g1, %g3
   st %g2, [%g3]
   !PTD: context=0, index=1, level=2, child_p_addr=0x90, p_addr=0x104
   ! *(PAGE_TABLE_BASE + 0x104) = ptd(PAGE_TABLE_BASE + 0x0)
   ! make PTD from 0x90
   set 0x0, %g4
   add %g1, %g4, %g4
   srl %g4, 0x4, %g4
   or  %g4, 0x1, %g4
   ! g4 contains PTD 
   set 0x104, %g5
   add %g5, %g1, %g3
   st %g4, [%g3]
   ! g4 stored into [g3] 
   !PTE: context=0, index=36, level=3,  ppnr=0x50000000, p_addr=0x90, cacheable=0x0, acc=0x3
   ! *(PAGE_TABLE_BASE + 0x90) = 0x500000e (pte)
   set 0x500000e, %g2
   set 0x90, %g5
   add %g5, %g1, %g3
   st %g2, [%g3]
   !PTD: context=0, index=238, level=1, child_p_addr=0x300, p_addr=0xbb8
   ! *(PAGE_TABLE_BASE + 0xbb8) = ptd(PAGE_TABLE_BASE + 0x300)
   ! make PTD from 0x300
   set 0x300, %g4
   add %g1, %g4, %g4
   srl %g4, 0x4, %g4
   or  %g4, 0x1, %g4
   ! g4 contains PTD 
   set 0xbb8, %g5
   add %g5, %g1, %g3
   st %g4, [%g3]
   ! g4 stored into [g3] 
   !PTD: context=0, index=0, level=2, child_p_addr=0x200, p_addr=0x300
   ! *(PAGE_TABLE_BASE + 0x300) = ptd(PAGE_TABLE_BASE + 0x200)
   ! make PTD from 0x200
   set 0x200, %g4
   add %g1, %g4, %g4
   srl %g4, 0x4, %g4
   or  %g4, 0x1, %g4
   ! g4 contains PTD 
   set 0x300, %g5
   add %g5, %g1, %g3
   st %g4, [%g3]
   ! g4 stored into [g3] 
   !PTE: context=0, index=0, level=3,  ppnr=0xee000000, p_addr=0x200, cacheable=0x1, acc=0x3
   ! *(PAGE_TABLE_BASE + 0x200) = 0xee0008e (pte)
   set 0xee0008e, %g2
   set 0x200, %g5
   add %g5, %g1, %g3
   st %g2, [%g3]
   !PTE: context=0, index=1, level=3,  ppnr=0xee001000, p_addr=0x204, cacheable=0x1, acc=0x3
   ! *(PAGE_TABLE_BASE + 0x204) = 0xee0018e (pte)
   set 0xee0018e, %g2
   set 0x204, %g5
   add %g5, %g1, %g3
   st %g2, [%g3]
   !PTE: context=0, index=2, level=3,  ppnr=0xee002000, p_addr=0x208, cacheable=0x1, acc=0x3
   ! *(PAGE_TABLE_BASE + 0x208) = 0xee0028e (pte)
   set 0xee0028e, %g2
   set 0x208, %g5
   add %g5, %g1, %g3
   st %g2, [%g3]
   !PTE: context=0, index=3, level=3,  ppnr=0xee003000, p_addr=0x20c, cacheable=0x1, acc=0x3
   ! *(PAGE_TABLE_BASE + 0x20c) = 0xee0038e (pte)
   set 0xee0038e, %g2
   set 0x20c, %g5
   add %g5, %g1, %g3
   st %g2, [%g3]
   !PTE: context=0, index=4, level=3,  ppnr=0xee004000, p_addr=0x210, cacheable=0x1, acc=0x3
   ! *(PAGE_TABLE_BASE + 0x210) = 0xee0048e (pte)
   set 0xee0048e, %g2
   set 0x210, %g5
   add %g5, %g1, %g3
   st %g2, [%g3]
   !PTD: context=0, index=255, level=1, child_p_addr=0x5f0, p_addr=0xbfc
   ! *(PAGE_TABLE_BASE + 0xbfc) = ptd(PAGE_TABLE_BASE + 0x500)
   ! make PTD from 0x5f0
   set 0x500, %g4
   add %g1, %g4, %g4
   srl %g4, 0x4, %g4
   or  %g4, 0x1, %g4
   ! g4 contains PTD 
   set 0xbfc, %g5
   add %g5, %g1, %g3
   st %g4, [%g3]
   ! g4 stored into [g3] 
   !PTE: context=0, index=60, level=2,  ppnr=0xfff00000, p_addr=0x5f0, cacheable=0x0, acc=0x3
   ! *(PAGE_TABLE_BASE + 0x5f0) = 0xfff000e (pte)
   set 0xfff000e, %g2
   set 0x5f0, %g5
   add %g5, %g1, %g3
   st %g2, [%g3]
   !PTE: context=0, index=61, level=2,  ppnr=0xfff40000, p_addr=0x5f4, cacheable=0x0, acc=0x3
   ! *(PAGE_TABLE_BASE + 0x5f4) = 0xfff400e (pte)
   set 0xfff400e, %g2
   set 0x5f4, %g5
   add %g5, %g1, %g3
   st %g2, [%g3]
   !PTD: context=0, index=63, level=2, child_p_addr=0x4cc, p_addr=0x5fc
   ! *(PAGE_TABLE_BASE + 0x5fc) = ptd(PAGE_TABLE_BASE + 0x400)
   ! make PTD from 0x4cc
   set 0x400, %g4
   add %g1, %g4, %g4
   srl %g4, 0x4, %g4
   or  %g4, 0x1, %g4
   ! g4 contains PTD 
   set 0x5fc, %g5
   add %g5, %g1, %g3
   st %g4, [%g3]
   ! g4 stored into [g3] 
   !PTE: context=0, index=51, level=3,  ppnr=0xffff3000, p_addr=0x4cc, cacheable=0x0, acc=0x3
   ! *(PAGE_TABLE_BASE + 0x4cc) = 0xffff30e (pte)
   set 0xffff30e, %g2
   set 0x4cc, %g5
   add %g5, %g1, %g3
   st %g2, [%g3]
   !PTE: context=0, index=52, level=3,  ppnr=0xffff4000, p_addr=0x4d0, cacheable=0x0, acc=0x3
   ! *(PAGE_TABLE_BASE + 0x4d0) = 0xffff40e (pte)
   set 0xffff40e, %g2
   set 0x4d0, %g5
   add %g5, %g1, %g3
   st %g2, [%g3]
   retl;
   nop;
! done: page_table_setup
! start: set context-table-pointer = PAGE_TABLE_BASE + 0xc00
.global set_context_table_pointer
set_context_table_pointer:
   set PAGE_TABLE_BASE, %g1
   set 0xc00, %g5
   add %g5, %g1, %g2
   srl  %g2, 0x4, %g2
   or  %g2, 0x1, %g2
   set 0x100, %g3
   sta %g2, [%g3] 0x4
   retl;
   nop;
! done: set  context-table-pointer
.align 1024
PAGE_TABLE_BASE: .skip 4096
