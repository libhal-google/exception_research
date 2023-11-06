#include <unwind.h>

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <limits>
#include <span>
#include <string_view>

volatile std::int32_t side_effect = 0;
std::uint32_t start_cycles = 0;
std::uint32_t end_cycles = 0;

/// Structure type to access the Data Watch point and Trace Register (DWT).
struct dwt_register_t
{
  /// Offset: 0x000 (R/W)  Control Register
  volatile uint32_t ctrl;
  /// Offset: 0x004 (R/W)  Cycle Count Register
  volatile uint32_t cyccnt;
  /// Offset: 0x008 (R/W)  CPI Count Register
  volatile uint32_t cpicnt;
  /// Offset: 0x00C (R/W)  Exception Overhead Count Register
  volatile uint32_t exccnt;
  /// Offset: 0x010 (R/W)  Sleep Count Register
  volatile uint32_t sleepcnt;
  /// Offset: 0x014 (R/W)  LSU Count Register
  volatile uint32_t lsucnt;
  /// Offset: 0x018 (R/W)  Folded-instruction Count Register
  volatile uint32_t foldcnt;
  /// Offset: 0x01C (R/ )  Program Counter Sample Register
  volatile const uint32_t pcsr;
  /// Offset: 0x020 (R/W)  Comparator Register 0
  volatile uint32_t comp0;
  /// Offset: 0x024 (R/W)  Mask Register 0
  volatile uint32_t mask0;
  /// Offset: 0x028 (R/W)  Function Register 0
  volatile uint32_t function0;
  /// Reserved 0
  std::array<uint32_t, 1> reserved0;
  /// Offset: 0x030 (R/W)  Comparator Register 1
  volatile uint32_t comp1;
  /// Offset: 0x034 (R/W)  Mask Register 1
  volatile uint32_t mask1;
  /// Offset: 0x038 (R/W)  Function Register 1
  volatile uint32_t function1;
  /// Reserved 1
  std::array<uint32_t, 1> reserved1;
  /// Offset: 0x040 (R/W)  Comparator Register 2
  volatile uint32_t comp2;
  /// Offset: 0x044 (R/W)  Mask Register 2
  volatile uint32_t mask2;
  /// Offset: 0x048 (R/W)  Function Register 2
  volatile uint32_t function2;
  /// Reserved 2
  std::array<uint32_t, 1> reserved2;
  /// Offset: 0x050 (R/W)  Comparator Register 3
  volatile uint32_t comp3;
  /// Offset: 0x054 (R/W)  Mask Register 3
  volatile uint32_t mask3;
  /// Offset: 0x058 (R/W)  Function Register 3
  volatile uint32_t function3;
};

/// Structure type to access the Core Debug Register (CoreDebug)
struct core_debug_registers_t
{
  /// Offset: 0x000 (R/W)  Debug Halting Control and Status Register
  volatile uint32_t dhcsr;
  /// Offset: 0x004 ( /W)  Debug Core Register Selector Register
  volatile uint32_t dcrsr;
  /// Offset: 0x008 (R/W)  Debug Core Register Data Register
  volatile uint32_t dcrdr;
  /// Offset: 0x00C (R/W)  Debug Exception and Monitor Control Register
  volatile uint32_t demcr;
};

/// Address of the hardware DWT registers
constexpr intptr_t dwt_address = 0xE0001000UL;

/// Address of the Cortex M CoreDebug module
constexpr intptr_t core_debug_address = 0xE000EDF0UL;

// NOLINTNEXTLINE
auto* dwt = reinterpret_cast<dwt_register_t*>(dwt_address);

// NOLINTNEXTLINE
auto* core = reinterpret_cast<core_debug_registers_t*>(core_debug_address);

void
dwt_counter_enable()
{
  /**
   * @brief This bit must be set to 1 to enable use of the trace and debug
   * blocks:
   *
   *   - Data Watchpoint and Trace (DWT)
   *   - Instrumentation Trace Macrocell (ITM)
   *   - Embedded Trace Macrocell (ETM)
   *   - Trace Port Interface Unit (TPIU).
   */
  constexpr unsigned core_trace_enable = 1 << 24U;

  /// Mask for turning on cycle counter.
  constexpr unsigned enable_cycle_count = 1 << 0;

  // Enable trace core
  core->demcr = (core->demcr | core_trace_enable);

  // Reset cycle count
  dwt->cyccnt = 0;

  // Start cycle count
  dwt->ctrl = (dwt->ctrl | enable_cycle_count);
}

std::uint32_t
uptime()
{
  return dwt->cyccnt;
}

struct my_error_t
{
  std::array<std::uint8_t, 4> data;
};

[[noreturn]] void
terminate() noexcept
{
  while (true) {
    continue;
  }
}

namespace __cxxabiv1 {                                  // NOLINT
std::terminate_handler __terminate_handler = terminate; // NOLINT
}

