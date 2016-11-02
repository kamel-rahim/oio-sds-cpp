/**
 * This file is part of the OpenIO client libraries
 * Copyright (C) 2016 OpenIO SAS
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3.0 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.
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
