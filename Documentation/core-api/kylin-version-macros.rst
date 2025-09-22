.. SPDX-License-Identifier: GPL-2.0

==========================================
KYLIN Version Checking Macros
==========================================

.. contents:: :depth: 2

Introduction
============

The KYLIN version checking macros provide a comprehensive set of compile-time and
runtime version comparison facilities for the KYLIN kernel. These macros enable
developers to write version-aware code that can adapt to different KYLIN kernel
releases while maintaining optimal performance through compile-time optimization.

The macros are automatically generated during the kernel build process and are
available through the standard kernel header ``include/generated/uapi/linux/version.h``.

Architecture Overview
=====================

The version checking system is built around a compact encoding scheme that allows
efficient comparison operations. Version numbers are encoded as single integer
values, enabling both compile-time constant folding and runtime comparison
operations with minimal overhead.

Version Encoding
================

KYLIN versions use a compact binary encoding scheme optimized for efficient
comparison operations:

.. code-block:: text

   Version Code = (Major Version * 256) + Minor Version

This encoding provides:

- Single integer comparison operations
- Efficient compile-time constant evaluation
- Minimal memory footprint
- Backward compatibility with existing version schemes

API Reference
=============

Core Version Macros
-------------------

The following macros provide access to the current kernel version information:

.. c:macro:: KYLIN_VERSION_MAJOR

   Expands to the current major version number of the KYLIN kernel.

   **Type**: Integer constant

   **Example**:

   .. code-block:: c

       #if KYLIN_VERSION_MAJOR >= 11
       /* Code for KYLIN 11.x series */
       #endif

.. c:macro:: KYLIN_VERSION_MINOR

   Expands to the current minor version number of the KYLIN kernel.

   **Type**: Integer constant

   **Example**:

   .. code-block:: c

       pr_info("Running KYLIN %d.%d\n", KYLIN_VERSION_MAJOR, KYLIN_VERSION_MINOR);

.. c:macro:: KYLIN_RELEASE_CODE

   Expands to the encoded version code of the current KYLIN kernel release.

   **Type**: Integer constant

   **Example**:

   .. code-block:: c

       if (KYLIN_RELEASE_CODE >= 2816) { /* KYLIN 11.0 */
               /* Use 11.0+ features */
       }

.. c:macro:: KYLIN_RELEASE_VERSION(major, minor)

   Converts a major and minor version pair into the corresponding version code.

   **Parameters**:

   * ``major`` - Major version number
   * ``minor`` - Minor version number

   **Returns**: Encoded version code

   **Example**:

.. code-block:: c

       #define KYLIN_11_0_CODE KYLIN_RELEASE_VERSION(11, 0)

Version Comparison Macros
-------------------------

The comparison macros provide both compile-time and runtime version checking
capabilities. All macros expand to integer constants that evaluate to 1 (true)
or 0 (false).

.. c:macro:: KYLIN_VERSION_GE(major, minor)

   Checks if the current kernel version is greater than or equal to the specified version.

   **Parameters**:

   * ``major`` - Major version to compare against
   * ``minor`` - Minor version to compare against

   **Returns**: 1 if current version >= specified version, 0 otherwise

   **Example**:

   .. code-block:: c

       #if KYLIN_VERSION_GE(11, 0)
       /* Code for KYLIN 11.0 and later */
       #endif

.. c:macro:: KYLIN_VERSION_GT(major, minor)

   Checks if the current kernel version is greater than the specified version.

   **Parameters**:

   * ``major`` - Major version to compare against
   * ``minor`` - Minor version to compare against

   **Returns**: 1 if current version > specified version, 0 otherwise

.. c:macro:: KYLIN_VERSION_LE(major, minor)

   Checks if the current kernel version is less than or equal to the specified version.

   **Parameters**:

   * ``major`` - Major version to compare against
   * ``minor`` - Minor version to compare against

   **Returns**: 1 if current version <= specified version, 0 otherwise

.. c:macro:: KYLIN_VERSION_LT(major, minor)

   Checks if the current kernel version is less than the specified version.

   **Parameters**:

   * ``major`` - Major version to compare against
   * ``minor`` - Minor version to compare against

   **Returns**: 1 if current version < specified version, 0 otherwise