extern "C"
{
  void _exit([[maybe_unused]] int rc)
  {
    while (true) {
      continue;
    }
  }
  int kill(int, int)
  {
    return -1;
  }
  struct _reent* _impure_ptr = nullptr; // NOLINT
  int getpid()
  {
    return 1;
  }

  std::array<std::uint8_t, 1024> storage;
  std::span<std::uint8_t> storage_left(storage);
  void* __wrap___cxa_allocate_exception(unsigned int p_size) // NOLINT
  {
    // I only know this needs to be 128 because of the disassembly. I cannot
    // figure out why its needed yet, but maybe the answer is in the
    // libunwind-arm.cpp file.
    static constexpr size_t offset = 128;
    if (p_size + offset > storage_left.size()) {
      return nullptr;
    }
    auto* memory = &storage_left[offset];
    storage_left = storage_left.subspan(p_size + offset);
    return memory;
  }
  void __wrap___cxa_free_exception(void*) // NOLINT
  {
    storage_left = std::span<std::uint8_t>(storage);
  }
  void __wrap___cxa_call_unexpected(void*) // NOLINT
  {
    std::terminate();
  }
#define USE_KHALIL_EXCEPTIONS 1

#if USE_KHALIL_EXCEPTIONS == 1
  typedef struct __EIT_entry
  {
    std::uint32_t fnoffset;
    std::uint32_t content;
  } __EIT_entry;

  std::uint32_t selfrel_offset31(const std::uint32_t* p)
  {
    std::uint32_t offset;

    offset = *p;
    /* Sign extend to 32 bits.  */
    if (offset & (1 << 30))
      offset |= 1u << 31;
    else
      offset &= ~(1u << 31);

    return offset + (std::uint32_t)p;
  }

  extern std::uint32_t __trivial_handle_start;
  extern std::uint32_t __trivial_handle_end;
  extern std::uint32_t __text_start;
  extern std::uint32_t __text_end;

  [[gnu::always_inline]] inline bool is_trivial_function(
    std::uint32_t return_address)
  {
    // NOLINTNEXTLINE
    std::uint32_t* check = reinterpret_cast<std::uint32_t*>(return_address);
    return &__trivial_handle_start <= check && check <= &__trivial_handle_end;
  }

  struct eit_entry_less_than
  {
    [[gnu::always_inline]] static std::uint32_t to_prel31_offset(
      const __EIT_entry& entry,
      std::uint32_t address)
    {
      std::uint32_t entry_addr = reinterpret_cast<std::uint32_t>(&entry);
      std::uint32_t final_address = (address - entry_addr) & ~(1 << 31);
      return final_address;
    }

    bool operator()(const __EIT_entry& left, const __EIT_entry& right)
    {
      return left.fnoffset < right.fnoffset;
    }
    bool operator()(const __EIT_entry& left, std::uint32_t right)
    {
      std::uint32_t final_address = to_prel31_offset(left, right);
      return left.fnoffset < final_address;
    }
    bool operator()(std::uint32_t left, const __EIT_entry& right)
    {
      std::uint32_t final_address = to_prel31_offset(right, left);
      return final_address < right.fnoffset;
    }
  };

  // NOLINTNEXTLINE
  const __EIT_entry* search_EIT_table(const __EIT_entry* table,
                                      int nrec, // NOLINT
                                      std::uint32_t return_address)
  {
    if (nrec == 0) {
      return nullptr;
    }

    if (is_trivial_function(return_address)) {
      return &table[nrec - 2];
    }

#define SEARCH_ALGORITHM 2
#if SEARCH_ALGORITHM == 0
    int left = 0;
    int right = nrec - 1;
    while (true) {
      int n = (left + right) / 2;
      std::uint32_t next_fn = std::numeric_limits<std::uint32_t>::max();
      std::uint32_t this_fn = selfrel_offset31(&table[n].fnoffset);

      if (n != nrec - 1) {
        next_fn = selfrel_offset31(&table[n + 1].fnoffset) - 1;
      }

      if (return_address < this_fn) {
        if (n == left) {
          return nullptr;
        }
        right = n - 1;
      } else if (return_address <= next_fn) {
        return &table[n];
      } else {
        left = n + 1;
      }
    }
#elif SEARCH_ALGORITHM == 1
    int left = 0;
    int right = nrec - 1;

    while (true) {
      int n = (left + right) / 2;
      std::uint32_t next_fn = std::numeric_limits<std::uint32_t>::max();
      std::uint32_t this_fn = table[n].fnoffset;

      if (n != nrec - 1) {
        next_fn = table[n + 1].fnoffset - 1;
      }

      std::uint32_t prel31 =
        return_address - reinterpret_cast<std::uint32_t>(&table[n]);
      // clear MSB to conform to the prel31 fnoffset format
      prel31 = prel31 & ~(1 << 31);

      if (prel31 < this_fn) {
        if (n == left) {
          return nullptr;
        }
        right = n - 1;
      } else if (prel31 <= next_fn) {
        return &table[n];
      } else {
        left = n + 1;
      }
    }
#elif SEARCH_ALGORITHM == 2
    std::span<const __EIT_entry> table_span(table, nrec);
    const auto& entry = std::upper_bound(table_span.begin(),
                                         table_span.end(),
                                         return_address,
                                         eit_entry_less_than{});
    return &(*(entry - 1));
#endif
  }

/* Misc constants.  */
#define R_IP 12
#define R_SP 13
#define R_LR 14
#define R_PC 15

  struct core_regs
  {
    std::uint32_t r[16];
  };

  /* We use normal integer types here to avoid the compiler generating
     coprocessor instructions.  */
  struct vfp_regs
  {
    std::uint64_t d[16];
    std::uint32_t pad;
  };

  struct vfpv3_regs
  {
    /* Always populated via VSTM, so no need for the "pad" field from
       vfp_regs (which is used to store the format word for FSTMX).  */
    std::uint64_t d[16];
  };

  struct wmmxd_regs
  {
    std::uint64_t wd[16];
  };

  struct wmmxc_regs
  {
    std::uint32_t wc[4];
  };

  /* The ABI specifies that the unwind routines may only use core registers,
     except when actually manipulating coprocessor state.  This allows
     us to write one implementation that works on all platforms by
     demand-saving coprocessor registers.

     During unwinding we hold the coprocessor state in the actual hardware
     registers and allocate demand-save areas for use during phase1
     unwinding.  */

  struct phase1_vrs
  {
    /* The first fields must be the same as a phase2_vrs.  */
    std::uint32_t demand_save_flags;
    struct core_regs core;
    std::uint32_t prev_sp; /* Only valid during forced unwinding.  */
    struct vfp_regs vfp;
    struct vfpv3_regs vfp_regs_16_to_31;
    struct wmmxd_regs wmmxd;
    struct wmmxc_regs wmmxc;
  };

  /* This must match the structure created by the assembly wrappers.  */
  struct phase2_vrs
  {
    std::uint32_t demand_save_flags;
    struct core_regs core;
  };

  static volatile int vfp_show_up = 0;

  [[gnu::always_inline]] _Unwind_VRS_Result _Unwind_VRS_Pop(
    _Unwind_Context* context,
    _Unwind_VRS_RegClass regclass,
    _uw discriminator,
    _Unwind_VRS_DataRepresentation representation)
  {
    auto* vrs = reinterpret_cast<phase1_vrs*>(context);

    switch (regclass) {
      case _UVRSC_CORE: {
        std::uint32_t mask = discriminator & 0xffff;
        // The mask may not demand that the stack pointer be popped, but the
        // stack pointer will still need to be popped anyway, so this check
        // determines if the mask handles this or not.
        bool set_stack_pointer_afterwards = (mask & R_SP) == 0x0;

        std::uint32_t* ptr = // NOTLINTNEXTLINE
          reinterpret_cast<std::uint32_t*>(vrs->core.r[R_SP]);
        /* Pop the requested registers.  */
        while (mask) {
          auto reg_to_restore = std::countr_zero(mask);
          mask &= ~(1 << reg_to_restore);
          vrs->core.r[reg_to_restore] = *(ptr++);
        }
        if (set_stack_pointer_afterwards) {
          vrs->core.r[R_SP] = reinterpret_cast<std::uint32_t>(ptr);
        }
      }
        return _UVRSR_OK;
      case _UVRSC_VFP:
        return _UVRSR_OK;
      case _UVRSC_WMMXD:
        return _UVRSR_OK;
      case _UVRSC_WMMXC:
        return _UVRSR_OK;
      default:
        return _UVRSR_FAILED;
    }
  }

  /* Return the next byte of unwinding information, or CODE_FINISH if there is
     no data remaining.  */
  [[gnu::always_inline]] _uw8 next_unwind_byte(__gnu_unwind_state* uws)
  {
    _uw8 b;

    if (uws->bytes_left == 0) {
      /* Load another word */
      if (uws->words_left == 0)
        return 0xB0; /* Nothing left.  */
      uws->words_left--;
      uws->data = *(uws->next++);
      uws->bytes_left = 3;
    } else
      uws->bytes_left--;

    /* Extract the most significant byte.  */
    b = (uws->data >> 24) & 0xff;
    uws->data <<= 8;
    return b;
  }

  [[gnu::always_inline]] _Unwind_VRS_Result _My_Unwind_VRS_Get(
    _Unwind_Context* context,
    _Unwind_VRS_RegClass regclass,
    _uw regno,
    _Unwind_VRS_DataRepresentation representation,
    void* valuep)
  {
    auto* vrs = reinterpret_cast<phase1_vrs*>(context);
    *(_uw*)valuep = vrs->core.r[regno];
    return _UVRSR_OK;
  }

  /* ABI defined function to load a virtual register from memory.  */

  [[gnu::always_inline]] _Unwind_VRS_Result _My_Unwind_VRS_Set(
    _Unwind_Context* context,
    _Unwind_VRS_RegClass regclass,
    _uw regno,
    _Unwind_VRS_DataRepresentation representation,
    void* valuep)
  {
    auto* vrs = reinterpret_cast<phase1_vrs*>(context);
    vrs->core.r[regno] = *(_uw*)valuep;
    return _UVRSR_OK;
  }

  /* Execute the unwinding instructions described by UWS.  */
  _Unwind_Reason_Code __gnu_unwind_execute(_Unwind_Context* context,
                                           __gnu_unwind_state* uws)
  {
    _uw op;
    int set_pc;
    _uw reg;

    set_pc = 0;
    for (;;) {
      op = next_unwind_byte(uws);
      if (op == 0xb0) {
        /* If we haven't already set pc then copy it from lr.  */
        if (!set_pc) {
          _My_Unwind_VRS_Get(context, _UVRSC_CORE, R_LR, _UVRSD_UINT32, &reg);
          _My_Unwind_VRS_Set(context, _UVRSC_CORE, R_PC, _UVRSD_UINT32, &reg);
          set_pc = 1;
        }
        /* Drop out of the loop.  */
        break;
      }
      if ((op & 0x80) == 0) {
        /* vsp = vsp +- (imm6 << 2 + 4).  */
        _uw offset;

        offset = ((op & 0x3f) << 2) + 4;
        _My_Unwind_VRS_Get(context, _UVRSC_CORE, R_SP, _UVRSD_UINT32, &reg);
        if (op & 0x40)
          reg -= offset;
        else
          reg += offset;
        _My_Unwind_VRS_Set(context, _UVRSC_CORE, R_SP, _UVRSD_UINT32, &reg);
        continue;
      }

      if ((op & 0xf0) == 0x80) {
        op = (op << 8) | next_unwind_byte(uws);
        if (op == 0x8000) {
          /* Refuse to unwind.  */
          return _URC_FAILURE;
        }
        /* Pop r4-r15 under mask.  */
        op = (op << 4) & 0xfff0;
        if (_Unwind_VRS_Pop(context, _UVRSC_CORE, op, _UVRSD_UINT32) !=
            _UVRSR_OK)
          return _URC_FAILURE;
        if (op & (1 << R_PC))
          set_pc = 1;
        continue;
      }
      if ((op & 0xf0) == 0x90) {
        op &= 0xf;
        if (op == 13 || op == 15)
          /* Reserved.  */
          return _URC_FAILURE;
        /* vsp = r[nnnn].  */
        _My_Unwind_VRS_Get(context, _UVRSC_CORE, op, _UVRSD_UINT32, &reg);
        _My_Unwind_VRS_Set(context, _UVRSC_CORE, R_SP, _UVRSD_UINT32, &reg);
        continue;
      }
      if ((op & 0xf0) == 0xa0) {
        /* Pop r4-r[4+nnn], [lr].  */
        _uw mask;

        mask = (0xff0 >> (7 - (op & 7))) & 0xff0;
        if (op & 8)
          mask |= (1 << R_LR);
        if (_Unwind_VRS_Pop(context, _UVRSC_CORE, mask, _UVRSD_UINT32) !=
            _UVRSR_OK)
          return _URC_FAILURE;
        continue;
      }
      if ((op & 0xf0) == 0xb0) {
        /* op == 0xb0 already handled.  */
        if (op == 0xb1) {
          op = next_unwind_byte(uws);
          if (op == 0 || ((op & 0xf0) != 0))
            /* Spare.  */
            return _URC_FAILURE;
          /* Pop r0-r4 under mask.  */
          if (_Unwind_VRS_Pop(context, _UVRSC_CORE, op, _UVRSD_UINT32) !=
              _UVRSR_OK)
            return _URC_FAILURE;
          continue;
        }
        if (op == 0xb2) {
          /* vsp = vsp + 0x204 + (uleb128 << 2).  */
          int shift;

          _My_Unwind_VRS_Get(context, _UVRSC_CORE, R_SP, _UVRSD_UINT32, &reg);
          op = next_unwind_byte(uws);
          shift = 2;
          while (op & 0x80) {
            reg += ((op & 0x7f) << shift);
            shift += 7;
            op = next_unwind_byte(uws);
          }
          reg += ((op & 0x7f) << shift) + 0x204;
          _My_Unwind_VRS_Set(context, _UVRSC_CORE, R_SP, _UVRSD_UINT32, &reg);
          continue;
        }
        if (op == 0xb3) {
          /* Pop VFP registers with fldmx.  */
          op = next_unwind_byte(uws);
          op = ((op & 0xf0) << 12) | ((op & 0xf) + 1);
          if (_Unwind_VRS_Pop(context, _UVRSC_VFP, op, _UVRSD_VFPX) !=
              _UVRSR_OK)
            return _URC_FAILURE;
          continue;
        }
        if ((op & 0xfc) == 0xb4) /* Obsolete FPA.  */
          return _URC_FAILURE;

        /* op & 0xf8 == 0xb8.  */
        /* Pop VFP D[8]-D[8+nnn] with fldmx.  */
        op = 0x80000 | ((op & 7) + 1);
        if (_Unwind_VRS_Pop(context, _UVRSC_VFP, op, _UVRSD_VFPX) != _UVRSR_OK)
          return _URC_FAILURE;
        continue;
      }
      if ((op & 0xf0) == 0xc0) {
        if (op == 0xc6) {
          /* Pop iWMMXt D registers.  */
          op = next_unwind_byte(uws);
          op = ((op & 0xf0) << 12) | ((op & 0xf) + 1);
          if (_Unwind_VRS_Pop(context, _UVRSC_WMMXD, op, _UVRSD_UINT64) !=
              _UVRSR_OK)
            return _URC_FAILURE;
          continue;
        }
        if (op == 0xc7) {
          op = next_unwind_byte(uws);
          if (op == 0 || (op & 0xf0) != 0)
            /* Spare.  */
            return _URC_FAILURE;
          /* Pop iWMMXt wCGR{3,2,1,0} under mask.  */
          if (_Unwind_VRS_Pop(context, _UVRSC_WMMXC, op, _UVRSD_UINT32) !=
              _UVRSR_OK)
            return _URC_FAILURE;
          continue;
        }
        if ((op & 0xf8) == 0xc0) {
          /* Pop iWMMXt wR[10]-wR[10+nnn].  */
          op = 0xa0000 | ((op & 0xf) + 1);
          if (_Unwind_VRS_Pop(context, _UVRSC_WMMXD, op, _UVRSD_UINT64) !=
              _UVRSR_OK)
            return _URC_FAILURE;
          continue;
        }
        if (op == 0xc8) {
          /* Pop VFPv3 registers D[16+ssss]-D[16+ssss+cccc] with vldm.  */
          op = next_unwind_byte(uws);
          op = (((op & 0xf0) + 16) << 12) | ((op & 0xf) + 1);
          if (_Unwind_VRS_Pop(context, _UVRSC_VFP, op, _UVRSD_DOUBLE) !=
              _UVRSR_OK)
            return _URC_FAILURE;
          continue;
        }
        if (op == 0xc9) {
          /* Pop VFP registers with fldmd.  */
          op = next_unwind_byte(uws);
          op = ((op & 0xf0) << 12) | ((op & 0xf) + 1);
          if (_Unwind_VRS_Pop(context, _UVRSC_VFP, op, _UVRSD_DOUBLE) !=
              _UVRSR_OK)
            return _URC_FAILURE;
          continue;
        }
        /* Spare.  */
        return _URC_FAILURE;
      }
      if ((op & 0xf8) == 0xd0) {
        /* Pop VFP D[8]-D[8+nnn] with fldmd.  */
        op = 0x80000 | ((op & 7) + 1);
        if (_Unwind_VRS_Pop(context, _UVRSC_VFP, op, _UVRSD_DOUBLE) !=
            _UVRSR_OK)
          return _URC_FAILURE;
        continue;
      }
      /* Spare.  */
      return _URC_FAILURE;
    }
    return _URC_OK;
  }
#endif
} // extern "C"

