// SPDX-License-Identifier: GPL-2.0 or MIT

//! Debugfs support for the Tyr DRM driver.

use kernel::{
    debugfs,
    device::Core,
    drm,
    drm::gem::{
        BaseObject,
        IntoGEMObject, //
    },
    fmt,
    fmt::Write,
    global_lock,
    impl_flags,
    platform,
    prelude::*,
    str::CString,
    sync::{
        aref::ARef,
        Arc,
        ArcBorrow,
        Mutex, //
    }, //
};

use crate::driver::TyrDrmDriver;
use crate::gem::Bo;
use crate::vm::Vm;

global_lock! {
    pub(crate) unsafe(uninit) static DEBUGFS_ROOT: Mutex<Option<debugfs::Dir>> = None;
}

impl_flags!(
    #[derive(Debug, Clone, Default, Copy, PartialEq, Eq)]
    struct BoStateFlags(u32);

    #[derive(Debug, Clone, Copy, PartialEq, Eq)]
    enum BoStateFlag {
        Imported = 1 << 0,
        Exported = 1 << 1,
    }
);

/// Writes per-BO debug information for the "gems" debugfs file.
fn show_bo(bo: &Bo, f: &mut impl Write) -> core::fmt::Result {
    let refcount: u32 = bo.refcount();
    let size: usize = bo.size();
    let resident_size: usize = bo.resident_size();

    let name: i32 = bo.name();
    let is_exported = bo.is_exported();
    let mmap_offset = bo.mmap_offset();

    let mut gem_state_flags = BoStateFlags::default();

    if bo.is_imported() {
        gem_state_flags |= BoStateFlag::Imported;
    }
    if is_exported {
        gem_state_flags |= BoStateFlag::Exported;
    }

    writeln!(
        f,
        "{:<16}{:<16}{:<16}{:<16}0x{:<16x}0x{:<8x}",
        name,
        refcount,
        size,
        resident_size,
        mmap_offset,
        u32::from(gem_state_flags),
    )
}

fn show_gems(data: &Arc<TyrDebugFSData>, f: &mut fmt::Formatter<'_>) -> core::fmt::Result {
    writeln!(
        f,
        "GEM state flags: {:?} (0x{:x}), {:?} (0x{:x})",
        BoStateFlag::Imported,
        BoStateFlag::Imported as u32,
        BoStateFlag::Exported,
        BoStateFlag::Exported as u32,
    )?;
    writeln!(
        f,
        "global-name     refcount        size            resident-size   file-offset       state",
    )?;
    writeln!(
        f,
        "--------------------------------------------------------------------------------------------",
    )?;

    let mut total_size: usize = 0;
    let mut total_resident: usize = 0;
    let mut total_reclaimable: usize = 0;
    let gems = data.gems.lock();
    for bo in gems.iter() {
        total_size += bo.size();
        total_resident += bo.resident_size();
        if bo.madv() > 0 {
            total_reclaimable += bo.resident_size();
        }
        show_bo(bo, f)?;
    }

    writeln!(
        f,
        "============================================================================================",
    )?;
    writeln!(
        f,
        "Total size: {}, Total resident: {}, Total reclaimable: {}",
        total_size, total_resident, total_reclaimable,
    )?;
    Ok(())
}

/// Per-device debugfs data.
#[pin_data]
pub(crate) struct TyrDebugFSData {
    #[pin]
    pub(crate) vms: Mutex<KVec<Arc<Vm>>>,
    #[pin]
    pub(crate) gems: Mutex<KVec<ARef<Bo>>>,
}

/// Writes VM debug information for the "gpuvas" debugfs file.
fn show_vm(vm: &Vm, f: &mut impl Write) -> core::fmt::Result {
    writeln!(
        f,
        "DRM GPU VA space ({:?}) [0x{:016x};0x{:016x}]",
        vm.gpuvm.name(),
        vm.va_range.start,
        vm.va_range.end,
    )?;

    let kva = vm.gpuvm.kernel_alloc_va();
    writeln!(
        f,
        "Kernel reserved node [0x{:016x};0x{:016x}]",
        kva.addr(),
        kva.addr() + kva.length(),
    )?;

    writeln!(f, " VAs | start              | range              | end                | object             | object offset")?;
    writeln!(f, "-------------------------------------------------------------------------------------------------------------")?;
    for va in vm.gpuvm_unique.lock().va_mappings() {
        f.write_fmt(fmt!(
            "     | 0x{:016x} | 0x{:016x} | 0x{:016x} | {:18p} | 0x{:016x}\n",
            va.addr(),
            va.length(),
            va.addr() + va.length(),
            va.obj().as_raw(),
            va.gem_offset(),
        ))?;
    }
    Ok(())
}

fn show_gpuvas(data: &Arc<TyrDebugFSData>, f: &mut fmt::Formatter<'_>) -> core::fmt::Result {
    let vms = data.vms.lock();
    for vm in vms.iter() {
        show_vm(vm, f)?;
        writeln!(f)?;
    }
    Ok(())
}

/// Registers per-device debugfs directory under the module's debugfs root.
pub(crate) fn debugfs_init(
    ddev: &drm::Device<TyrDrmDriver>,
    pdev: &platform::Device<Core>,
    debugfs_data: ArcBorrow<'_, TyrDebugFSData>,
) -> Result {
    let idx = ddev.primary_index();
    let dir_name = CString::try_from_fmt(fmt!("{}", idx))?;

    if let Some(root_dir) = DEBUGFS_ROOT.lock().as_ref() {
        let debugfs_data: Arc<TyrDebugFSData> = debugfs_data.into();
        let scope_init = root_dir.scope(debugfs_data, &dir_name, |data, dir| {
            dir.read_callback_file(c"gpuvas", data, &show_gpuvas);
            dir.read_callback_file(c"gems", data, &show_gems);
        });
        kernel::devres::register(pdev.as_ref(), scope_init, GFP_KERNEL)
    } else {
        dev_err!(ddev, "debugfs root not found");
        Err(ENOENT)
    }
}
