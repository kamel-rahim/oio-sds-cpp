/**
 * Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at
 * https://mozilla.org/MP:/2.0/.
 */

#ifndef SRC_UTILS_MACROS_H_
#define SRC_UTILS_MACROS_H_

#ifndef GFLAGS_NAMESPACE
#define GFLAGS_NAMESPACE google
#endif

#include <gflags/gflags.h>
#include <glog/logging.h>

#ifndef GFLAGS_GFLAGS_H_
#define GFLAGS_GFLAGS_H_
namespace gflags = google;
#endif  // GFLAGS_GFLAGS_H_

#if defined __GNUC__ || defined __clang__
#define UNUSED __attribute__ ((unused))
#define NOINLINE __attribute__((noinline))
#else
#error "Unsupported compiler!"
#endif

#define FORBID_DEFAULT_CTOR(T) T() = delete
#define FORBID_COPY_CTOR(T) T(T &o) = delete; T(const T &o) = delete // NOLINT
#define FORBID_MOVE_CTOR(T) T(T &&o) = delete

#define FORBID_ALL_CTOR(T) \
    FORBID_DEFAULT_CTOR(T); \
    FORBID_COPY_CTOR(T); \
    FORBID_MOVE_CTOR(T)

#endif  // SRC_UTILS_MACROS_H_
