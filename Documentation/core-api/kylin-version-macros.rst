.. SPDX-License-Identifier: GPL-2.0

==========================================
KYLIN Version Checking Macros
==========================================

Overview
========

This document describes the KYLIN version checking macros that provide compile-time
and runtime version comparison capabilities for the KYLIN kernel. These macros are
automatically generated during the kernel build process and are available in
``include/generated/uapi/linux/version.h``.

Version Encoding Scheme
=======================

KYLIN versions use a compact encoding scheme for efficient comparison:

.. code-block:: text

    Version Code = (Major Version * 256) + Minor Version

Available Macros
================

Core Version Information
------------------------

.. code-block:: c

    #define KYLIN_VERSION_MAJOR              // Current major version number
    #define KYLIN_VERSION_MINOR              // Current minor version number
    #define KYLIN_RELEASE_CODE               // Current version code
    #define KYLIN_RELEASE_VERSION(a, b)      // Convert version to code: ((a) << 8) + (b)

Version Comparison Macros
-------------------------

.. code-block:: c

    #define KYLIN_VERSION_GE(major, minor)   // Greater than or equal to
    #define KYLIN_VERSION_GT(major, minor)   // Greater than
    #define KYLIN_VERSION_LE(major, minor)   // Less than or equal to
    #define KYLIN_VERSION_LT(major, minor)   // Less than
    #define KYLIN_VERSION_EQ(major, minor)   // Equal to
    #define KYLIN_VERSION_RANGE(major1, minor1, major2, minor2)  // Version range check

Usage Examples
==============

Runtime Version Checking
------------------------

.. code-block:: c

    #include <linux/version.h>

    void check_kylin_version(void)
    {
        /* Check if current version is 11.0 or higher */
        if (KYLIN_VERSION_GE(11, 0)) {
            pr_info("KYLIN 11.0+ features available\n");
        }

        /* Check version range */
        if (KYLIN_VERSION_RANGE(10, 5, 11, 5)) {
            pr_info("Version is within supported range 10.5-11.5\n");
        }

        /* Print current version information */
        pr_info("Current KYLIN version: %d.%d (code: %d)\n",
                KYLIN_VERSION_MAJOR, KYLIN_VERSION_MINOR, KYLIN_RELEASE_CODE);
    }

Compile-Time Conditional Compilation
------------------------------------

.. code-block:: c

    /* Conditional compilation based on version */
    #if KYLIN_VERSION_GE(11, 0)
        /* Code compiled only for KYLIN 11.0+ */
        static void kylin_11_0_feature(void)
        {
            /* New feature implementation */
        }
    #endif

    #if KYLIN_VERSION_RANGE(10, 5, 11, 0)
        /* Code compiled only for versions 10.5-11.0 */
        static void legacy_compatibility_feature(void)
        {
            /* Compatibility implementation */
        }
    #endif

Feature Availability Checking
-----------------------------

.. code-block:: c

    bool is_feature_supported(int feature_id)
    {
        switch (feature_id) {
        case FEATURE_A:
            return KYLIN_VERSION_GE(11, 0);  /* Requires 11.0+ */
        case FEATURE_B:
            return KYLIN_VERSION_GE(11, 2);  /* Requires 11.2+ */
        case FEATURE_C:
            return KYLIN_VERSION_RANGE(10, 5, 11, 5);  /* Requires 10.5-11.5 */
        default:
            return false;
        }
    }

Driver Module Implementation
----------------------------

.. code-block:: c

    #include <linux/version.h>
    #include <linux/module.h>

    static int __init my_driver_init(void)
    {
        /* Check minimum version requirement */
        if (KYLIN_VERSION_LT(10, 0)) {
            pr_err("This driver requires KYLIN 10.0 or higher\n");
            return -ENOTSUPP;
        }

        /* Select implementation based on version */
        if (KYLIN_VERSION_GE(11, 0)) {
            pr_info("Using KYLIN 11.0+ optimized implementation\n");
            /* New version implementation */
        } else {
            pr_info("Using compatibility implementation\n");
            /* Compatibility implementation */
        }

        return 0;
    }

    module_init(my_driver_init);

API Compatibility Layer
-----------------------

.. code-block:: c

    /* API compatibility layer example */
    static inline int kylin_specific_api_call(void)
    {
    #if KYLIN_VERSION_GE(11, 2)
        /* Use new API for 11.2+ */
        return new_api_function();
    #else
        /* Use legacy API for older versions */
        return legacy_api_function();
    #endif
    }

Version Code Reference
========================