.. c:macro:: KYLIN_VERSION_EQ(major, minor)

   Checks if the current kernel version equals the specified version.

   **Parameters**:

   * ``major`` - Major version to compare against
   * ``minor`` - Minor version to compare against

   **Returns**: 1 if current version == specified version, 0 otherwise

.. c:macro:: KYLIN_VERSION_RANGE(major1, minor1, major2, minor2)

   Checks if the current kernel version falls within the specified range (inclusive).

   **Parameters**:

   * ``major1`` - Lower bound major version
   * ``minor1`` - Lower bound minor version
   * ``major2`` - Upper bound major version
   * ``minor2`` - Upper bound minor version

   **Returns**: 1 if current version is within range, 0 otherwise

   **Example**:

.. code-block:: c

       #if KYLIN_VERSION_RANGE(10, 5, 11, 5)
       /* Code for KYLIN 10.5 through 11.5 */
       #endif

Programming Guidelines
=======================

Use compile-time version checking for optimal performance and code clarity.
All macros expand to integer constants enabling both compile-time and runtime usage.

Basic Usage
-----------

.. code-block:: c

    #include <linux/version.h>

    /* Feature selection */
    #if KYLIN_VERSION_GE(11, 0)
    static void modern_feature(void) { /* KYLIN 11.0+ implementation */ }
    #else
    static void legacy_feature(void) { /* Fallback implementation */ }
    #endif

    /* API compatibility */
    static int device_operation(struct device *dev)
    {
    #if KYLIN_VERSION_GE(11, 0)
            return dev->modern_op(dev);
    #else
            return legacy_device_operation(dev);
    #endif
    }

    /* Version ranges and multiple tiers */
    #if KYLIN_VERSION_RANGE(10, 5, 11, 0)
    static void transitional_feature(void) { /* 10.5-11.0 implementation */ }
    #endif

    #if KYLIN_VERSION_GE(11, 2)
    static void latest_feature(void) { /* Latest implementation */ }
    #elif KYLIN_VERSION_GE(11, 0)
    static void intermediate_feature(void) { /* Intermediate implementation */ }
    #else
    static void basic_feature(void) { /* Basic implementation */ }
    #endif

Runtime Usage
-------------

Use runtime checks only for debugging, user-space interfaces, or dynamic configuration:

.. code-block:: c

    /* Debug output */
    pr_info("KYLIN version: %d.%d (code: %d)\n",
            KYLIN_VERSION_MAJOR, KYLIN_VERSION_MINOR, KYLIN_RELEASE_CODE);

    /* User-space interface */
    static long device_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
    {
            switch (cmd) {
            case KYLIN_VERSION_QUERY:
                    return put_user(KYLIN_RELEASE_CODE, (int __user *)arg);
        default:
                    return -ENOTTY;
            }
    }

Driver Development Examples
============================

Module Initialization
---------------------

.. code-block:: c

    #include <linux/version.h>
    #include <linux/module.h>

    /* Version requirement enforcement */
    #if KYLIN_VERSION_LT(10, 0)
    #error "This driver requires KYLIN 10.0 or higher"
    #endif

    /* Version-specific implementations */
    #if KYLIN_VERSION_GE(11, 0)
    static int init(void) { return register_new_features(); }
    static void cleanup(void) { unregister_new_features(); }
    #else
    static int init(void) { return register_legacy_features(); }
    static void cleanup(void) { unregister_legacy_features(); }
    #endif

    static int __init my_driver_init(void) { return init(); }
    static void __exit my_driver_exit(void) { cleanup(); }

    module_init(my_driver_init);
    module_exit(my_driver_exit);
    MODULE_LICENSE("GPL");

API Compatibility
-----------------

