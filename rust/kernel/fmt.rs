// SPDX-License-Identifier: GPL-2.0

//! Formatting utilities.
//!
//! This module is intended to be used in place of `core::fmt` in kernel code.

pub use core::fmt::{Arguments, Debug, Error, Formatter, Result, Write};
use kernel::str::CStrExt;

/// Internal adapter used to route and allow implementations of formatting traits for foreign types.
///
/// It is inserted automatically by the [`fmt!`] macro and is not meant to be used directly.
///
/// [`fmt!`]: crate::prelude::fmt!
#[doc(hidden)]
pub struct Adapter<T>(pub T);

macro_rules! impl_fmt_adapter_forward {
    ($($trait:ident),* $(,)?) => {
        $(
            impl<T: $trait> $trait for Adapter<T> {
                fn fmt(&self, f: &mut Formatter<'_>) -> Result {
                    let Self(t) = self;
                    $trait::fmt(t, f)
                }
            }
        )*
    };
}

use core::fmt::{Binary, LowerExp, LowerHex, Octal, UpperExp, UpperHex};
impl_fmt_adapter_forward!(Debug, LowerHex, UpperHex, Octal, Binary, LowerExp, UpperExp);

/// A copy of [`core::fmt::Display`] that allows us to implement it for foreign types.
///
/// Types should implement this trait rather than [`core::fmt::Display`]. Together with the
/// [`Adapter`] type and [`fmt!`] macro, it allows for formatting foreign types (e.g. types from
/// core) which do not implement [`core::fmt::Display`] directly.
///
/// [`fmt!`]: crate::prelude::fmt!
pub trait Display {
    /// Same as [`core::fmt::Display::fmt`].
    fn fmt(&self, f: &mut Formatter<'_>) -> Result;
}

impl<T: ?Sized + Display> Display for &T {
    fn fmt(&self, f: &mut Formatter<'_>) -> Result {
        Display::fmt(*self, f)
    }
}

impl<T: ?Sized + Display> core::fmt::Display for Adapter<&T> {
    fn fmt(&self, f: &mut Formatter<'_>) -> Result {
        let Self(t) = self;
        Display::fmt(t, f)
    }
}

macro_rules! impl_display_forward {
    ($(
        $( { $($generics:tt)* } )? $ty:ty $( { where $($where:tt)* } )?
    ),* $(,)?) => {
        $(
            impl$($($generics)*)? Display for $ty $(where $($where)*)? {
                fn fmt(&self, f: &mut Formatter<'_>) -> Result {
                    core::fmt::Display::fmt(self, f)
                }
            }
        )*
    };
}

impl_display_forward!(
    bool,
    char,
    core::panic::PanicInfo<'_>,
    Arguments<'_>,
    i128,
    i16,
    i32,
    i64,
    i8,
    isize,
    str,
    u128,
    u16,
    u32,
    u64,
    u8,
    usize,
    {<T: ?Sized>} crate::sync::Arc<T> {where crate::sync::Arc<T>: core::fmt::Display},
    {<T: ?Sized>} crate::sync::UniqueArc<T> {where crate::sync::UniqueArc<T>: core::fmt::Display},
);

/// A copy of [`core::fmt::Pointer`] to prevent raw pointers from being
/// formatted directly.
///
/// Together, the [`Adapter`] type, [`fmt!`] macro, and this trait ensure
/// that raw pointers (`*const T` and `*mut T`) are automatically
/// formatted using [`HashedPtr`] instead of being formatted directly,
/// preventing kernel memory layout information leakage.
///
/// [`fmt!`]: crate::prelude::fmt!
pub trait Pointer {
    /// Same as [`core::fmt::Pointer::fmt`].
    fn fmt(&self, f: &mut Formatter<'_>) -> Result;
}

impl<T: ?Sized> Pointer for &T {
    fn fmt(&self, f: &mut Formatter<'_>) -> Result {
        Pointer::fmt(&core::ptr::from_ref(*self), f)
    }
}

impl<T: ?Sized> Pointer for &mut T {
    fn fmt(&self, f: &mut Formatter<'_>) -> Result {
        Pointer::fmt(&core::ptr::from_ref(&**self), f)
    }
}

impl<T: Pointer> core::fmt::Pointer for Adapter<&T> {
    fn fmt(&self, f: &mut Formatter<'_>) -> Result {
        let Self(t) = self;
        Pointer::fmt(*t, f)
    }
}

macro_rules! impl_hashed_pointer {
    ($($ptr_ty:ty),* $(,)?) => {
        $(
            impl<T: ?Sized> Pointer for $ptr_ty {
                fn fmt(&self, f: &mut Formatter<'_>) -> Result {
                    Pointer::fmt(&HashedPtr(*self), f)
                }
            }
        )*
    };
}

impl_hashed_pointer!(*const T, *mut T);

/// A pointer that will be hashed when printed (corresponds to `%p`).
///
/// This is the default behavior for kernel pointers - they are hashed to
/// prevent leaking information about the kernel memory layout.
#[derive(Copy, Clone)]
pub struct HashedPtr<T: ?Sized>(pub *const T);

impl<T: ?Sized> Pointer for HashedPtr<T> {
    fn fmt(&self, f: &mut Formatter<'_>) -> Result {
        let ptr = self.0.cast::<()>();
        // The default width for %p is 2 * sizeof(ptr) (see lib/vsprintf.c:
        // pointer_string() and ptr_to_id()), which is 16 characters on
        // 64-bit systems. With the "0x" prefix and null terminator, this
        // is at most 19 characters. 32 bytes is sufficient for all pointer
        // formats including hash values.
        let mut buf = [0u8; 32];

        // SAFETY: scnprintf never writes more than size-1 characters, so
        // len is at most buf.len() - 1.
        let len =
            unsafe { bindings::scnprintf(buf.as_mut_ptr(), buf.len(), c"0x%p".as_char_ptr(), ptr) };
        // SAFETY: scnprintf with "%p" format always produces valid ASCII
        // output, which can be directly converted to UTF-8.
        let formatted = unsafe { core::str::from_utf8_unchecked(&buf[..len as usize]) };

        // Handle zero padding: insert zeros after "0x" prefix, matching
        // the behavior of core::fmt::Pointer for {:0width$p}.
        if f.sign_aware_zero_pad() {
            if let Some(width) = f.width() {
                if formatted.len() < width && formatted.starts_with("0x") {
                    return write!(f, "0x{:0>width$}", &formatted[2..], width = width - 2);
                }
            }
        }

        // Use pad() to respect width and alignment.
        f.pad(formatted)
    }
}

#[macros::kunit_tests(rust_kernel_fmt)]
mod tests {
    use crate::{
        bindings,
        prelude::fmt,
        str::CString, //
    };