int
start();

int
main()
{
  dwt_counter_enable();
  // Set flash accelerator to 1 CPU cycle per instruction
  *reinterpret_cast<std::uint32_t*>(0x400F'C000) = 0xA;
  volatile int return_code = 0;
  try {
    return_code = start();
  } catch (...) {
    return_code = -1;
  }
  std::terminate();
  return return_code;
}

std::array<std::uint64_t, 25> cycle_map{};
std::array<std::uint64_t, 25> happy_cycle_map{};

int
funct_group0_0();
int
funct_group1_0();
int
funct_group2_0();
int
funct_group3_0();
int
funct_group4_0();
int
funct_group5_0();
int
funct_group6_0();
int
funct_group7_0();
int
funct_group8_0();
int
funct_group9_0();
int
funct_group10_0();
int
funct_group11_0();
int
funct_group12_0();
int
funct_group13_0();
int
funct_group14_0();
int
funct_group15_0();
int
funct_group16_0();
int
funct_group17_0();
int
funct_group18_0();
int
funct_group19_0();
int
funct_group20_0();
int
funct_group21_0();
int
funct_group22_0();
int
funct_group23_0();
int
funct_group24_0();

using signature = int(void);

#if 0
std::array<signature*, 25> functions = {
  funct_group0_0,  funct_group1_0,  funct_group2_0,  funct_group3_0,
  funct_group4_0,  funct_group5_0,  funct_group6_0,  funct_group7_0,
  funct_group8_0,  funct_group9_0,  funct_group10_0, funct_group11_0,
  funct_group12_0, funct_group13_0, funct_group14_0, funct_group15_0,
  funct_group16_0, funct_group17_0, funct_group18_0, funct_group19_0,
  funct_group20_0, funct_group21_0, funct_group22_0, funct_group23_0,
  funct_group24_0
};
#else
std::array<signature*, 25> functions = {
  funct_group0_0,  funct_group1_0,  funct_group2_0,  funct_group3_0,
  funct_group0_0,  funct_group5_0,  funct_group6_0,  funct_group7_0,
  funct_group8_0,  funct_group0_0,  funct_group10_0, funct_group11_0,
  funct_group12_0, funct_group13_0, funct_group0_0,  funct_group15_0,
  funct_group16_0, funct_group17_0, funct_group18_0, funct_group0_0,
  funct_group20_0, funct_group21_0, funct_group22_0, funct_group23_0,
  funct_group0_0
};
#endif

int
start()
{
  cycle_map.fill(0);
  happy_cycle_map.fill(std::numeric_limits<std::int32_t>::max());
  std::uint32_t index = 0;
  for (auto& funct : functions) {
    try {
      start_cycles = uptime();
      funct();
    } catch ([[maybe_unused]] const my_error_t& p_error) {
      end_cycles = uptime();
      cycle_map[index++] = end_cycles - start_cycles;
    }
  }
  index = 0;
  for (auto& funct : functions) {
    bool was_caught = false;
    try {
      // should prevent throw from executing
      side_effect = -1'000'000'000;
      start_cycles = uptime();
      funct();
    } catch ([[maybe_unused]] const my_error_t& p_error) {
      was_caught = true;
    }
    if (!was_caught) {
      end_cycles = uptime();
      happy_cycle_map[index++] = end_cycles - start_cycles;
    }
  }
  return side_effect;
}

class class_0
{
public:
  class_0(std::int32_t p_channel)
    : m_channel(p_channel)
  {
    if (m_channel >= 1'000'000'000) {
      throw my_error_t{ .data = { 0x55, 0xAA, 0x33, 0x44 } };
    }
    side_effect = side_effect + 1;
  }

  class_0(class_0&) = delete;
  class_0& operator=(class_0&) = delete;
  class_0(class_0&&) noexcept = default;
  class_0& operator=(class_0&&) noexcept = default;
  ~class_0() = default;
  void trigger()
  {
    if (m_channel >= 1'000'000'000) {
      throw my_error_t{ .data = { 0xAA, 0xBB, 0x33, 0x44 } };
    }
    side_effect = side_effect + 1;
  }

private:
  std::int32_t m_channel = 0;
};

class class_1
{
public:
  class_1(std::int32_t p_channel)
    : m_channel(p_channel)
  {
    if (m_channel >= 1'000'000'000) {
      throw my_error_t{ .data = { 0x55, 0xAA, 0x33, 0x44 } };
    }
    side_effect = side_effect + 1;
  }

  class_1(class_1&) = delete;
  class_1& operator=(class_1&) = delete;
  class_1(class_1&&) noexcept = default;
  class_1& operator=(class_1&&) noexcept = default;
  ~class_1() { side_effect = side_effect & ~(1 << m_channel); }

  void trigger()
  {
    if (m_channel >= 1'000'000'000) {
      throw my_error_t{ .data = { 0xAA, 0xBB, 0x33, 0x44 } };
    }
    side_effect = side_effect + 1;
  }

private:
  std::int32_t m_channel = 0;
};

int
funct_group0_1();

[[gnu::section(".trivial_handle")]] int
funct_group0_0()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group0_1();
  return side_effect;
}

int
funct_group0_2();

[[gnu::section(".trivial_handle")]] int
funct_group0_1()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group0_2();
  return side_effect;
}

int
funct_group0_3();

[[gnu::section(".trivial_handle")]] int
funct_group0_2()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group0_3();
  return side_effect;
}

int
funct_group0_4();

[[gnu::section(".trivial_handle")]] int
funct_group0_3()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group0_4();
  return side_effect;
}

int
funct_group0_5();

[[gnu::section(".trivial_handle")]] int
funct_group0_4()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  volatile static float my_float = 0.0f;
  my_float = my_float + 5.5f;
  instance_0.trigger();
  side_effect = side_effect + funct_group0_5();
  return side_effect;
}

[[gnu::section(".trivial_handle")]] int
funct_group0_5()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();

  if (side_effect > 0) {
    start_cycles = uptime();
    throw my_error_t{ .data = { 0xDE, 0xAD } };
  }

  return side_effect;
}

int
funct_group1_1();

[[gnu::section(".trivial_handle")]] int
funct_group1_0()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group1_1();
  return side_effect;
}

int
funct_group1_2();

[[gnu::section(".trivial_handle")]] int
funct_group1_1()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group1_2();
  return side_effect;
}

int
funct_group1_3();

[[gnu::section(".trivial_handle")]] int
funct_group1_2()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group1_3();
  return side_effect;
}

int
funct_group1_4();

[[gnu::section(".trivial_handle")]] int
funct_group1_3()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group1_4();
  return side_effect;
}

int
funct_group1_5();

[[gnu::section(".trivial_handle")]] int
funct_group1_4()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group1_5();
  return side_effect;
}

int
funct_group1_6();

[[gnu::section(".trivial_handle")]] int
funct_group1_5()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group1_6();
  return side_effect;
}

int
funct_group1_7();

[[gnu::section(".trivial_handle")]] int
funct_group1_6()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group1_7();
  return side_effect;
}

int
funct_group1_8();

[[gnu::section(".trivial_handle")]] int
funct_group1_7()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group1_8();
  return side_effect;
}

int
funct_group1_9();

[[gnu::section(".trivial_handle")]] int
funct_group1_8()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group1_9();
  return side_effect;
}

int
funct_group1_10();

[[gnu::section(".trivial_handle")]] int
funct_group1_9()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group1_10();
  return side_effect;
}

int
funct_group1_11();

[[gnu::section(".trivial_handle")]] int
funct_group1_10()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group1_11();
  return side_effect;
}

[[gnu::section(".trivial_handle")]] int
funct_group1_11()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();

  if (side_effect > 0) {
    start_cycles = uptime();
    throw my_error_t{ .data = { 0xDE, 0xAD } };
  }

  return side_effect;
}

int
funct_group2_1();

[[gnu::section(".trivial_handle")]] int
funct_group2_0()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group2_1();
  return side_effect;
}

int
funct_group2_2();

[[gnu::section(".trivial_handle")]] int
funct_group2_1()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group2_2();
  return side_effect;
}

int
funct_group2_3();

[[gnu::section(".trivial_handle")]] int
funct_group2_2()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group2_3();
  return side_effect;
}

int
funct_group2_4();

[[gnu::section(".trivial_handle")]] int
funct_group2_3()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group2_4();
  return side_effect;
}

int
funct_group2_5();

[[gnu::section(".trivial_handle")]] int
funct_group2_4()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group2_5();
  return side_effect;
}

int
funct_group2_6();

[[gnu::section(".trivial_handle")]] int
funct_group2_5()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group2_6();
  return side_effect;
}

int
funct_group2_7();

[[gnu::section(".trivial_handle")]] int
funct_group2_6()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group2_7();
  return side_effect;
}

int
funct_group2_8();

[[gnu::section(".trivial_handle")]] int
funct_group2_7()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group2_8();
  return side_effect;
}

int
funct_group2_9();

[[gnu::section(".trivial_handle")]] int
funct_group2_8()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group2_9();
  return side_effect;
}

int
funct_group2_10();

[[gnu::section(".trivial_handle")]] int
funct_group2_9()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group2_10();
  return side_effect;
}

int
funct_group2_11();

[[gnu::section(".trivial_handle")]] int
funct_group2_10()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group2_11();
  return side_effect;
}

