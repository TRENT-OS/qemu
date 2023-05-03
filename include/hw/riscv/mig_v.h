/*
 * QEMU RISC-V Board, loosely compatible with HENSOLDT Cyber MiG-V
 *
 * Copyright (c) 2020 Fraunhofer AISEC
 *
 * Based on opentitan.h, which is
 * Copyright (c) 2020 Western Digital
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2 or later, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef HW_MIG_V_H
#define HW_MIG_V_H

#include "hw/nvram/mig_v_otp.h"
#include "hw/riscv/riscv_hart.h"
#include "qom/object.h"

#define TYPE_RISCV_MIG_V_SOC "riscv.mig_v.soc"
OBJECT_DECLARE_SIMPLE_TYPE(MiGVSoCState, RISCV_MIG_V_SOC)

struct MiGVSoCState {
    /*< private >*/
    SysBusDevice parent_obj;

    /*< public >*/
    RISCVHartArrayState cpus;
    DeviceState *plic;
    MiGVOTPState otp;

    MemoryRegion rom;
    MemoryRegion int_flash;
    MemoryRegion int_ram;
    MemoryRegion soc_ctrl;
    MemoryRegion gpio;
};

#define TYPE_RISCV_MIG_V_MACHINE MACHINE_TYPE_NAME("mig-v")
OBJECT_DECLARE_SIMPLE_TYPE(MiGVState, RISCV_MIG_V_MACHINE)

struct MiGVState {
    /*< private >*/
    SysBusDevice parent_obj;

    /*< public >*/
    MiGVSoCState soc;
    bool ext_flash;
    bool ext_ram;
};

enum {
    MIG_V_DEV_PLIC,
    MIG_V_DEV_CLINT,
    MIG_V_DEV_UART,
    MIG_V_DEV_OTP,
    MIG_V_DEV_GPIO,
    MIG_V_DEV_SOC_CTRL,
    MIG_V_DEV_INT_RAM,
    MIG_V_DEV_ROM,
    MIG_V_DEV_INT_FLASH,
    MIG_V_DEV_EXT_RAM,
};

enum {
    UART_IRQ = 10,
};

#define MIG_V_PLIC_NUM_SOURCES 30
#define MIG_V_PLIC_NUM_PRIORITIES 7
#define MIG_V_PLIC_PRIORITY_BASE 0x04
#define MIG_V_PLIC_PENDING_BASE 0x1000
#define MIG_V_PLIC_ENABLE_BASE 0x2000
#define MIG_V_PLIC_ENABLE_STRIDE 0x80
#define MIG_V_PLIC_CONTEXT_BASE 0x200000
#define MIG_V_PLIC_CONTEXT_STRIDE 0x1000

#endif
