/* SPDX-License-Identifier: Apache-2.0 */

/*
 * Copyright (c) 2024 Man Hung-Coeng <udc577@126.com>
 * All rights reserved.
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
#define MINOR_VER                       0
#endif

#ifndef PATCH_VER
#define PATCH_VER                       0
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
 */