.. code-block:: c

    /* Multi-version API compatibility */
    static int device_register_compat(struct device *dev)
    {
    #if KYLIN_VERSION_GE(11, 2)
            return device_register_advanced(dev, DEVICE_FLAGS_11_2);
    #elif KYLIN_VERSION_GE(11, 0)
            return device_register_modern(dev, DEVICE_FLAGS_11_0);
    #else
            return device_register_legacy(dev);
    #endif
    }

    /* Conditional structure members */
    struct device_context {
            int common_field;
    #if KYLIN_VERSION_GE(11, 0)
            struct new_api_context *new_ctx;
    #endif
    #if KYLIN_VERSION_GE(11, 2)
            struct advanced_features *advanced;
    #endif
    };

    /* Version-specific features */
    static int process_request(struct device_context *ctx, int request_type)
    {
            switch (request_type) {
            case REQUEST_BASIC:
                    return handle_basic_request(ctx);
    #if KYLIN_VERSION_GE(11, 0)
            case REQUEST_ADVANCED:
                    return handle_advanced_request(ctx);
    #endif
    #if KYLIN_VERSION_GE(11, 2)
            case REQUEST_EXPERIMENTAL:
                    return handle_experimental_request(ctx);
    #endif
            default:
                    return -EINVAL;
            }
    }

Third-Party Driver Compatibility
=================================

For cross-platform drivers, create a compatibility header:

.. code-block:: c

    /*
     * kylin_compat.h - Compatibility header for third-party drivers
     */
    #ifndef __KYLIN_COMPAT_H
    #define __KYLIN_COMPAT_H

    #include <linux/version.h>

    #ifndef CONFIG_KYLIN_KERNEL
        #define KYLIN_VERSION_MAJOR                 0
        #define KYLIN_VERSION_MINOR                 0
        #define KYLIN_RELEASE_CODE                  0
        #define KYLIN_RELEASE_VERSION(a, b)         (((a) << 8) + (b))
        #define KYLIN_VERSION_GE(major, minor)      0
        #define KYLIN_VERSION_GT(major, minor)      0
        #define KYLIN_VERSION_LE(major, minor)      0
        #define KYLIN_VERSION_LT(major, minor)      0
        #define KYLIN_VERSION_EQ(major, minor)      0
        #define KYLIN_VERSION_RANGE(m1, m2, m3, m4) 0
    #endif

    #endif /* __KYLIN_COMPAT_H */

Usage in driver:

.. code-block:: c

    #include <linux/module.h>
    #include "kylin_compat.h"

    /* Version requirement (KYLIN systems only) */
    #if KYLIN_VERSION_LT(10, 0)
    #error "Driver requires KYLIN 10.0 or higher"
    #endif

    /* Version-specific initialization */
    #if KYLIN_VERSION_GE(11, 0)
    static int init(void) { return register_kylin_features(); }
    #else
    static int init(void) { return register_generic_features(); }
    #endif

    static int __init my_driver_init(void) { return init(); }
    module_init(my_driver_init);

Version Code Reference
======================

The following table provides a reference for common KYLIN version codes:

.. list-table::
   :widths: 12 15 25
   :header-rows: 1

   * - Version
     - Version Code
     - Binary Calculation
   * - 10.0
     - 2560
     - 0x0A00 (10 << 8 + 0)
   * - 10.5
     - 2565
     - 0x0A05 (10 << 8 + 5)
   * - 11.0
     - 2816
     - 0x0B00 (11 << 8 + 0)
   * - 11.1
     - 2817
     - 0x0B01 (11 << 8 + 1)
   * - 11.2
     - 2818
     - 0x0B02 (11 << 8 + 2)
   * - 12.0
     - 3072
     - 0x0C00 (12 << 8 + 0)

Best Practices
==============

**Version Design**:

- Use clear version boundaries
- Avoid complex conditions
- Prefer range macros for complex ranges

**Error Handling**:

.. code-block:: c

    /* Compile-time requirement enforcement */
    #if KYLIN_VERSION_LT(10, 0)
    #error "Driver requires KYLIN 10.0 or higher"
    #endif

    /* Runtime debugging */
    static void debug_version_info(void)
    {
            pr_info("Version: %d.%d (code: %d)\n",
                    KYLIN_VERSION_MAJOR, KYLIN_VERSION_MINOR, KYLIN_RELEASE_CODE);
    }

Troubleshooting
===============

**Common Issues**:

1. **Macro not found**: Include ``#include <linux/version.h>``
2. **Build errors on non-KYLIN**: Use compatibility header (see above)
3. **Incorrect comparisons**: Use specific macros, not raw version codes
4. **Performance issues**: Use compile-time checks in hot paths