int
funct_group2_12();

[[gnu::section(".trivial_handle")]] int
funct_group2_11()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group2_12();
  return side_effect;
}

int
funct_group2_13();

[[gnu::section(".trivial_handle")]] int
funct_group2_12()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group2_13();
  return side_effect;
}

int
funct_group2_14();

[[gnu::section(".trivial_handle")]] int
funct_group2_13()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group2_14();
  return side_effect;
}

int
funct_group2_15();

[[gnu::section(".trivial_handle")]] int
funct_group2_14()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group2_15();
  return side_effect;
}

int
funct_group2_16();

[[gnu::section(".trivial_handle")]] int
funct_group2_15()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group2_16();
  return side_effect;
}

int
funct_group2_17();

[[gnu::section(".trivial_handle")]] int
funct_group2_16()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group2_17();
  return side_effect;
}

int
funct_group2_18();

[[gnu::section(".trivial_handle")]] int
funct_group2_17()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group2_18();
  return side_effect;
}

int
funct_group2_19();

[[gnu::section(".trivial_handle")]] int
funct_group2_18()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group2_19();
  return side_effect;
}

int
funct_group2_20();

[[gnu::section(".trivial_handle")]] int
funct_group2_19()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group2_20();
  return side_effect;
}

int
funct_group2_21();

[[gnu::section(".trivial_handle")]] int
funct_group2_20()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group2_21();
  return side_effect;
}

int
funct_group2_22();

[[gnu::section(".trivial_handle")]] int
funct_group2_21()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group2_22();
  return side_effect;
}

int
funct_group2_23();

[[gnu::section(".trivial_handle")]] int
funct_group2_22()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group2_23();
  return side_effect;
}

[[gnu::section(".trivial_handle")]] int
funct_group2_23()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();

  if (side_effect > 0) {
    start_cycles = uptime();
    throw my_error_t{ .data = { 0xDE, 0xAD } };
  }

  return side_effect;
}

int
funct_group3_1();

[[gnu::section(".trivial_handle")]] int
funct_group3_0()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group3_1();
  return side_effect;
}

int
funct_group3_2();

[[gnu::section(".trivial_handle")]] int
funct_group3_1()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group3_2();
  return side_effect;
}

int
funct_group3_3();

[[gnu::section(".trivial_handle")]] int
funct_group3_2()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group3_3();
  return side_effect;
}

int
funct_group3_4();

[[gnu::section(".trivial_handle")]] int
funct_group3_3()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group3_4();
  return side_effect;
}

int
funct_group3_5();

[[gnu::section(".trivial_handle")]] int
funct_group3_4()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group3_5();
  return side_effect;
}

int
funct_group3_6();

[[gnu::section(".trivial_handle")]] int
funct_group3_5()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group3_6();
  return side_effect;
}

int
funct_group3_7();

[[gnu::section(".trivial_handle")]] int
funct_group3_6()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group3_7();
  return side_effect;
}

int
funct_group3_8();

[[gnu::section(".trivial_handle")]] int
funct_group3_7()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group3_8();
  return side_effect;
}

int
funct_group3_9();

[[gnu::section(".trivial_handle")]] int
funct_group3_8()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group3_9();
  return side_effect;
}

int
funct_group3_10();

[[gnu::section(".trivial_handle")]] int
funct_group3_9()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group3_10();
  return side_effect;
}

int
funct_group3_11();

[[gnu::section(".trivial_handle")]] int
funct_group3_10()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group3_11();
  return side_effect;
}

int
funct_group3_12();

[[gnu::section(".trivial_handle")]] int
funct_group3_11()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group3_12();
  return side_effect;
}

int
funct_group3_13();

[[gnu::section(".trivial_handle")]] int
funct_group3_12()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group3_13();
  return side_effect;
}

int
funct_group3_14();

[[gnu::section(".trivial_handle")]] int
funct_group3_13()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group3_14();
  return side_effect;
}

int
funct_group3_15();

[[gnu::section(".trivial_handle")]] int
funct_group3_14()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group3_15();
  return side_effect;
}

int
funct_group3_16();

[[gnu::section(".trivial_handle")]] int
funct_group3_15()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group3_16();
  return side_effect;
}

int
funct_group3_17();

[[gnu::section(".trivial_handle")]] int
funct_group3_16()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group3_17();
  return side_effect;
}

int
funct_group3_18();

[[gnu::section(".trivial_handle")]] int
funct_group3_17()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group3_18();
  return side_effect;
}

int
funct_group3_19();

[[gnu::section(".trivial_handle")]] int
funct_group3_18()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group3_19();
  return side_effect;
}

int
funct_group3_20();

[[gnu::section(".trivial_handle")]] int
funct_group3_19()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group3_20();
  return side_effect;
}

int
funct_group3_21();

[[gnu::section(".trivial_handle")]] int
funct_group3_20()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group3_21();
  return side_effect;
}

int
funct_group3_22();

[[gnu::section(".trivial_handle")]] int
funct_group3_21()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group3_22();
  return side_effect;
}

int
funct_group3_23();

[[gnu::section(".trivial_handle")]] int
funct_group3_22()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group3_23();
  return side_effect;
}

int
funct_group3_24();

[[gnu::section(".trivial_handle")]] int
funct_group3_23()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group3_24();
  return side_effect;
}

int
funct_group3_25();

[[gnu::section(".trivial_handle")]] int
funct_group3_24()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group3_25();
  return side_effect;
}

int
funct_group3_26();

[[gnu::section(".trivial_handle")]] int
funct_group3_25()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group3_26();
  return side_effect;
}

int
funct_group3_27();

[[gnu::section(".trivial_handle")]] int
funct_group3_26()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group3_27();
  return side_effect;
}

int
funct_group3_28();

[[gnu::section(".trivial_handle")]] int
funct_group3_27()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group3_28();
  return side_effect;
}

int
funct_group3_29();

[[gnu::section(".trivial_handle")]] int
funct_group3_28()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group3_29();
  return side_effect;
}

int
funct_group3_30();

[[gnu::section(".trivial_handle")]] int
funct_group3_29()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group3_30();
  return side_effect;
}

int
funct_group3_31();

[[gnu::section(".trivial_handle")]] int
funct_group3_30()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group3_31();
  return side_effect;
}

int
funct_group3_32();

[[gnu::section(".trivial_handle")]] int
funct_group3_31()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group3_32();
  return side_effect;
}

int
funct_group3_33();

[[gnu::section(".trivial_handle")]] int
funct_group3_32()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group3_33();
  return side_effect;
}

int
funct_group3_34();

[[gnu::section(".trivial_handle")]] int
funct_group3_33()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group3_34();
  return side_effect;
}

int
funct_group3_35();

[[gnu::section(".trivial_handle")]] int
funct_group3_34()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group3_35();
  return side_effect;
}

int
funct_group3_36();

[[gnu::section(".trivial_handle")]] int
funct_group3_35()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group3_36();
  return side_effect;
}

int
funct_group3_37();

[[gnu::section(".trivial_handle")]] int
funct_group3_36()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group3_37();
  return side_effect;
}

int
funct_group3_38();

[[gnu::section(".trivial_handle")]] int
funct_group3_37()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group3_38();
  return side_effect;
}

int
funct_group3_39();

[[gnu::section(".trivial_handle")]] int
funct_group3_38()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group3_39();
  return side_effect;
}

int
funct_group3_40();

[[gnu::section(".trivial_handle")]] int
funct_group3_39()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group3_40();
  return side_effect;
}

int
funct_group3_41();

[[gnu::section(".trivial_handle")]] int
funct_group3_40()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group3_41();
  return side_effect;
}

int
funct_group3_42();

[[gnu::section(".trivial_handle")]] int
funct_group3_41()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group3_42();
  return side_effect;
}

int
funct_group3_43();

[[gnu::section(".trivial_handle")]] int
funct_group3_42()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group3_43();
  return side_effect;
}

int
funct_group3_44();

[[gnu::section(".trivial_handle")]] int
funct_group3_43()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group3_44();
  return side_effect;
}

int
funct_group3_45();

[[gnu::section(".trivial_handle")]] int
funct_group3_44()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group3_45();
  return side_effect;
}

int
funct_group3_46();

[[gnu::section(".trivial_handle")]] int
funct_group3_45()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group3_46();
  return side_effect;
}

int
funct_group3_47();

[[gnu::section(".trivial_handle")]] int
funct_group3_46()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group3_47();
  return side_effect;
}

[[gnu::section(".trivial_handle")]] int
funct_group3_47()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();

  if (side_effect > 0) {
    start_cycles = uptime();
    throw my_error_t{ .data = { 0xDE, 0xAD } };
  }

  return side_effect;
}

int
funct_group5_1();

[[gnu::section(".trivial_handle")]] int
funct_group5_0()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group5_1();
  return side_effect;
}

int
funct_group5_2();

[[gnu::section(".trivial_handle")]] int
funct_group5_1()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group5_2();
  return side_effect;
}

int
funct_group5_3();

[[gnu::section(".trivial_handle")]] int
funct_group5_2()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group5_3();
  return side_effect;
}

int
funct_group5_4();

[[gnu::section(".trivial_handle")]] int
funct_group5_3()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group5_4();
  return side_effect;
}

int
funct_group5_5();

[[gnu::section(".trivial_handle")]] int
funct_group5_4()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group5_5();
  return side_effect;
}

[[gnu::section(".trivial_handle")]] int
funct_group5_5()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();

  if (side_effect > 0) {
    start_cycles = uptime();
    throw my_error_t{ .data = { 0xDE, 0xAD } };
  }

  return side_effect;
}

int
funct_group6_1();

[[gnu::section(".trivial_handle")]] int
funct_group6_0()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group6_1();
  return side_effect;
}

int
funct_group6_2();

[[gnu::section(".trivial_handle")]] int
funct_group6_1()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group6_2();
  return side_effect;
}

int
funct_group6_3();

[[gnu::section(".trivial_handle")]] int
funct_group6_2()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group6_3();
  return side_effect;
}

int
funct_group6_4();

[[gnu::section(".trivial_handle")]] int
funct_group6_3()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group6_4();
  return side_effect;
}

int
funct_group6_5();

[[gnu::section(".trivial_handle")]] int
funct_group6_4()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group6_5();
  return side_effect;
}

int
funct_group6_6();

[[gnu::section(".trivial_handle")]] int
funct_group6_5()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group6_6();
  return side_effect;
}

int
funct_group6_7();

[[gnu::section(".trivial_handle")]] int
funct_group6_6()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group6_7();
  return side_effect;
}

int
funct_group6_8();

