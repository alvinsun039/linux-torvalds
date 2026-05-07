// SPDX-License-Identifier: GPL-2.0 or MIT

//! Arm Mali Tyr DRM driver.
//!
//! The name "Tyr" is inspired by Norse mythology, reflecting Arm's tradition of
//! naming their GPUs after Nordic mythological figures and places.

use crate::{
    debugfs::DEBUGFS_ROOT,
    driver::TyrPlatformDriverData, //
};
use kernel::{
    driver::Registration,
    error,
    platform,
    prelude::*,
    InPlaceModule, //
};

mod debugfs;
mod driver;
mod file;
mod fw;
mod gem;
mod gpu;
mod mmu;
mod regs;
mod slot;
mod vm;
mod wait;

pub(crate) const MODULE_NAME: &kernel::str::CStr = <LocalModule as kernel::ModuleMetadata>::NAME;

#[pin_data(PinnedDrop)]
struct TyrModule {
    #[pin]
    _driver: Registration<platform::Adapter<TyrPlatformDriverData>>,
}

#[pinned_drop]
impl PinnedDrop for TyrModule {
    fn drop(self: Pin<&mut Self>) {
        let _ = DEBUGFS_ROOT.lock().take();
    }
}

impl InPlaceModule for TyrModule {
    fn init(module: &'static kernel::ThisModule) -> impl PinInit<Self, error::Error> {
        // SAFETY: The module initializer never runs twice, so we only call this once.
        unsafe { DEBUGFS_ROOT.init() };

        let mut lock = DEBUGFS_ROOT.lock();
        *lock = Some(kernel::debugfs::Dir::new(kernel::c_str!("tyr")));
        drop(lock);

        try_pin_init!(Self {
            _driver <- Registration::new(MODULE_NAME, module),
        })
    }
}

module! {
    type: TyrModule,
    name: "tyr",
    authors: ["The Tyr driver authors"],
    description: "Arm Mali Tyr DRM driver",
    license: "Dual MIT/GPL",
}
