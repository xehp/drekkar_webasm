/*
 * Drekkar WebAsm runtime environment
 * http://www.drekkar.com/
 * https://github.com/xehp/drekkar_webasm.git
 * Copyright (C) 2023 Henrik Bjorkman http://www.eit.se/hb
 *
 */
#pragma once

#include <stdint.h>
#include <limits.h>
//#include <ctype.h>

#include "drekkar_core.h"

#define WASI_ESUCCESS        (UINT16_C(0))
#define WASI_E2BIG           (UINT16_C(1))
#define WASI_EACCES          (UINT16_C(2))
#define WASI_EADDRINUSE      (UINT16_C(3))
#define WASI_EADDRNOTAVAIL   (UINT16_C(4))
#define WASI_EAFNOSUPPORT    (UINT16_C(5))
#define WASI_EAGAIN          (UINT16_C(6))
#define WASI_EALREADY        (UINT16_C(7))
#define WASI_EBADF           (UINT16_C(8))
#define WASI_EBADMSG         (UINT16_C(9))
#define WASI_EBUSY           (UINT16_C(10))
#define WASI_ECANCELED       (UINT16_C(11))
#define WASI_ECHILD          (UINT16_C(12))
#define WASI_ECONNABORTED    (UINT16_C(13))
#define WASI_ECONNREFUSED    (UINT16_C(14))
#define WASI_ECONNRESET      (UINT16_C(15))
#define WASI_EDEADLK         (UINT16_C(16))
#define WASI_EDESTADDRREQ    (UINT16_C(17))
#define WASI_EDOM            (UINT16_C(18))
#define WASI_EDQUOT          (UINT16_C(19))
#define WASI_EEXIST          (UINT16_C(20))
#define WASI_EFAULT          (UINT16_C(21))
#define WASI_EFBIG           (UINT16_C(22))
#define WASI_EHOSTUNREACH    (UINT16_C(23))
#define WASI_EIDRM           (UINT16_C(24))
#define WASI_EILSEQ          (UINT16_C(25))
#define WASI_EINPROGRESS     (UINT16_C(26))
#define WASI_EINTR           (UINT16_C(27))
#define WASI_EINVAL          (UINT16_C(28))
#define WASI_EIO             (UINT16_C(29))
#define WASI_EISCONN         (UINT16_C(30))
#define WASI_EISDIR          (UINT16_C(31))
#define WASI_ELOOP           (UINT16_C(32))
#define WASI_EMFILE          (UINT16_C(33))
#define WASI_EMLINK          (UINT16_C(34))
#define WASI_EMSGSIZE        (UINT16_C(35))
#define WASI_EMULTIHOP       (UINT16_C(36))
#define WASI_ENAMETOOLONG    (UINT16_C(37))
#define WASI_ENETDOWN        (UINT16_C(38))
#define WASI_ENETRESET       (UINT16_C(39))
#define WASI_ENETUNREACH     (UINT16_C(40))
#define WASI_ENFILE          (UINT16_C(41))
#define WASI_ENOBUFS         (UINT16_C(42))
#define WASI_ENODEV          (UINT16_C(43))
#define WASI_ENOENT          (UINT16_C(44))
#define WASI_ENOEXEC         (UINT16_C(45))
#define WASI_ENOLCK          (UINT16_C(46))
#define WASI_ENOLINK         (UINT16_C(47))
#define WASI_ENOMEM          (UINT16_C(48))
#define WASI_ENOMSG          (UINT16_C(49))
#define WASI_ENOPROTOOPT     (UINT16_C(50))
#define WASI_ENOSPC          (UINT16_C(51))
#define WASI_ENOSYS          (UINT16_C(52))
#define WASI_ENOTCONN        (UINT16_C(53))
#define WASI_ENOTDIR         (UINT16_C(54))
#define WASI_ENOTEMPTY       (UINT16_C(55))
#define WASI_ENOTRECOVERABLE (UINT16_C(56))
#define WASI_ENOTSOCK        (UINT16_C(57))
#define WASI_ENOTSUP         (UINT16_C(58))
#define WASI_ENOTTY          (UINT16_C(59))
#define WASI_ENXIO           (UINT16_C(60))
#define WASI_EOVERFLOW       (UINT16_C(61))
#define WASI_EOWNERDEAD      (UINT16_C(62))
#define WASI_EPERM           (UINT16_C(63))
#define WASI_EPIPE           (UINT16_C(64))
#define WASI_EPROTO          (UINT16_C(65))
#define WASI_EPROTONOSUPPORT (UINT16_C(66))
#define WASI_EPROTOTYPE      (UINT16_C(67))
#define WASI_ERANGE          (UINT16_C(68))
#define WASI_EROFS           (UINT16_C(69))
#define WASI_ESPIPE          (UINT16_C(70))
#define WASI_ESRCH           (UINT16_C(71))
#define WASI_ESTALE          (UINT16_C(72))
#define WASI_ETIMEDOUT       (UINT16_C(73))
#define WASI_ETXTBSY         (UINT16_C(74))
#define WASI_EXDEV           (UINT16_C(75))
#define WASI_ENOTCAPABLE     (UINT16_C(76))


typedef struct wa_env_type wa_env_type;

struct wa_env_type
{
	char file_name[PATH_MAX];
	drekkar_linear_storage_8_type bytes;
	drekkar_wa_prog *p;
	drekkar_wa_data *d;
};



//void* ewasi_find(const char* sym);
long wa_env_init(wa_env_type *e);
long wa_env_tick(wa_env_type *e);
void wa_env_deinit(wa_env_type *);