[[gnu::section(".trivial_handle")]] int
funct_group6_7()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group6_8();
  return side_effect;
}

int
funct_group6_9();

[[gnu::section(".trivial_handle")]] int
funct_group6_8()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group6_9();
  return side_effect;
}

int
funct_group6_10();

int
funct_group6_9()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group6_10();
  return side_effect;
}

int
funct_group6_11();

[[gnu::section(".trivial_handle")]] int
funct_group6_10()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group6_11();
  return side_effect;
}

[[gnu::section(".trivial_handle")]] int
funct_group6_11()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();

  if (side_effect > 0) {
    start_cycles = uptime();
    throw my_error_t{ .data = { 0xDE, 0xAD } };
  }

  return side_effect;
}

int
funct_group7_1();

[[gnu::section(".trivial_handle")]] int
funct_group7_0()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group7_1();
  return side_effect;
}

int
funct_group7_2();

[[gnu::section(".trivial_handle")]] int
funct_group7_1()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group7_2();
  return side_effect;
}

int
funct_group7_3();

[[gnu::section(".trivial_handle")]] int
funct_group7_2()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group7_3();
  return side_effect;
}

int
funct_group7_4();

[[gnu::section(".trivial_handle")]] int
funct_group7_3()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group7_4();
  return side_effect;
}

int
funct_group7_5();

[[gnu::section(".trivial_handle")]] int
funct_group7_4()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group7_5();
  return side_effect;
}

int
funct_group7_6();

[[gnu::section(".trivial_handle")]] int
funct_group7_5()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group7_6();
  return side_effect;
}

int
funct_group7_7();

[[gnu::section(".trivial_handle")]] int
funct_group7_6()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group7_7();
  return side_effect;
}

int
funct_group7_8();

[[gnu::section(".trivial_handle")]] int
funct_group7_7()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group7_8();
  return side_effect;
}

int
funct_group7_9();

[[gnu::section(".trivial_handle")]] int
funct_group7_8()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group7_9();
  return side_effect;
}

int
funct_group7_10();

int
funct_group7_9()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group7_10();
  return side_effect;
}

int
funct_group7_11();

[[gnu::section(".trivial_handle")]] int
funct_group7_10()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group7_11();
  return side_effect;
}

int
funct_group7_12();

[[gnu::section(".trivial_handle")]] int
funct_group7_11()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group7_12();
  return side_effect;
}

int
funct_group7_13();

[[gnu::section(".trivial_handle")]] int
funct_group7_12()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group7_13();
  return side_effect;
}

int
funct_group7_14();

[[gnu::section(".trivial_handle")]] int
funct_group7_13()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group7_14();
  return side_effect;
}

int
funct_group7_15();

[[gnu::section(".trivial_handle")]] int
funct_group7_14()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group7_15();
  return side_effect;
}

int
funct_group7_16();

[[gnu::section(".trivial_handle")]] int
funct_group7_15()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group7_16();
  return side_effect;
}

int
funct_group7_17();

[[gnu::section(".trivial_handle")]] int
funct_group7_16()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group7_17();
  return side_effect;
}

int
funct_group7_18();

[[gnu::section(".trivial_handle")]] int
funct_group7_17()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group7_18();
  return side_effect;
}

int
funct_group7_19();

[[gnu::section(".trivial_handle")]] int
funct_group7_18()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group7_19();
  return side_effect;
}

int
funct_group7_20();

int
funct_group7_19()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group7_20();
  return side_effect;
}

int
funct_group7_21();

[[gnu::section(".trivial_handle")]] int
funct_group7_20()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group7_21();
  return side_effect;
}

int
funct_group7_22();

[[gnu::section(".trivial_handle")]] int
funct_group7_21()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group7_22();
  return side_effect;
}

int
funct_group7_23();

[[gnu::section(".trivial_handle")]] int
funct_group7_22()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group7_23();
  return side_effect;
}

[[gnu::section(".trivial_handle")]] int
funct_group7_23()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();

  if (side_effect > 0) {
    start_cycles = uptime();
    throw my_error_t{ .data = { 0xDE, 0xAD } };
  }

  return side_effect;
}

int
funct_group8_1();

[[gnu::section(".trivial_handle")]] int
funct_group8_0()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group8_1();
  return side_effect;
}

int
funct_group8_2();

[[gnu::section(".trivial_handle")]] int
funct_group8_1()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group8_2();
  return side_effect;
}

int
funct_group8_3();

[[gnu::section(".trivial_handle")]] int
funct_group8_2()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group8_3();
  return side_effect;
}

int
funct_group8_4();

[[gnu::section(".trivial_handle")]] int
funct_group8_3()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group8_4();
  return side_effect;
}

int
funct_group8_5();

[[gnu::section(".trivial_handle")]] int
funct_group8_4()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group8_5();
  return side_effect;
}

int
funct_group8_6();

[[gnu::section(".trivial_handle")]] int
funct_group8_5()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group8_6();
  return side_effect;
}

int
funct_group8_7();

[[gnu::section(".trivial_handle")]] int
funct_group8_6()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group8_7();
  return side_effect;
}

int
funct_group8_8();

[[gnu::section(".trivial_handle")]] int
funct_group8_7()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group8_8();
  return side_effect;
}

int
funct_group8_9();

[[gnu::section(".trivial_handle")]] int
funct_group8_8()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group8_9();
  return side_effect;
}

int
funct_group8_10();

int
funct_group8_9()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group8_10();
  return side_effect;
}

int
funct_group8_11();

[[gnu::section(".trivial_handle")]] int
funct_group8_10()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group8_11();
  return side_effect;
}

int
funct_group8_12();

[[gnu::section(".trivial_handle")]] int
funct_group8_11()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group8_12();
  return side_effect;
}

int
funct_group8_13();

[[gnu::section(".trivial_handle")]] int
funct_group8_12()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group8_13();
  return side_effect;
}

int
funct_group8_14();

[[gnu::section(".trivial_handle")]] int
funct_group8_13()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group8_14();
  return side_effect;
}

int
funct_group8_15();

[[gnu::section(".trivial_handle")]] int
funct_group8_14()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group8_15();
  return side_effect;
}

int
funct_group8_16();

[[gnu::section(".trivial_handle")]] int
funct_group8_15()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group8_16();
  return side_effect;
}

int
funct_group8_17();

[[gnu::section(".trivial_handle")]] int
funct_group8_16()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group8_17();
  return side_effect;
}

int
funct_group8_18();

[[gnu::section(".trivial_handle")]] int
funct_group8_17()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group8_18();
  return side_effect;
}

int
funct_group8_19();

[[gnu::section(".trivial_handle")]] int
funct_group8_18()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group8_19();
  return side_effect;
}

int
funct_group8_20();

int
funct_group8_19()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group8_20();
  return side_effect;
}

int
funct_group8_21();

[[gnu::section(".trivial_handle")]] int
funct_group8_20()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group8_21();
  return side_effect;
}

int
funct_group8_22();

[[gnu::section(".trivial_handle")]] int
funct_group8_21()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group8_22();
  return side_effect;
}

int
funct_group8_23();

[[gnu::section(".trivial_handle")]] int
funct_group8_22()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group8_23();
  return side_effect;
}

int
funct_group8_24();

[[gnu::section(".trivial_handle")]] int
funct_group8_23()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group8_24();
  return side_effect;
}

int
funct_group8_25();

[[gnu::section(".trivial_handle")]] int
funct_group8_24()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group8_25();
  return side_effect;
}

int
funct_group8_26();

[[gnu::section(".trivial_handle")]] int
funct_group8_25()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group8_26();
  return side_effect;
}

int
funct_group8_27();

[[gnu::section(".trivial_handle")]] int
funct_group8_26()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group8_27();
  return side_effect;
}

int
funct_group8_28();

[[gnu::section(".trivial_handle")]] int
funct_group8_27()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group8_28();
  return side_effect;
}

int
funct_group8_29();

[[gnu::section(".trivial_handle")]] int
funct_group8_28()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group8_29();
  return side_effect;
}

int
funct_group8_30();

int
funct_group8_29()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group8_30();
  return side_effect;
}

int
funct_group8_31();

[[gnu::section(".trivial_handle")]] int
funct_group8_30()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group8_31();
  return side_effect;
}

int
funct_group8_32();

[[gnu::section(".trivial_handle")]] int
funct_group8_31()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group8_32();
  return side_effect;
}

int
funct_group8_33();

[[gnu::section(".trivial_handle")]] int
funct_group8_32()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group8_33();
  return side_effect;
}

int
funct_group8_34();

[[gnu::section(".trivial_handle")]] int
funct_group8_33()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group8_34();
  return side_effect;
}

int
funct_group8_35();

[[gnu::section(".trivial_handle")]] int
funct_group8_34()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group8_35();
  return side_effect;
}

int
funct_group8_36();

[[gnu::section(".trivial_handle")]] int
funct_group8_35()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group8_36();
  return side_effect;
}

int
funct_group8_37();

[[gnu::section(".trivial_handle")]] int
funct_group8_36()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group8_37();
  return side_effect;
}

int
funct_group8_38();

[[gnu::section(".trivial_handle")]] int
funct_group8_37()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group8_38();
  return side_effect;
}

int
funct_group8_39();

[[gnu::section(".trivial_handle")]] int
funct_group8_38()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group8_39();
  return side_effect;
}

int
funct_group8_40();

int
funct_group8_39()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group8_40();
  return side_effect;
}

int
funct_group8_41();

[[gnu::section(".trivial_handle")]] int
funct_group8_40()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group8_41();
  return side_effect;
}

int
funct_group8_42();

[[gnu::section(".trivial_handle")]] int
funct_group8_41()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group8_42();
  return side_effect;
}

int
funct_group8_43();

[[gnu::section(".trivial_handle")]] int
funct_group8_42()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group8_43();
  return side_effect;
}

int
funct_group8_44();

[[gnu::section(".trivial_handle")]] int
funct_group8_43()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group8_44();
  return side_effect;
}

int
funct_group8_45();

[[gnu::section(".trivial_handle")]] int
funct_group8_44()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group8_45();
  return side_effect;
}

int
funct_group8_46();

[[gnu::section(".trivial_handle")]] int
funct_group8_45()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group8_46();
  return side_effect;
}

int
funct_group8_47();

[[gnu::section(".trivial_handle")]] int
funct_group8_46()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group8_47();
  return side_effect;
}

[[gnu::section(".trivial_handle")]] int
funct_group8_47()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();

  if (side_effect > 0) {
    start_cycles = uptime();
    throw my_error_t{ .data = { 0xDE, 0xAD } };
  }

  return side_effect;
}

