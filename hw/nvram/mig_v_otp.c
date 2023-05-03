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

#include "qemu/osdep.h"
#include "qemu/log.h"
#include "qapi/error.h"
#include "migration/vmstate.h"
#include "hw/block/block.h"
#include "hw/nvram/mig_v_otp.h"
#include "hw/qdev-properties.h"
#include "hw/sysbus.h"
#include "sysemu/block-backend.h"

static uint64_t mig_v_otp_read(void *opaque, hwaddr addr, unsigned int size)
{
    MiGVOTPState *s = opaque;

    switch (addr) {
    case MIG_V_OTP_ADDR:
        return (uint64_t)s->addr;
    case MIG_V_OTP_DATA:
        if (s->addr < s->nb_fuses) {
            return (uint64_t)s->fuse_array[s->addr];
        } else {
            qemu_log_mask(LOG_GUEST_ERROR,
                          "mig_v_otp: out of bounds OTP fuse read: fuse array "
                          "index: 0x%" PRIx32 "\n", s->addr);
            break;
        }
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "mig_v_otp: invalid register read access: register "
                      "address: 0x%" HWADDR_PRIx "\n", addr);
    }

    return 0;
}

static void mig_v_otp_write(void *opaque, hwaddr addr, uint64_t val64,
                            unsigned int size)
{
    MiGVOTPState *s = opaque;

    switch (addr) {
    case MIG_V_OTP_ADDR:
        s->addr = (uint32_t)val64;
        break;
    case MIG_V_OTP_DATA:
        if (s->addr < s->nb_fuses) {
            s->fuse_array[s->addr] |= (uint32_t)val64;

            if (s->blk && !s->blk_ro) {
                blk_pwrite(s->blk, s->addr * 4, &s->fuse_array[s->addr], 4, 0);
            }
        } else {
            qemu_log_mask(LOG_GUEST_ERROR,
                          "mig_v_otp: out of bounds OTP fuse write: fuse array "
                          "index: 0x%" PRIx32 ", value: 0x%" PRIx64 "\n",
                          s->addr, val64);
        }
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "mig_v_otp: invalid register write access: register "
                      "address: 0x%" HWADDR_PRIx "\n", addr);
    }
}

static const MemoryRegionOps mig_v_otp_ops = {
    .read = mig_v_otp_read,
    .write = mig_v_otp_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 8
    }
};

static const VMStateDescription mig_v_otp_vmstate = {
    .name = TYPE_MIG_V_OTP,
    .version_id = 0,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32(addr, MiGVOTPState),
        VMSTATE_UINT32_ARRAY(fuse_array, MiGVOTPState, MIG_V_OTP_NUM_FUSES),
        VMSTATE_UINT16(nb_fuses, MiGVOTPState),
        VMSTATE_END_OF_LIST()
    }
};

static void mig_v_otp_realize(DeviceState *dev, Error **errp)
{
    MiGVOTPState *s = MIG_V_OTP(dev);
    uint64_t perm;
    int64_t blk_len;
    int ret;

    memory_region_init_io(&s->mmio, OBJECT(dev), &mig_v_otp_ops, s,
                          TYPE_MIG_V_OTP, 0x1000);
    sysbus_init_mmio(SYS_BUS_DEVICE(dev), &s->mmio);

    if (s->blk) {
        s->blk_ro = !blk_supports_write_perm(s->blk);
        perm = BLK_PERM_CONSISTENT_READ | (s->blk_ro ? 0 : BLK_PERM_WRITE);

        ret = blk_set_perm(s->blk, perm, BLK_PERM_ALL, errp);
        if (ret < 0) {
            return;
        }

        blk_len = blk_getlength(s->blk);
        if (blk_len < 0) {
            return;
        }

        s->nb_fuses = QEMU_ALIGN_UP(blk_len, 4) >> 2;
        if (s->nb_fuses > MIG_V_OTP_NUM_FUSES) {
            error_setg(errp, "mig_v_otp: specified OTP backend exceeds maximum "
                             "size of %d fuses", MIG_V_OTP_NUM_FUSES);
            return;
        }

        if (!blk_check_size_and_read_all(s->blk, s->fuse_array, blk_len,
                                         errp)) {
            error_setg(errp, "mig_v_otp: failed to initialize OTP from drive");
            return;
        }
    }
}

static void mig_v_otp_enter_reset(Object *obj, ResetType type)
{
    MiGVOTPState *s = MIG_V_OTP(obj);

    /* If no drive is specified, initialize fuse array to 0's (antifuse) */
    if (!s->blk) {
        memset(s->fuse_array, 0, sizeof(s->fuse_array));
        s->nb_fuses = MIG_V_OTP_NUM_FUSES;
    }
}

static Property mig_v_properties[] = {
    DEFINE_PROP_DRIVE("drive", MiGVOTPState, blk),
    DEFINE_PROP_END_OF_LIST(),
};

static void mig_v_otp_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    ResettableClass *rc = RESETTABLE_CLASS(klass);

    dc->realize = mig_v_otp_realize;
    dc->vmsd = &mig_v_otp_vmstate;
    device_class_set_props(dc, mig_v_properties);
    rc->phases.enter = mig_v_otp_enter_reset;
}

static const TypeInfo mig_v_otp_info = {
    .name = TYPE_MIG_V_OTP,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(MiGVOTPState),
    .class_init = mig_v_otp_class_init,
};

static void mig_v_otp_register_types(void)
{
    type_register_static(&mig_v_otp_info);
}

type_init(mig_v_otp_register_types)