.. list-table::
   :widths: 10 15 20
   :header-rows: 1

   * - Version
     - Version Code
     - Calculation
   * - 10.0
     - 2560
     - 10\*256+0
   * - 10.5
     - 2565
     - 10\*256+5
   * - 11.0
     - 2816
     - 11\*256+0
   * - 11.1
     - 2817
     - 11\*256+1
   * - 11.2
     - 2818
     - 11\*256+2
   * - 12.0
     - 3072
     - 12\*256+0

Best Practices
==============

Compile-Time vs Runtime Checks
------------------------------

- **Use ``#if`` for compile-time checks**: When you need different code paths
  that cannot coexist
- **Use ``if`` for runtime checks**: When you need dynamic behavior based on
  version

Version Design
--------------------

.. code-block:: c

    /* Good: Clear version requirements */
    if (KYLIN_VERSION_GE(11, 0)) {
        /* Feature available from 11.0 onwards */
    }

    /* Good: Specific version range */
    if (KYLIN_VERSION_RANGE(10, 5, 11, 5)) {
        /* Feature available in specific range */
    }

    /* Avoid: Overly complex conditions */
    if (KYLIN_VERSION_GE(10, 0) && KYLIN_VERSION_LT(12, 0) && !KYLIN_VERSION_EQ(11, 3)) {
        /* Too complex - consider using KYLIN_VERSION_RANGE */
    }

Error Handling
--------------

.. code-block:: c

    static int check_version_compatibility(void)
    {
        if (KYLIN_VERSION_LT(MIN_REQUIRED_MAJOR, MIN_REQUIRED_MINOR)) {
            pr_err("Incompatible KYLIN version: %d.%d (required: %d.%d+)\n",
                   KYLIN_VERSION_MAJOR, KYLIN_VERSION_MINOR,
                   MIN_REQUIRED_MAJOR, MIN_REQUIRED_MINOR);
            return -ENOTSUPP;
        }

        return 0;
    }

Performance Considerations
==========================

- **Zero runtime overhead**: All macros expand to simple integer comparisons
- **Compile-time optimization**: Unreachable code paths are eliminated by the
  compiler
- **Cache-friendly**: Version codes fit in a single integer, enabling efficient
  comparisons

Implementation Details
======================

The version macros are generated during the kernel build process in the
``filechk_version.h`` function of the main Makefile. The generation process:

1. Reads ``KYLIN_VERSION_MAJOR`` and ``KYLIN_VERSION_MINOR`` from
   ``Makefile.kylin``
2. Calculates ``KYLIN_RELEASE_CODE`` using the encoding formula
3. Generates all comparison macros in ``include/generated/uapi/linux/version.h``

Migration Guide
===============

From Manual Version Checking
----------------------------

.. code-block:: c

    /* Old approach 1: Manual major/minor comparison */
    if (KYLIN_VERSION_MAJOR > 11 ||
        (KYLIN_VERSION_MAJOR == 11 && KYLIN_VERSION_MINOR >= 2)) {
        /* Use new feature */
    }

    /* Old approach 2: Direct version code comparison */
    if (KYLIN_RELEASE_CODE >= KYLIN_RELEASE_VERSION(11, 2)) {
        /* Use new feature */
    }

    /* New approach: Use convenience macros */
    if (KYLIN_VERSION_GE(11, 2)) {
        /* Use new feature */
    }

Adding New Version Checks
-------------------------

1. **Define version requirements** in your module/driver
2. **Use appropriate macro** for the comparison type
3. **Test with different versions** to ensure correctness
4. **Document version dependencies** in your code

Troubleshooting
===============

Common Issues
-------------

1. **Macro not found**: Ensure ``#include <linux/version.h>`` is present
2. **Incorrect comparisons**: Verify version code calculations
3. **Build errors**: Check that version macros are properly generated

Debug Information
-----------------

.. code-block:: c

    /* Print version information for debugging */
    pr_info("KYLIN Version Debug Info:\n");
    pr_info("  Major: %d\n", KYLIN_VERSION_MAJOR);
    pr_info("  Minor: %d\n", KYLIN_VERSION_MINOR);
    pr_info("  Code: %d\n", KYLIN_RELEASE_CODE);
    pr_info("  GE(11,0): %d\n", KYLIN_VERSION_GE(11, 0));
    pr_info("  RANGE(10,5,11,5): %d\n", KYLIN_VERSION_RANGE(10, 5, 11, 5));

Cross-Platform Compatibility
============================

Third-Party Driver Support
--------------------------

When developing third-party drivers that need to work across different operating
systems, you may encounter compilation warnings about undefined KYLIN macros on
non-KYLIN systems. Here are several approaches to handle this:

