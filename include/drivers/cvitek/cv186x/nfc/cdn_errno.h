 /*
 * Copyright (c) 2018, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef INCLUDE_CDN_ERRNO_H
#define INCLUDE_CDN_ERRNO_H

#ifndef ERRNO_H_NOT_PRESENT
#include <errno.h>
#endif //ERRNO_H_NOT_PRESENT

#ifndef EOK
#define EOK             0      /* no error */
#endif //EOK

#ifndef EPERM
#define EPERM           1      /* Operation not permitted */
#endif //EPERM

#ifndef ENOENT
#define ENOENT          2      /* No such file or directory */
#endif //ENOENT

#ifndef EIO
#define EIO             5      /* I/O error */
#endif //EIO

#ifndef ENOEXEC
#define ENOEXEC         8      /* Exec format error */
#endif //ENOEXEC

#ifndef EAGAIN
#define EAGAIN          11      /* Try again */
#endif //EAGAIN

#ifndef ENOMEM
#define ENOMEM          12      /* Out of memory */
#endif //ENOMEM

#ifndef EFAULT
#define EFAULT          14      /* Bad address */
#endif //EFAULT

#ifndef EBUSY
#define EBUSY           16      /* Device or resource busy */
#endif //EBUSY

#ifndef EINVAL
#define EINVAL          22      /* Invalid argument */
#endif //EINVAL

#ifndef ENOSPC
#define ENOSPC          28      /* No space left on device */
#endif //ENOSPC

#ifndef EBADSLT
#define EBADSLT         57      /* Invalid slot */
#endif //EBADSLT

#ifndef EPROTO
#define EPROTO          71      /* Protocol error */
#endif //EPROTO

#ifndef EOVERFLOW
#define EOVERFLOW       75      /* Value too large for defined data type */
#endif //EOVERFLOW

#ifndef EOPNOTSUPP
#define EOPNOTSUPP      95      /* Operation not supported */
#endif //EOPNOTSUPP

#ifndef EINPROGRESS
#define EINPROGRESS     115     /* Operation now in progress */
#endif //EINPROGRESS

#ifndef EDQUOT
#define EDQUOT          122     /* Quota exceeded */
#endif //EDQUOT

#ifndef ENOTSUP
#define ENOTSUP         EOPNOTSUPP
#endif //ENOTSUP

#ifndef ECANCELED
#define ECANCELED       126      /* Cancelled */
#endif //ECANCELED

#endif //INCLUDE_CDN_ERRNO_H
