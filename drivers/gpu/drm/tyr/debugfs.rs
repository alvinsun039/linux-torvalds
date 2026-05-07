// SPDX-License-Identifier: GPL-2.0 or MIT

//! Debugfs support for the Tyr DRM driver.

use kernel::{
    debugfs,
    device::Core,
    drm,
    global_lock,
    platform,
    prelude::*,
    str::CString,
    sync::{
        Arc,
        ArcBorrow, //
    }, //
};

use crate::driver::TyrDrmDriver;

global_lock! {
    pub(crate) unsafe(uninit) static DEBUGFS_ROOT: Mutex<Option<debugfs::Dir>> = None;
}

/// Per-device debugfs data.
pub(crate) struct TyrDebugFSData {}

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
        let scope_init = root_dir.scope(debugfs_data, &dir_name, |_data, _dir| {});
        kernel::devres::register(pdev.as_ref(), scope_init, GFP_KERNEL)
    } else {
        dev_err!(ddev, "debugfs root not found");
        Err(ENOENT)
    }
}