int
funct_group10_1();

[[gnu::section(".trivial_handle")]] int
funct_group10_0()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group10_1();
  return side_effect;
}

int
funct_group10_2();

[[gnu::section(".trivial_handle")]] int
funct_group10_1()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group10_2();
  return side_effect;
}

int
funct_group10_3();

[[gnu::section(".trivial_handle")]] int
funct_group10_2()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group10_3();
  return side_effect;
}

int
funct_group10_4();

int
funct_group10_3()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group10_4();
  return side_effect;
}

int
funct_group10_5();

[[gnu::section(".trivial_handle")]] int
funct_group10_4()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group10_5();
  return side_effect;
}

[[gnu::section(".trivial_handle")]] int
funct_group10_5()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();

  if (side_effect > 0) {
    start_cycles = uptime();
    throw my_error_t{ .data = { 0xDE, 0xAD } };
  }

  return side_effect;
}

int
funct_group11_1();

[[gnu::section(".trivial_handle")]] int
funct_group11_0()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group11_1();
  return side_effect;
}

int
funct_group11_2();

[[gnu::section(".trivial_handle")]] int
funct_group11_1()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group11_2();
  return side_effect;
}

int
funct_group11_3();

[[gnu::section(".trivial_handle")]] int
funct_group11_2()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group11_3();
  return side_effect;
}

int
funct_group11_4();

int
funct_group11_3()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group11_4();
  return side_effect;
}

int
funct_group11_5();

[[gnu::section(".trivial_handle")]] int
funct_group11_4()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group11_5();
  return side_effect;
}

int
funct_group11_6();

[[gnu::section(".trivial_handle")]] int
funct_group11_5()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group11_6();
  return side_effect;
}

int
funct_group11_7();

[[gnu::section(".trivial_handle")]] int
funct_group11_6()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group11_7();
  return side_effect;
}

int
funct_group11_8();

int
funct_group11_7()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group11_8();
  return side_effect;
}

int
funct_group11_9();

[[gnu::section(".trivial_handle")]] int
funct_group11_8()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group11_9();
  return side_effect;
}

int
funct_group11_10();

[[gnu::section(".trivial_handle")]] int
funct_group11_9()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group11_10();
  return side_effect;
}

int
funct_group11_11();

[[gnu::section(".trivial_handle")]] int
funct_group11_10()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group11_11();
  return side_effect;
}

int
funct_group11_11()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();

  if (side_effect > 0) {
    start_cycles = uptime();
    throw my_error_t{ .data = { 0xDE, 0xAD } };
  }

  return side_effect;
}

int
funct_group12_1();

[[gnu::section(".trivial_handle")]] int
funct_group12_0()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group12_1();
  return side_effect;
}

int
funct_group12_2();

[[gnu::section(".trivial_handle")]] int
funct_group12_1()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group12_2();
  return side_effect;
}

int
funct_group12_3();

[[gnu::section(".trivial_handle")]] int
funct_group12_2()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group12_3();
  return side_effect;
}

int
funct_group12_4();

int
funct_group12_3()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group12_4();
  return side_effect;
}

int
funct_group12_5();

[[gnu::section(".trivial_handle")]] int
funct_group12_4()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group12_5();
  return side_effect;
}

int
funct_group12_6();

[[gnu::section(".trivial_handle")]] int
funct_group12_5()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group12_6();
  return side_effect;
}

int
funct_group12_7();

[[gnu::section(".trivial_handle")]] int
funct_group12_6()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group12_7();
  return side_effect;
}

int
funct_group12_8();

int
funct_group12_7()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group12_8();
  return side_effect;
}

int
funct_group12_9();

[[gnu::section(".trivial_handle")]] int
funct_group12_8()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group12_9();
  return side_effect;
}

int
funct_group12_10();

[[gnu::section(".trivial_handle")]] int
funct_group12_9()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group12_10();
  return side_effect;
}

int
funct_group12_11();

[[gnu::section(".trivial_handle")]] int
funct_group12_10()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group12_11();
  return side_effect;
}

int
funct_group12_12();

int
funct_group12_11()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group12_12();
  return side_effect;
}

int
funct_group12_13();

[[gnu::section(".trivial_handle")]] int
funct_group12_12()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group12_13();
  return side_effect;
}

int
funct_group12_14();

[[gnu::section(".trivial_handle")]] int
funct_group12_13()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group12_14();
  return side_effect;
}

int
funct_group12_15();

[[gnu::section(".trivial_handle")]] int
funct_group12_14()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group12_15();
  return side_effect;
}

int
funct_group12_16();

int
funct_group12_15()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group12_16();
  return side_effect;
}

int
funct_group12_17();

[[gnu::section(".trivial_handle")]] int
funct_group12_16()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group12_17();
  return side_effect;
}

int
funct_group12_18();

[[gnu::section(".trivial_handle")]] int
funct_group12_17()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group12_18();
  return side_effect;
}

int
funct_group12_19();

[[gnu::section(".trivial_handle")]] int
funct_group12_18()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group12_19();
  return side_effect;
}

int
funct_group12_20();

int
funct_group12_19()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group12_20();
  return side_effect;
}

int
funct_group12_21();

[[gnu::section(".trivial_handle")]] int
funct_group12_20()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group12_21();
  return side_effect;
}

int
funct_group12_22();

[[gnu::section(".trivial_handle")]] int
funct_group12_21()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group12_22();
  return side_effect;
}

int
funct_group12_23();

[[gnu::section(".trivial_handle")]] int
funct_group12_22()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group12_23();
  return side_effect;
}

int
funct_group12_23()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();

  if (side_effect > 0) {
    start_cycles = uptime();
    throw my_error_t{ .data = { 0xDE, 0xAD } };
  }

  return side_effect;
}

int
funct_group13_1();

[[gnu::section(".trivial_handle")]] int
funct_group13_0()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group13_1();
  return side_effect;
}

int
funct_group13_2();

[[gnu::section(".trivial_handle")]] int
funct_group13_1()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group13_2();
  return side_effect;
}

int
funct_group13_3();

[[gnu::section(".trivial_handle")]] int
funct_group13_2()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group13_3();
  return side_effect;
}

int
funct_group13_4();

int
funct_group13_3()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group13_4();
  return side_effect;
}

int
funct_group13_5();

[[gnu::section(".trivial_handle")]] int
funct_group13_4()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group13_5();
  return side_effect;
}

int
funct_group13_6();

[[gnu::section(".trivial_handle")]] int
funct_group13_5()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group13_6();
  return side_effect;
}

int
funct_group13_7();

[[gnu::section(".trivial_handle")]] int
funct_group13_6()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group13_7();
  return side_effect;
}

int
funct_group13_8();

int
funct_group13_7()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group13_8();
  return side_effect;
}

int
funct_group13_9();

[[gnu::section(".trivial_handle")]] int
funct_group13_8()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group13_9();
  return side_effect;
}

int
funct_group13_10();

[[gnu::section(".trivial_handle")]] int
funct_group13_9()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group13_10();
  return side_effect;
}

int
funct_group13_11();

[[gnu::section(".trivial_handle")]] int
funct_group13_10()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group13_11();
  return side_effect;
}

int
funct_group13_12();

int
funct_group13_11()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group13_12();
  return side_effect;
}

int
funct_group13_13();

[[gnu::section(".trivial_handle")]] int
funct_group13_12()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group13_13();
  return side_effect;
}

int
funct_group13_14();

[[gnu::section(".trivial_handle")]] int
funct_group13_13()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group13_14();
  return side_effect;
}

int
funct_group13_15();

[[gnu::section(".trivial_handle")]] int
funct_group13_14()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group13_15();
  return side_effect;
}

int
funct_group13_16();

int
funct_group13_15()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group13_16();
  return side_effect;
}

int
funct_group13_17();

[[gnu::section(".trivial_handle")]] int
funct_group13_16()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group13_17();
  return side_effect;
}

int
funct_group13_18();

[[gnu::section(".trivial_handle")]] int
funct_group13_17()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group13_18();
  return side_effect;
}

int
funct_group13_19();

[[gnu::section(".trivial_handle")]] int
funct_group13_18()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group13_19();
  return side_effect;
}

int
funct_group13_20();

int
funct_group13_19()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group13_20();
  return side_effect;
}

int
funct_group13_21();

[[gnu::section(".trivial_handle")]] int
funct_group13_20()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group13_21();
  return side_effect;
}

int
funct_group13_22();

[[gnu::section(".trivial_handle")]] int
funct_group13_21()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group13_22();
  return side_effect;
}

int
funct_group13_23();

[[gnu::section(".trivial_handle")]] int
funct_group13_22()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group13_23();
  return side_effect;
}

int
funct_group13_24();

int
funct_group13_23()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group13_24();
  return side_effect;
}

int
funct_group13_25();

[[gnu::section(".trivial_handle")]] int
funct_group13_24()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group13_25();
  return side_effect;
}

int
funct_group13_26();

[[gnu::section(".trivial_handle")]] int
funct_group13_25()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group13_26();
  return side_effect;
}

int
funct_group13_27();

[[gnu::section(".trivial_handle")]] int
funct_group13_26()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group13_27();
  return side_effect;
}

int
funct_group13_28();

int
funct_group13_27()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group13_28();
  return side_effect;
}

int
funct_group13_29();

[[gnu::section(".trivial_handle")]] int
funct_group13_28()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group13_29();
  return side_effect;
}

int
funct_group13_30();

[[gnu::section(".trivial_handle")]] int
funct_group13_29()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group13_30();
  return side_effect;
}

int
funct_group13_31();

[[gnu::section(".trivial_handle")]] int
funct_group13_30()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group13_31();
  return side_effect;
}

int
funct_group13_32();

int
funct_group13_31()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group13_32();
  return side_effect;
}

int
funct_group13_33();

[[gnu::section(".trivial_handle")]] int
funct_group13_32()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group13_33();
  return side_effect;
}

int
funct_group13_34();

[[gnu::section(".trivial_handle")]] int
funct_group13_33()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group13_34();
  return side_effect;
}

int
funct_group13_35();

[[gnu::section(".trivial_handle")]] int
funct_group13_34()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group13_35();
  return side_effect;
}

int
funct_group13_36();

int
funct_group13_35()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group13_36();
  return side_effect;
}

int
funct_group13_37();

[[gnu::section(".trivial_handle")]] int
funct_group13_36()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group13_37();
  return side_effect;
}

int
funct_group13_38();

[[gnu::section(".trivial_handle")]] int
funct_group13_37()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group13_38();
  return side_effect;
}

int
funct_group13_39();