    struct NoHashPointersGuard {
        original: bool,
    }

    impl NoHashPointersGuard {
        fn new(hashed: bool) -> Self {
            // SAFETY: no_hash_pointers is a global variable, safe to read.
            let original = unsafe { bindings::no_hash_pointers };
            // SAFETY: no_hash_pointers is writable during test execution.
            // KUnit tests run serially, so no other code modifies it.
            unsafe { bindings::no_hash_pointers = hashed };
            Self { original }
        }
    }

    impl Drop for NoHashPointersGuard {
        fn drop(&mut self) {
            // SAFETY: no_hash_pointers is writable during test execution.
            // KUnit tests run serially, so no other code modifies it.
            unsafe { bindings::no_hash_pointers = self.original };
        }
    }

    #[cfg(CONFIG_64BIT)]
    mod expected {
        // Test pointer value (64-bit)
        pub(super) const PTR_VALUE: usize = 0xffffffffdeadbeef;

        // Hashed pointer prefix
        pub(super) const HASHED_PREFIX: &str = "0x00000000";

        // Expected raw pointer output
        pub(super) const RAW_POINTER: &str = "0xffffffffdeadbeef";

        // Width padding (30 chars, space-filled)
        pub(super) const PADDED_LEFT: &str = "0xffffffffdeadbeef            ";
        pub(super) const PADDED_RIGHT: &str = "            0xffffffffdeadbeef";
        pub(super) const PADDED_CENTER: &str = "      0xffffffffdeadbeef      ";

