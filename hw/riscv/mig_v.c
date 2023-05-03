/*
 * QEMU RISC-V Board, loosely compatible with HENSOLDT Cyber MiG-V
 *
 * Copyright (c) 2020 Fraunhofer AISEC
 *
 * Based on opentitan.c, which is
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

#include "qemu/osdep.h"
#include "qemu/units.h"
#include "qapi/error.h"
#include "hw/boards.h"
#include "hw/qdev-properties.h"
#include "hw/char/serial.h"
#include "hw/intc/riscv_aclint.h"
#include "hw/intc/sifive_plic.h"
#include "hw/nvram/mig_v_otp.h"
#include "hw/riscv/boot.h"
#include "hw/riscv/mig_v.h"
#include "sysemu/block-backend.h"
#include "sysemu/sysemu.h"

static const struct MemmapEntry {
    hwaddr base;
    hwaddr size;
} mig_v_memmap[] = {
    [MIG_V_DEV_PLIC]        = { 0x00200000, 4 * KiB },
    [MIG_V_DEV_CLINT]       = { 0x00201000, 48 * KiB },
    [MIG_V_DEV_UART]        = { 0x00404000, 4 * KiB },
    [MIG_V_DEV_OTP]         = { 0x00410000, 4 * KiB },
    [MIG_V_DEV_GPIO]        = { 0x00408000, 4 * KiB },
    [MIG_V_DEV_SOC_CTRL]    = { 0x0040E000, 4 * KiB },
    [MIG_V_DEV_INT_RAM]     = { 0x01000000, 1 * MiB },
    [MIG_V_DEV_ROM]         = { 0x02000000, 512 * KiB },
    [MIG_V_DEV_INT_FLASH]   = { 0x03000000, 2 * MiB },
    [MIG_V_DEV_EXT_RAM]     = { 0x40000000, 8 * MiB }
};

static void mig_v_board_init(MachineState *machine)
{
    const struct MemmapEntry *memmap = mig_v_memmap;
    MiGVState *s = RISCV_MIG_V_MACHINE(machine);
    MemoryRegion *sys_mem = get_system_memory();
    MemoryRegion *ext_ram_mem = g_new(MemoryRegion, 1);

    /* SoC */
    object_initialize_child(OBJECT(machine), "soc", &s->soc,
                            TYPE_RISCV_MIG_V_SOC);
    qdev_realize(DEVICE(&s->soc), NULL, &error_abort);

    /* External RAM */
    if (s->ext_ram) {
        memory_region_init_ram(ext_ram_mem, NULL, "riscv.mig_v.ext_ram",
                            memmap[MIG_V_DEV_EXT_RAM].size, &error_fatal);
        memory_region_add_subregion(sys_mem, memmap[MIG_V_DEV_EXT_RAM].base,
                                    ext_ram_mem);
    }

    if (!machine->firmware) {
        error_report("No ROM image (-bios) specified");
        exit(1);
    }

    riscv_load_firmware(machine->firmware, memmap[MIG_V_DEV_ROM].base, NULL);
}

static bool mig_v_get_ext_flash(Object *obj, Error **errp)
{
    MiGVState *s = RISCV_MIG_V_MACHINE(obj);

    return s->ext_flash;
}

static void mig_v_set_ext_flash(Object *obj, bool value, Error **errp)
{
    MiGVState *s = RISCV_MIG_V_MACHINE(obj);

    s->ext_flash = value;
}

static bool mig_v_get_ext_ram(Object *obj, Error **errp)
{
    MiGVState *s = RISCV_MIG_V_MACHINE(obj);

    return s->ext_ram;
}

static void mig_v_set_ext_ram(Object *obj, bool value, Error **errp)
{
    MiGVState *s = RISCV_MIG_V_MACHINE(obj);

    s->ext_ram = value;
}

static void mig_v_machine_instance_init(Object *obj)
{
    MiGVState *s = RISCV_MIG_V_MACHINE(obj);

    s->ext_flash = true;
    object_property_add_bool(obj, "ext_flash", mig_v_get_ext_flash,
                             mig_v_set_ext_flash);
    object_property_set_description(obj, "ext_flash",
                                    "Set to on / off to enable / disable the "
                                    "external flash");

    s->ext_ram = true;
    object_property_add_bool(obj, "ext_ram", mig_v_get_ext_ram,
                             mig_v_set_ext_ram);
    object_property_set_description(obj, "ext_ram",
                                    "Set to on / off to enable / disable the "
                                    "external RAM");
}

static void mig_v_machine_class_init(ObjectClass *oc, void *data)
{
    MachineClass *mc = MACHINE_CLASS(oc);

    mc->desc = "RISC-V board loosely compatible with HENSOLDT Cyber MiG-V";
    mc->init = mig_v_board_init;
    mc->max_cpus = 1;
    mc->default_cpu_type = TYPE_RISCV_CPU_MIG_V;
}

static const TypeInfo mig_v_machine_typeinfo = {
    .name = TYPE_RISCV_MIG_V_MACHINE,
    .parent = TYPE_MACHINE,
    .class_init = mig_v_machine_class_init,
    .instance_init = mig_v_machine_instance_init,
    .instance_size = sizeof(MiGVState),
};

static void mig_v_machine_init_register_types(void)
{
    type_register_static(&mig_v_machine_typeinfo);
}

type_init(mig_v_machine_init_register_types)