[[gnu::section(".trivial_handle")]] int
funct_group13_38()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group13_39();
  return side_effect;
}

int
funct_group13_40();

int
funct_group13_39()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group13_40();
  return side_effect;
}

int
funct_group13_41();

[[gnu::section(".trivial_handle")]] int
funct_group13_40()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group13_41();
  return side_effect;
}

int
funct_group13_42();

[[gnu::section(".trivial_handle")]] int
funct_group13_41()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group13_42();
  return side_effect;
}

int
funct_group13_43();

[[gnu::section(".trivial_handle")]] int
funct_group13_42()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group13_43();
  return side_effect;
}

int
funct_group13_44();

int
funct_group13_43()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group13_44();
  return side_effect;
}

int
funct_group13_45();

[[gnu::section(".trivial_handle")]] int
funct_group13_44()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group13_45();
  return side_effect;
}

int
funct_group13_46();

[[gnu::section(".trivial_handle")]] int
funct_group13_45()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group13_46();
  return side_effect;
}

int
funct_group13_47();

[[gnu::section(".trivial_handle")]] int
funct_group13_46()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group13_47();
  return side_effect;
}

int
funct_group13_47()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();

  if (side_effect > 0) {
    start_cycles = uptime();
    throw my_error_t{ .data = { 0xDE, 0xAD } };
  }

  return side_effect;
}

int
funct_group15_1();

[[gnu::section(".trivial_handle")]] int
funct_group15_0()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group15_1();
  return side_effect;
}

int
funct_group15_2();

int
funct_group15_1()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group15_2();
  return side_effect;
}

int
funct_group15_3();

[[gnu::section(".trivial_handle")]] int
funct_group15_2()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group15_3();
  return side_effect;
}

int
funct_group15_4();

int
funct_group15_3()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group15_4();
  return side_effect;
}

int
funct_group15_5();

[[gnu::section(".trivial_handle")]] int
funct_group15_4()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group15_5();
  return side_effect;
}

int
funct_group15_5()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();

  if (side_effect > 0) {
    start_cycles = uptime();
    throw my_error_t{ .data = { 0xDE, 0xAD } };
  }

  return side_effect;
}

int
funct_group16_1();

[[gnu::section(".trivial_handle")]] int
funct_group16_0()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group16_1();
  return side_effect;
}

int
funct_group16_2();

int
funct_group16_1()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group16_2();
  return side_effect;
}

int
funct_group16_3();

[[gnu::section(".trivial_handle")]] int
funct_group16_2()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group16_3();
  return side_effect;
}

int
funct_group16_4();

int
funct_group16_3()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group16_4();
  return side_effect;
}

int
funct_group16_5();

[[gnu::section(".trivial_handle")]] int
funct_group16_4()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group16_5();
  return side_effect;
}

int
funct_group16_6();

int
funct_group16_5()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group16_6();
  return side_effect;
}

int
funct_group16_7();

[[gnu::section(".trivial_handle")]] int
funct_group16_6()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group16_7();
  return side_effect;
}

int
funct_group16_8();

int
funct_group16_7()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group16_8();
  return side_effect;
}

int
funct_group16_9();

[[gnu::section(".trivial_handle")]] int
funct_group16_8()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group16_9();
  return side_effect;
}

int
funct_group16_10();

int
funct_group16_9()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group16_10();
  return side_effect;
}

int
funct_group16_11();

[[gnu::section(".trivial_handle")]] int
funct_group16_10()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group16_11();
  return side_effect;
}

int
funct_group16_11()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();

  if (side_effect > 0) {
    start_cycles = uptime();
    throw my_error_t{ .data = { 0xDE, 0xAD } };
  }

  return side_effect;
}

int
funct_group17_1();

[[gnu::section(".trivial_handle")]] int
funct_group17_0()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group17_1();
  return side_effect;
}

int
funct_group17_2();

int
funct_group17_1()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group17_2();
  return side_effect;
}

int
funct_group17_3();

[[gnu::section(".trivial_handle")]] int
funct_group17_2()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group17_3();
  return side_effect;
}

int
funct_group17_4();

int
funct_group17_3()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group17_4();
  return side_effect;
}

int
funct_group17_5();

[[gnu::section(".trivial_handle")]] int
funct_group17_4()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group17_5();
  return side_effect;
}

int
funct_group17_6();

int
funct_group17_5()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group17_6();
  return side_effect;
}

int
funct_group17_7();

[[gnu::section(".trivial_handle")]] int
funct_group17_6()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group17_7();
  return side_effect;
}

int
funct_group17_8();

int
funct_group17_7()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group17_8();
  return side_effect;
}

int
funct_group17_9();

[[gnu::section(".trivial_handle")]] int
funct_group17_8()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group17_9();
  return side_effect;
}

int
funct_group17_10();

int
funct_group17_9()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group17_10();
  return side_effect;
}

int
funct_group17_11();

[[gnu::section(".trivial_handle")]] int
funct_group17_10()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group17_11();
  return side_effect;
}

int
funct_group17_12();

int
funct_group17_11()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group17_12();
  return side_effect;
}

int
funct_group17_13();

[[gnu::section(".trivial_handle")]] int
funct_group17_12()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group17_13();
  return side_effect;
}

int
funct_group17_14();

int
funct_group17_13()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group17_14();
  return side_effect;
}

int
funct_group17_15();

[[gnu::section(".trivial_handle")]] int
funct_group17_14()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group17_15();
  return side_effect;
}

int
funct_group17_16();

int
funct_group17_15()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group17_16();
  return side_effect;
}

int
funct_group17_17();

[[gnu::section(".trivial_handle")]] int
funct_group17_16()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group17_17();
  return side_effect;
}

int
funct_group17_18();

int
funct_group17_17()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group17_18();
  return side_effect;
}

int
funct_group17_19();

[[gnu::section(".trivial_handle")]] int
funct_group17_18()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group17_19();
  return side_effect;
}

int
funct_group17_20();

int
funct_group17_19()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group17_20();
  return side_effect;
}

int
funct_group17_21();

[[gnu::section(".trivial_handle")]] int
funct_group17_20()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group17_21();
  return side_effect;
}

int
funct_group17_22();

int
funct_group17_21()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group17_22();
  return side_effect;
}

int
funct_group17_23();

[[gnu::section(".trivial_handle")]] int
funct_group17_22()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group17_23();
  return side_effect;
}

int
funct_group17_23()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();

  if (side_effect > 0) {
    start_cycles = uptime();
    throw my_error_t{ .data = { 0xDE, 0xAD } };
  }

  return side_effect;
}

int
funct_group18_1();

[[gnu::section(".trivial_handle")]] int
funct_group18_0()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group18_1();
  return side_effect;
}

int
funct_group18_2();

int
funct_group18_1()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group18_2();
  return side_effect;
}

int
funct_group18_3();

[[gnu::section(".trivial_handle")]] int
funct_group18_2()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group18_3();
  return side_effect;
}

int
funct_group18_4();

int
funct_group18_3()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group18_4();
  return side_effect;
}

int
funct_group18_5();

[[gnu::section(".trivial_handle")]] int
funct_group18_4()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group18_5();
  return side_effect;
}

int
funct_group18_6();

int
funct_group18_5()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group18_6();
  return side_effect;
}

int
funct_group18_7();

[[gnu::section(".trivial_handle")]] int
funct_group18_6()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group18_7();
  return side_effect;
}

int
funct_group18_8();

int
funct_group18_7()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group18_8();
  return side_effect;
}

int
funct_group18_9();

[[gnu::section(".trivial_handle")]] int
funct_group18_8()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group18_9();
  return side_effect;
}

int
funct_group18_10();

int
funct_group18_9()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group18_10();
  return side_effect;
}

int
funct_group18_11();

[[gnu::section(".trivial_handle")]] int
funct_group18_10()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group18_11();
  return side_effect;
}

int
funct_group18_12();

int
funct_group18_11()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group18_12();
  return side_effect;
}

int
funct_group18_13();

[[gnu::section(".trivial_handle")]] int
funct_group18_12()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group18_13();
  return side_effect;
}

int
funct_group18_14();

int
funct_group18_13()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group18_14();
  return side_effect;
}

int
funct_group18_15();

[[gnu::section(".trivial_handle")]] int
funct_group18_14()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group18_15();
  return side_effect;
}

int
funct_group18_16();

int
funct_group18_15()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group18_16();
  return side_effect;
}

int
funct_group18_17();

[[gnu::section(".trivial_handle")]] int
funct_group18_16()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group18_17();
  return side_effect;
}

int
funct_group18_18();

int
funct_group18_17()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group18_18();
  return side_effect;
}

int
funct_group18_19();

[[gnu::section(".trivial_handle")]] int
funct_group18_18()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group18_19();
  return side_effect;
}

int
funct_group18_20();

int
funct_group18_19()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group18_20();
  return side_effect;
}

int
funct_group18_21();

[[gnu::section(".trivial_handle")]] int
funct_group18_20()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group18_21();
  return side_effect;
}

int
funct_group18_22();

int
funct_group18_21()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group18_22();
  return side_effect;
}

int
funct_group18_23();

[[gnu::section(".trivial_handle")]] int
funct_group18_22()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group18_23();
  return side_effect;
}

int
funct_group18_24();

int
funct_group18_23()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group18_24();
  return side_effect;
}

int
funct_group18_25();

[[gnu::section(".trivial_handle")]] int
funct_group18_24()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group18_25();
  return side_effect;
}

int
funct_group18_26();

int
funct_group18_25()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group18_26();
  return side_effect;
}

int
funct_group18_27();

[[gnu::section(".trivial_handle")]] int
funct_group18_26()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group18_27();
  return side_effect;
}

int
funct_group18_28();

int
funct_group18_27()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group18_28();
  return side_effect;
}

int
funct_group18_29();

[[gnu::section(".trivial_handle")]] int
funct_group18_28()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group18_29();
  return side_effect;
}

int
funct_group18_30();

int
funct_group18_29()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group18_30();
  return side_effect;
}

int
funct_group18_31();

[[gnu::section(".trivial_handle")]] int
funct_group18_30()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group18_31();
  return side_effect;
}

int
funct_group18_32();

int
funct_group18_31()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group18_32();
  return side_effect;
}

int
funct_group18_33();

[[gnu::section(".trivial_handle")]] int
funct_group18_32()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group18_33();
  return side_effect;
}

int
funct_group18_34();

int
funct_group18_33()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group18_34();
  return side_effect;
}

int
funct_group18_35();

[[gnu::section(".trivial_handle")]] int
funct_group18_34()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group18_35();
  return side_effect;
}

int
funct_group18_36();