        // Zero padding (30 chars)
        pub(super) const ZERO_PADDED: &str = "0x000000000000ffffffffdeadbeef";

        // Fill character '*' padding (30 chars)
        pub(super) const FILLED_LEFT: &str = "0xffffffffdeadbeef************";
        pub(super) const FILLED_RIGHT: &str = "************0xffffffffdeadbeef";
        pub(super) const FILLED_CENTER: &str = "******0xffffffffdeadbeef******";
    }

    #[cfg(not(CONFIG_64BIT))]
    mod expected {
        // Test pointer value (32-bit)
        pub(super) const PTR_VALUE: usize = 0xdeadbeef;

        // Hashed pointer prefix
        pub(super) const HASHED_PREFIX: &str = "0x";

        // Expected raw pointer output
        pub(super) const RAW_POINTER: &str = "0xdeadbeef";

        // Width padding (30 chars, space-filled)
        pub(super) const PADDED_LEFT: &str = "0xdeadbeef                    ";
        pub(super) const PADDED_RIGHT: &str = "                    0xdeadbeef";
        pub(super) const PADDED_CENTER: &str = "          0xdeadbeef          ";

        // Zero padding (30 chars)
        pub(super) const ZERO_PADDED: &str = "0x00000000000000000000deadbeef";

        // Fill character '*' padding (30 chars)
        pub(super) const FILLED_LEFT: &str = "0xdeadbeef********************";
        pub(super) const FILLED_RIGHT: &str = "********************0xdeadbeef";
        pub(super) const FILLED_CENTER: &str = "**********0xdeadbeef**********";
    }

    #[test]
    fn test_hashed_pointer() -> Result<(), crate::error::Error> {
        let _guard = NoHashPointersGuard::new(false);

        // Thin Pointer: *const u8
        let thin_ptr = expected::PTR_VALUE as *const u8;
        let thin_ptr_cstr = CString::try_from_fmt(fmt!("{:p}", thin_ptr))?;
        let formatted = thin_ptr_cstr.to_str()?;
        assert!(formatted.starts_with(expected::HASHED_PREFIX));
        assert_ne!(formatted, expected::RAW_POINTER);

        // Fat Pointer: *const str
        let string_slice = "hello";
        let fat_ptr_str: *const str = string_slice as *const str;
        let str_ptr_cstr = CString::try_from_fmt(fmt!("{:p}", fat_ptr_str))?;
        let data_ptr_cstr = CString::try_from_fmt(fmt!("{:p}", fat_ptr_str.as_ptr()))?;
        assert_eq!(str_ptr_cstr.to_str()?, data_ptr_cstr.to_str()?);

        Ok(())
    }

    #[test]
    fn test_raw_pointer() -> Result<(), crate::error::Error> {
        let _guard = NoHashPointersGuard::new(true);

        // Thin Pointer: *const u8
        let thin_ptr = expected::PTR_VALUE as *const u8;
        let thin_ptr_cstr = CString::try_from_fmt(fmt!("{:p}", thin_ptr))?;
        assert_eq!(thin_ptr_cstr.to_str()?, expected::RAW_POINTER);

        // Fat Pointer: *const str
        let string_slice = "hello";
        let fat_ptr_str: *const str = string_slice as *const str;
        let str_ptr_cstr = CString::try_from_fmt(fmt!("{:p}", fat_ptr_str))?;
        let data_ptr_cstr = CString::try_from_fmt(fmt!("{:p}", fat_ptr_str.as_ptr()))?;
        assert_eq!(str_ptr_cstr.to_str()?, data_ptr_cstr.to_str()?);

        Ok(())
    }

    #[test]
    fn test_pointer_width_padding() -> Result<(), crate::error::Error> {
        let _guard = NoHashPointersGuard::new(true);
        let ptr = expected::PTR_VALUE as *const u8;

        let left_pad_cstr = CString::try_from_fmt(fmt!("{:<30p}", ptr))?;
        assert_eq!(left_pad_cstr.to_str()?, expected::PADDED_LEFT);

        let right_pad_cstr = CString::try_from_fmt(fmt!("{:>30p}", ptr))?;
        assert_eq!(right_pad_cstr.to_str()?, expected::PADDED_RIGHT);

        let center_pad_cstr = CString::try_from_fmt(fmt!("{:^30p}", ptr))?;
        assert_eq!(center_pad_cstr.to_str()?, expected::PADDED_CENTER);
        Ok(())
    }