static void mig_v_soc_init(Object *obj)
{
    MiGVSoCState *s = RISCV_MIG_V_SOC(obj);

    object_initialize_child(obj, "cpus", &s->cpus, TYPE_RISCV_HART_ARRAY);
    object_initialize_child(obj, "otp", &s->otp, TYPE_MIG_V_OTP);
}

static void mig_v_soc_realize(DeviceState *dev_soc, Error **errp)
{
    const struct MemmapEntry *memmap = mig_v_memmap;
    MachineState *ms = MACHINE(qdev_get_machine());
    MiGVSoCState *s = RISCV_MIG_V_SOC(dev_soc);
    MemoryRegion *sys_mem = get_system_memory();
    BlockBackend *blk;

    /* CPU */
    object_property_set_str(OBJECT(&s->cpus), "cpu-type", ms->cpu_type,
                            &error_abort);
    object_property_set_int(OBJECT(&s->cpus), "num-harts", ms->smp.cpus,
                            &error_abort);
    object_property_set_int(OBJECT(&s->cpus), "resetvec",
                            memmap[MIG_V_DEV_ROM].base, &error_abort);
    sysbus_realize(SYS_BUS_DEVICE(&s->cpus), &error_abort);

    /* Boot ROM */
    memory_region_init_rom(&s->rom, OBJECT(dev_soc), "riscv.mig_v.rom",
                           memmap[MIG_V_DEV_ROM].size, &error_fatal);
    memory_region_add_subregion(sys_mem, memmap[MIG_V_DEV_ROM].base, &s->rom);

    /* Internal flash memory */
    memory_region_init_ram(&s->int_flash, OBJECT(dev_soc),
                           "riscv.mig_v.int_flash",
                           memmap[MIG_V_DEV_INT_FLASH].size, &error_fatal);
    memory_region_add_subregion(sys_mem, memmap[MIG_V_DEV_INT_FLASH].base,
                                &s->int_flash);

    /* Internal RAM */
    memory_region_init_ram(&s->int_ram, NULL, "riscv.mig_v.int_ram",
                           memmap[MIG_V_DEV_INT_RAM].size, &error_fatal);
    memory_region_add_subregion(sys_mem, memmap[MIG_V_DEV_INT_RAM].base,
                                &s->int_ram);

    /* OTP */
    blk = blk_by_name("otp");
    if (blk) {
        qdev_prop_set_drive_err(DEVICE(&s->otp), "drive", blk, &error_abort);
    }
    sysbus_realize(SYS_BUS_DEVICE(&s->otp), &error_abort);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->otp), 0, memmap[MIG_V_DEV_OTP].base);

    /* SoC configuration/control */
    memory_region_init_ram(&s->soc_ctrl, NULL, "riscv.mig_v.soc_ctrl",
                           memmap[MIG_V_DEV_SOC_CTRL].size, &error_fatal);
    memory_region_add_subregion(sys_mem, memmap[MIG_V_DEV_SOC_CTRL].base,
                                &s->soc_ctrl);

    /* GPIO */
    memory_region_init_ram(&s->gpio, NULL, "riscv.mig_v.gpio",
                           memmap[MIG_V_DEV_GPIO].size, &error_fatal);
    memory_region_add_subregion(sys_mem, memmap[MIG_V_DEV_GPIO].base,
                                &s->gpio);

    /* PLIC */
    s->plic = sifive_plic_create(memmap[MIG_V_DEV_PLIC].base, (char *)"MS", 1,
        0, MIG_V_PLIC_NUM_SOURCES, MIG_V_PLIC_NUM_PRIORITIES,
        MIG_V_PLIC_PRIORITY_BASE, MIG_V_PLIC_PENDING_BASE,
        MIG_V_PLIC_ENABLE_BASE, MIG_V_PLIC_ENABLE_STRIDE,
        MIG_V_PLIC_CONTEXT_BASE, MIG_V_PLIC_CONTEXT_STRIDE,
        memmap[MIG_V_DEV_PLIC].size);

    /* CLINT */
    riscv_aclint_swi_create(memmap[MIG_V_DEV_CLINT].base, 0, 1, false);
    riscv_aclint_mtimer_create(
        memmap[MIG_V_DEV_CLINT].base + RISCV_ACLINT_SWI_SIZE,
        RISCV_ACLINT_DEFAULT_MTIMER_SIZE, 0, 1, RISCV_ACLINT_DEFAULT_MTIMECMP,
        RISCV_ACLINT_DEFAULT_MTIME, RISCV_ACLINT_DEFAULT_TIMEBASE_FREQ, false);

    /* UART */
    serial_mm_init(sys_mem, memmap[MIG_V_DEV_UART].base,
        0, qdev_get_gpio_in(DEVICE(s->plic), UART_IRQ), 115200,
        serial_hd(0), DEVICE_LITTLE_ENDIAN);
}

static void mig_v_soc_class_init(ObjectClass *oc, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);

    dc->realize = mig_v_soc_realize;
    /* Reason: Uses serial_hds in realize function, thus can't be used twice */
    dc->user_creatable = false;
}

static const TypeInfo mig_v_soc_type_info = {
    .name = TYPE_RISCV_MIG_V_SOC,
    .parent = TYPE_DEVICE,
    .instance_size = sizeof(MiGVSoCState),
    .instance_init = mig_v_soc_init,
    .class_init = mig_v_soc_class_init,
};

static void mig_v_soc_register_types(void)
{
    type_register_static(&mig_v_soc_type_info);
}

type_init(mig_v_soc_register_types)