Method 1: Conditional Compilation with Fallback
-----------------------------------------------

.. code-block:: c

    #include <linux/version.h>

    /* Define fallback macros for non-KYLIN systems */
    #ifndef CONFIG_KYLIN_KERNEL
        #define KYLIN_VERSION_MAJOR            0
        #define KYLIN_VERSION_MINOR            0
        #define KYLIN_RELEASE_CODE             0
        #define KYLIN_RELEASE_VERSION(a, b)    (((a) << 8) + (b))
        #define KYLIN_VERSION_GE(major, minor) 0
        #define KYLIN_VERSION_GT(major, minor) 0
        #define KYLIN_VERSION_LE(major, minor) 0
        #define KYLIN_VERSION_LT(major, minor) 0
        #define KYLIN_VERSION_EQ(major, minor) 0
        #define KYLIN_VERSION_RANGE(major1, minor1, major2, minor2) 0
    #endif

    /* Use KYLIN-specific features only when available */
    static void my_driver_feature(void)
    {
        if (KYLIN_VERSION_GE(11, 0)) {
            /* KYLIN 11.0+ specific implementation */
            pr_info("Using KYLIN 11.0+ features\n");
        } else {
            /* Generic implementation for other systems */
            pr_info("Using generic implementation\n");
        }
    }

Method 2: Header File Wrapper
-----------------------------

Create a header file ``kylin_compat.h`` for your driver:

.. code-block:: c

    /* kylin_compat.h - KYLIN compatibility header */
    #ifndef __KYLIN_COMPAT_H
    #define __KYLIN_COMPAT_H

    #include <linux/version.h>

    #ifndef CONFIG_KYLIN_KERNEL
        /* Fallback for non-KYLIN systems */
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

Then use it in your driver:

.. code-block:: c

    #include "kylin_compat.h"

    static void my_driver_feature(void)
    {
        if (KYLIN_VERSION_GE(11, 0)) {
            /* This will work on KYLIN 11.0+ and be safely ignored on other systems */
            pr_info("KYLIN 11.0+ feature enabled\n");
        }
    }

Best Practices for Third-Party Drivers
--------------------------------------

1. **Always provide fallbacks**: Ensure your driver works on non-KYLIN systems
2. **Use feature detection**: Check for macro availability before using
3. **Document dependencies**: Clearly state KYLIN version requirements
4. **Test on multiple systems**: Verify compatibility across different OS versions
5. **Graceful degradation**: Provide alternative implementations when KYLIN features are unavailable

Example Driver Template
-----------------------

First, create a compatibility header file ``kylin_compat.h``:

.. code-block:: c

    /*
     * kylin_compat.h - KYLIN compatibility header for third-party drivers
     */
    #ifndef __KYLIN_COMPAT_H
    #define __KYLIN_COMPAT_H

    #include <linux/version.h>

    #ifndef CONFIG_KYLIN_KERNEL
        /* Fallback macros for non-KYLIN systems */
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

Then use it in your driver:

.. code-block:: c

    /*
     * Example third-party driver with KYLIN compatibility
     */
    #include <linux/module.h>
    #include "kylin_compat.h"

    static int __init my_driver_init(void)
    {
        /* Check system compatibility */
        #ifdef CONFIG_KYLIN_KERNEL
        if (KYLIN_VERSION_LT(10, 0)) {
            pr_err("This driver requires KYLIN 10.0 or higher\n");
            return -ENOTSUPP;
        }
        pr_info("Running on KYLIN %d.%d\n",
                KYLIN_VERSION_MAJOR, KYLIN_VERSION_MINOR);
        #else
        pr_info("Running on non-KYLIN system\n");
        #endif

        /* Initialize driver features */
        if (KYLIN_VERSION_GE(11, 0)) {
            /* Use KYLIN 11.0+ optimized features */
            pr_info("Using KYLIN 11.0+ optimizations\n");
        } else {
            /* Use generic implementation */
            pr_info("Using generic implementation\n");
        }

        return 0;
    }

    static void __exit my_driver_exit(void)
    {
        pr_info("Driver unloaded\n");
    }

    module_init(my_driver_init);
    module_exit(my_driver_exit);

    MODULE_LICENSE("GPL");
    MODULE_DESCRIPTION("Example driver with KYLIN compatibility");
    MODULE_AUTHOR("Your Name");

Future Extensions
=================

Potential enhancements for future versions:

1. **Semantic versioning support**: Handle patch versions and pre-release
   identifiers
2. **Version compatibility matrix**: Predefined compatibility rules
3. **Version upgrade validation**: Check upgrade path validity
4. **Extended range operators**: Support for open-ended ranges and exclusions