**Debugging**:

.. code-block:: c

    static void dump_version_info(void)
    {
            pr_info("Version: %d.%d (code: %d)\n",
                    KYLIN_VERSION_MAJOR, KYLIN_VERSION_MINOR, KYLIN_RELEASE_CODE);
            pr_info("GE(11,0): %d, RANGE(10,5,11,5): %d\n",
                    KYLIN_VERSION_GE(11, 0), KYLIN_VERSION_RANGE(10, 5, 11, 5));
    }

Implementation Details
======================

The KYLIN version checking macros are automatically generated during the kernel
build process. The implementation follows the standard Linux kernel build system
patterns.

Build System Integration
------------------------

The version macros are generated by the kernel build system using the following
process:

1. **Version Source**: Version information is read from ``Makefile.kylin``
   containing ``KYLIN_VERSION_MAJOR`` and ``KYLIN_VERSION_MINOR``

2. **Code Generation**: The build system calculates ``KYLIN_RELEASE_CODE`` using
   the encoding formula and generates all comparison macros

3. **Header Generation**: All macros are written to
   ``include/generated/uapi/linux/version.h``

4. **Build Integration**: The generated header is automatically included in the
   kernel build process

Generated Macro Structure
-------------------------

The generated macros follow this pattern:

.. code-block:: c

    /* Core version information */
    #define KYLIN_VERSION_MAJOR             11
    #define KYLIN_VERSION_MINOR             2
    #define KYLIN_RELEASE_CODE              2818

    /* Comparison macros */
    #define KYLIN_VERSION_GE(major, minor) \
            (KYLIN_RELEASE_CODE >= KYLIN_RELEASE_VERSION(major, minor))
    #define KYLIN_VERSION_GT(major, minor) \
            (KYLIN_RELEASE_CODE > KYLIN_RELEASE_VERSION(major, minor))
    /* ... other comparison macros ... */

Migration Guide
===============

From Manual Version Checking
----------------------------

**Legacy Approach**: Manual major/minor comparison

.. code-block:: c

    /* Old approach: Manual version checking */
    if (KYLIN_VERSION_MAJOR > 11 ||
        (KYLIN_VERSION_MAJOR == 11 && KYLIN_VERSION_MINOR >= 2)) {
        /* Use new feature */
    }

**Modern Approach**: Use convenience macros

.. code-block:: c

    /* New approach: Use version comparison macros */
    #if KYLIN_VERSION_GE(11, 2)
    /* Use new feature */
    #endif

**Benefits of Migration**:

- Cleaner, more readable code
- Reduced chance of errors
- Better compiler optimization
- Consistent version checking patterns

Adding Version Dependencies
---------------------------

When adding new version-dependent features:

1. **Define Clear Requirements**: Specify exact version requirements
2. **Use Appropriate Macros**: Choose the right comparison macro for your needs
3. **Test Across Versions**: Verify behavior on different KYLIN versions
4. **Document Dependencies**: Clearly document version requirements in code
5. **Provide Fallbacks**: Always provide fallback implementations for older versions

Summary
=======

The KYLIN version checking macros provide a comprehensive solution for writing
version-aware kernel code. Key takeaways:

**For KYLIN Kernel Development**:

- Use compile-time version checking for optimal performance
- Leverage the full range of comparison macros for precise version control
- Implement proper error handling with compile-time checks

**For Third-Party Driver Development**:

- Use the compatibility header approach for cross-platform support
- Always provide fallback implementations for non-KYLIN systems
- Test thoroughly across different kernel versions

**Performance Considerations**:

- Compile-time checks provide zero runtime overhead
- Runtime checks should be limited to debugging and user-space interfaces
- Choose the appropriate checking method based on your use case

**Best Practices**:

- Prefer compile-time version checking over runtime checking
- Use clear version boundaries and avoid complex conditions
- Document version dependencies clearly
- Provide graceful fallbacks for older versions

The KYLIN version checking system enables developers to write robust, efficient,
and maintainable kernel code that adapts to different KYLIN kernel releases
while maintaining optimal performance characteristics.