int
funct_group18_35()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group18_36();
  return side_effect;
}

int
funct_group18_37();

[[gnu::section(".trivial_handle")]] int
funct_group18_36()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group18_37();
  return side_effect;
}

int
funct_group18_38();

int
funct_group18_37()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group18_38();
  return side_effect;
}

int
funct_group18_39();

[[gnu::section(".trivial_handle")]] int
funct_group18_38()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group18_39();
  return side_effect;
}

int
funct_group18_40();

int
funct_group18_39()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group18_40();
  return side_effect;
}

int
funct_group18_41();

[[gnu::section(".trivial_handle")]] int
funct_group18_40()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group18_41();
  return side_effect;
}

int
funct_group18_42();

int
funct_group18_41()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group18_42();
  return side_effect;
}

int
funct_group18_43();

[[gnu::section(".trivial_handle")]] int
funct_group18_42()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group18_43();
  return side_effect;
}

int
funct_group18_44();

int
funct_group18_43()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group18_44();
  return side_effect;
}

int
funct_group18_45();

[[gnu::section(".trivial_handle")]] int
funct_group18_44()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group18_45();
  return side_effect;
}

int
funct_group18_46();

int
funct_group18_45()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group18_46();
  return side_effect;
}

int
funct_group18_47();

[[gnu::section(".trivial_handle")]] int
funct_group18_46()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_0 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group18_47();
  return side_effect;
}

int
funct_group18_47()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();

  if (side_effect > 0) {
    start_cycles = uptime();
    throw my_error_t{ .data = { 0xDE, 0xAD } };
  }

  return side_effect;
}

int
funct_group20_1();

int
funct_group20_0()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group20_1();
  return side_effect;
}

int
funct_group20_2();

int
funct_group20_1()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group20_2();
  return side_effect;
}

int
funct_group20_3();

int
funct_group20_2()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group20_3();
  return side_effect;
}

int
funct_group20_4();

int
funct_group20_3()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group20_4();
  return side_effect;
}

int
funct_group20_5();

int
funct_group20_4()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group20_5();
  return side_effect;
}

int
funct_group20_5()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();

  if (side_effect > 0) {
    start_cycles = uptime();
    throw my_error_t{ .data = { 0xDE, 0xAD } };
  }

  return side_effect;
}

int
funct_group21_1();

int
funct_group21_0()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group21_1();
  return side_effect;
}

int
funct_group21_2();

int
funct_group21_1()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group21_2();
  return side_effect;
}

int
funct_group21_3();

int
funct_group21_2()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group21_3();
  return side_effect;
}

int
funct_group21_4();

int
funct_group21_3()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group21_4();
  return side_effect;
}

int
funct_group21_5();

int
funct_group21_4()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group21_5();
  return side_effect;
}

int
funct_group21_6();

int
funct_group21_5()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group21_6();
  return side_effect;
}

int
funct_group21_7();

int
funct_group21_6()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group21_7();
  return side_effect;
}

int
funct_group21_8();

int
funct_group21_7()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group21_8();
  return side_effect;
}

int
funct_group21_9();

int
funct_group21_8()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group21_9();
  return side_effect;
}

int
funct_group21_10();

int
funct_group21_9()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group21_10();
  return side_effect;
}

int
funct_group21_11();

int
funct_group21_10()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group21_11();
  return side_effect;
}

int
funct_group21_11()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();

  if (side_effect > 0) {
    start_cycles = uptime();
    throw my_error_t{ .data = { 0xDE, 0xAD } };
  }

  return side_effect;
}

int
funct_group22_1();

int
funct_group22_0()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group22_1();
  return side_effect;
}

int
funct_group22_2();

int
funct_group22_1()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group22_2();
  return side_effect;
}

int
funct_group22_3();

int
funct_group22_2()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group22_3();
  return side_effect;
}

int
funct_group22_4();

int
funct_group22_3()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group22_4();
  return side_effect;
}

int
funct_group22_5();

int
funct_group22_4()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group22_5();
  return side_effect;
}

int
funct_group22_6();

int
funct_group22_5()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group22_6();
  return side_effect;
}

int
funct_group22_7();

int
funct_group22_6()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group22_7();
  return side_effect;
}

int
funct_group22_8();

int
funct_group22_7()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group22_8();
  return side_effect;
}

int
funct_group22_9();

int
funct_group22_8()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group22_9();
  return side_effect;
}

int
funct_group22_10();

int
funct_group22_9()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group22_10();
  return side_effect;
}

int
funct_group22_11();

int
funct_group22_10()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group22_11();
  return side_effect;
}

int
funct_group22_12();

int
funct_group22_11()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group22_12();
  return side_effect;
}

int
funct_group22_13();

int
funct_group22_12()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group22_13();
  return side_effect;
}

int
funct_group22_14();

int
funct_group22_13()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group22_14();
  return side_effect;
}

int
funct_group22_15();

int
funct_group22_14()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group22_15();
  return side_effect;
}

int
funct_group22_16();

int
funct_group22_15()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group22_16();
  return side_effect;
}

int
funct_group22_17();

int
funct_group22_16()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group22_17();
  return side_effect;
}

int
funct_group22_18();

int
funct_group22_17()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group22_18();
  return side_effect;
}

int
funct_group22_19();

int
funct_group22_18()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group22_19();
  return side_effect;
}

int
funct_group22_20();

int
funct_group22_19()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group22_20();
  return side_effect;
}

int
funct_group22_21();

int
funct_group22_20()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group22_21();
  return side_effect;
}

int
funct_group22_22();

int
funct_group22_21()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group22_22();
  return side_effect;
}

int
funct_group22_23();

int
funct_group22_22()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group22_23();
  return side_effect;
}

int
funct_group22_23()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();

  if (side_effect > 0) {
    start_cycles = uptime();
    throw my_error_t{ .data = { 0xDE, 0xAD } };
  }

  return side_effect;
}

int
funct_group23_1();

int
funct_group23_0()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group23_1();
  return side_effect;
}

int
funct_group23_2();

int
funct_group23_1()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group23_2();
  return side_effect;
}

int
funct_group23_3();

int
funct_group23_2()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group23_3();
  return side_effect;
}

int
funct_group23_4();

int
funct_group23_3()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group23_4();
  return side_effect;
}

int
funct_group23_5();

int
funct_group23_4()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group23_5();
  return side_effect;
}

int
funct_group23_6();

int
funct_group23_5()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group23_6();
  return side_effect;
}

int
funct_group23_7();

int
funct_group23_6()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group23_7();
  return side_effect;
}

int
funct_group23_8();

int
funct_group23_7()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group23_8();
  return side_effect;
}

int
funct_group23_9();

int
funct_group23_8()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group23_9();
  return side_effect;
}

int
funct_group23_10();

int
funct_group23_9()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group23_10();
  return side_effect;
}

int
funct_group23_11();

int
funct_group23_10()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group23_11();
  return side_effect;
}

int
funct_group23_12();

int
funct_group23_11()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group23_12();
  return side_effect;
}

int
funct_group23_13();

int
funct_group23_12()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group23_13();
  return side_effect;
}

int
funct_group23_14();

int
funct_group23_13()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group23_14();
  return side_effect;
}

int
funct_group23_15();

int
funct_group23_14()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group23_15();
  return side_effect;
}

int
funct_group23_16();

int
funct_group23_15()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group23_16();
  return side_effect;
}

int
funct_group23_17();

int
funct_group23_16()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group23_17();
  return side_effect;
}

int
funct_group23_18();

int
funct_group23_17()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group23_18();
  return side_effect;
}

int
funct_group23_19();

int
funct_group23_18()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group23_19();
  return side_effect;
}

int
funct_group23_20();

int
funct_group23_19()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group23_20();
  return side_effect;
}

int
funct_group23_21();

int
funct_group23_20()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group23_21();
  return side_effect;
}

int
funct_group23_22();

int
funct_group23_21()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group23_22();
  return side_effect;
}

int
funct_group23_23();

int
funct_group23_22()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group23_23();
  return side_effect;
}

int
funct_group23_24();

int
funct_group23_23()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group23_24();
  return side_effect;
}

int
funct_group23_25();

int
funct_group23_24()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group23_25();
  return side_effect;
}

int
funct_group23_26();

int
funct_group23_25()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group23_26();
  return side_effect;
}

int
funct_group23_27();

int
funct_group23_26()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group23_27();
  return side_effect;
}

int
funct_group23_28();

int
funct_group23_27()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group23_28();
  return side_effect;
}

int
funct_group23_29();

int
funct_group23_28()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group23_29();
  return side_effect;
}

int
funct_group23_30();

int
funct_group23_29()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group23_30();
  return side_effect;
}

int
funct_group23_31();

int
funct_group23_30()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group23_31();
  return side_effect;
}

int
funct_group23_32();

int
funct_group23_31()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group23_32();
  return side_effect;
}

int
funct_group23_33();

int
funct_group23_32()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group23_33();
  return side_effect;
}

int
funct_group23_34();

int
funct_group23_33()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group23_34();
  return side_effect;
}

int
funct_group23_35();

int
funct_group23_34()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group23_35();
  return side_effect;
}

int
funct_group23_36();

int
funct_group23_35()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group23_36();
  return side_effect;
}

int
funct_group23_37();

int
funct_group23_36()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group23_37();
  return side_effect;
}

int
funct_group23_38();

int
funct_group23_37()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group23_38();
  return side_effect;
}

int
funct_group23_39();

int
funct_group23_38()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group23_39();
  return side_effect;
}

int
funct_group23_40();

int
funct_group23_39()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group23_40();
  return side_effect;
}

int
funct_group23_41();

int
funct_group23_40()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group23_41();
  return side_effect;
}

int
funct_group23_42();

int
funct_group23_41()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group23_42();
  return side_effect;
}

int
funct_group23_43();

int
funct_group23_42()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group23_43();
  return side_effect;
}

int
funct_group23_44();

int
funct_group23_43()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group23_44();
  return side_effect;
}

int
funct_group23_45();

int
funct_group23_44()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group23_45();
  return side_effect;
}

int
funct_group23_46();

int
funct_group23_45()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group23_46();
  return side_effect;
}

int
funct_group23_47();

int
funct_group23_46()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();
  side_effect = side_effect + funct_group23_47();
  return side_effect;
}

int
funct_group23_47()
{
  volatile static std::uint32_t inner_side_effect = 0;
  inner_side_effect = inner_side_effect + 1;
  class_1 instance_0(side_effect);
  instance_0.trigger();

  if (side_effect > 0) {
    start_cycles = uptime();
    throw my_error_t{ .data = { 0xDE, 0xAD } };
  }

  return side_effect;
}