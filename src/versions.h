/* SPDX-License-Identifier: Apache-2.0 */

/*
 * Copyright (c) 2024-2025 Man Hung-Coeng <udc577@126.com>
 * All rights reserved.
 *
 * V0.1.0:
 * * 01. Support generating register graphical tables from configuration file.
 * * 02. Support conversion between graphical tables and text box.
 * * 03. Support associated updates among widgets of each graphical table.
 *
 * V0.1.1:
 * * 01. Add a new description type "missing" to support the case that
 * *     the official doesn't provide any info.
 * * 02. Fix the error of getting default value for items
 *       that reference configurations of other registers.
 *
 * V0.1.2:
 * * 01. Fix the error of synchronizing the "Others" option
 *       of Description pull-down list.
 *
 * V0.1.3:
 * * 01. Fix the error of displaying table title for items
 *       that reference configurations of other registers.
 * * 02. Add Qt 6 compatibility.
 */

#ifndef __VERSIONS_H__
#define __VERSIONS_H__

#ifdef __cplusplus
extern "C" {
#endif

#undef ___stringify
#define ___stringify(x)                 #x
#undef __stringify
#define __stringify(x)                  ___stringify(x)

#ifndef MAJOR_VER
#define MAJOR_VER                       0
#endif

#ifndef MINOR_VER
#define MINOR_VER                       1
#endif

#ifndef PATCH_VER
#define PATCH_VER                       3
#endif

#ifndef PRODUCT_VERSION
#define PRODUCT_VERSION                 __stringify(MAJOR_VER) "." __stringify(MINOR_VER) "." __stringify(PATCH_VER)
#endif

#ifndef __VER__
#define __VER__                         "<none>"
#endif

#ifndef FULL_VERSION
#define FULL_VERSION()                  (__VER__[0] ? (PRODUCT_VERSION "." __VER__) : (PRODUCT_VERSION))
#endif

#ifdef __cplusplus
}
#endif

#endif /* #ifndef __VERSIONS_H__ */

/*
 * ================
 *   CHANGE LOG
 * ================
 *
 * >>> 2024-09-09, Man Hung-Coeng <udc577@126.com>:
 *  01. Initial commit.
 *
 * >>> 2024-10-07, Man Hung-Coeng <udc577@126.com>:
 *  01. Version 0.1.0.
 *
 * >>> 2024-10-09, Man Hung-Coeng <udc577@126.com>:
 *  01. Version 0.1.1.
 *
 * >>> 2024-10-15, Man Hung-Coeng <udc577@126.com>:
 *  01. Version 0.1.2.
 *
 * >>> 2025-04-08, Man Hung-Coeng <udc577@126.com>:
 *  01. Version 0.1.3.
 */