    #[test]
    fn test_pointer_zero_padding() -> Result<(), crate::error::Error> {
        let _guard = NoHashPointersGuard::new(true);
        let ptr = expected::PTR_VALUE as *const u8;

        let zero_pad_cstr = CString::try_from_fmt(fmt!("{:030p}", ptr))?;
        assert_eq!(zero_pad_cstr.to_str()?, expected::ZERO_PADDED);
        Ok(())
    }

    #[test]
    fn test_pointer_fill_char() -> Result<(), crate::error::Error> {
        let _guard = NoHashPointersGuard::new(true);
        let ptr = expected::PTR_VALUE as *const u8;

        let left_fill_cstr = CString::try_from_fmt(fmt!("{:*<30p}", ptr))?;
        assert_eq!(left_fill_cstr.to_str()?, expected::FILLED_LEFT);

        let right_fill_cstr = CString::try_from_fmt(fmt!("{:*>30p}", ptr))?;
        assert_eq!(right_fill_cstr.to_str()?, expected::FILLED_RIGHT);

        let center_fill_cstr = CString::try_from_fmt(fmt!("{:*^30p}", ptr))?;
        assert_eq!(center_fill_cstr.to_str()?, expected::FILLED_CENTER);

        Ok(())
    }

    #[test]
    fn test_reference_pointer() -> Result<(), crate::error::Error> {
        let _guard = NoHashPointersGuard::new(true);

        // Thin Pointer Reference: &u8
        let thin_ptr = expected::PTR_VALUE as *const u8;
        // SAFETY: Does not dereference, only takes address
        let thin_ref = unsafe { &*thin_ptr };
        let thin_ref_cstr = CString::try_from_fmt(fmt!("{:p}", thin_ref))?;
        assert_eq!(thin_ref_cstr.to_str()?, expected::RAW_POINTER);

        // Fat Pointer Reference: &str
        let string_literal = "hello world!";
        let str_ref_cstr = CString::try_from_fmt(fmt!("{:p}", string_literal))?;
        let data_ptr_cstr = CString::try_from_fmt(fmt!("{:p}", string_literal.as_ptr()))?;
        assert_eq!(str_ref_cstr.to_str()?, data_ptr_cstr.to_str()?);

        // Fat Pointer Reference: &[u8]
        let byte_array: &[u8] = b"hello";
        let slice_cstr = CString::try_from_fmt(fmt!("{:p}", byte_array))?;
        let data_ptr_cstr = CString::try_from_fmt(fmt!("{:p}", byte_array.as_ptr()))?;
        assert_eq!(slice_cstr.to_str()?, data_ptr_cstr.to_str()?);

        Ok(())
    }

    #[test]
    fn test_mutable_reference_pointer() -> Result<(), crate::error::Error> {
        let _guard = NoHashPointersGuard::new(true);

        // Thin Pointer Reference: &mut u8
        let thin_ptr = expected::PTR_VALUE as *mut u8;
        // SAFETY: Does not dereference, only takes address
        let mutable_ref = unsafe { &mut *thin_ptr };
        let mut_ref_cstr = CString::try_from_fmt(fmt!("{:p}", mutable_ref))?;
        assert_eq!(mut_ref_cstr.to_str()?, expected::RAW_POINTER);

        // Fat Pointer: &mut [u8]
        let mut byte_array = [1u8, 2, 3];
        let mutable_byte_slice: &mut [u8] = &mut byte_array[..];
        let mut_slice_cstr = CString::try_from_fmt(fmt!("{:p}", mutable_byte_slice))?;
        let data_ptr_cstr = CString::try_from_fmt(fmt!("{:p}", mutable_byte_slice.as_ptr()))?;
        assert_eq!(mut_slice_cstr.to_str()?, data_ptr_cstr.to_str()?);

        Ok(())
    }
}
