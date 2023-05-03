/*
 * Simplified MiG-V OTP controller
 *
 * Copyright (c) 2020 Fraunhofer AISEC
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

#ifndef HW_MIG_V_OTP_H
#define HW_MIG_V_OTP_H

#include "hw/qdev-properties.h"
#include "hw/sysbus.h"
#include "qom/object.h"

#define MIG_V_OTP_ADDR 0x00
#define MIG_V_OTP_DATA 0x04

/* 4 KiB OTP fuse array */
#define MIG_V_OTP_NUM_FUSES 0x400

#define TYPE_MIG_V_OTP "riscv.mig_v.otp"
OBJECT_DECLARE_SIMPLE_TYPE(MiGVOTPState, MIG_V_OTP)

struct MiGVOTPState {
    /*< private >*/
    SysBusDevice parent_obj;

    /*< public >*/
    MemoryRegion mmio;
    BlockBackend *blk;
    bool blk_ro;

    uint32_t addr;
    uint32_t fuse_array[MIG_V_OTP_NUM_FUSES];
    uint16_t nb_fuses;
};

#endif
