/*
 * This file is part of the libserialport project.
 *
 * Copyright (C) 2014 Martin Ling <martin-libserialport@earth.li>
 * Copyright (C) 2014 Aurelien Jacobs <aurel@gnuage.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIBSERIALPORT_LIBSERIALPORT_INTERNAL_H
#define LIBSERIALPORT_LIBSERIALPORT_INTERNAL_H

/* These MSVC-specific defines must appear before other headers.*/
#ifdef _MSC_VER
#define _CRT_NONSTDC_NO_DEPRECATE
//#define _CRT_SECURE_NO_WARNINGS
#endif


/* These feature test macros must appear before other headers.*/
#if defined(__linux__) || defined(__CYGWIN__)
/* For timeradd, timersub, timercmp, realpath. */
#define _BSD_SOURCE 1 /* for glibc < 2.19 */
#define _DEFAULT_SOURCE 1 /* for glibc >= 2.20 */
/* For clock_gettime and associated types. */
#define _POSIX_C_SOURCE 199309L
#endif

#ifdef LIBSERIALPORT_ATBUILD
/* If building with autoconf, include the generated config.h. */
#include <config.h>
#endif

#if 1//def LIBSERIALPORT_MSBUILD
/* If building with MS tools, define necessary things that
   would otherwise appear in config.h. */
#define SP_PRIV
#endif

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#ifdef _WIN32
#include <windows.h>
#include <tchar.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#undef DEFINE_GUID
#define DEFINE_GUID(name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
	static const GUID name = { l,w1,w2,{ b1,b2,b3,b4,b5,b6,b7,b8 } }
#include <usbioctl.h>
#include <usbiodef.h>
/* The largest size that can be passed to WriteFile() safely
 * on any architecture. This arises from the expression:
 * PAGE_SIZE * (65535 - sizeof(MDL)) / sizeof(ULONG_PTR)
 * and this worst-case value is found on x64. */
#define WRITEFILE_MAX_SIZE 33525760
#else
#include <limits.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <time.h>
#include <poll.h>
#include <unistd.h>
#endif
#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/serial/IOSerialKeys.h>
#include <IOKit/serial/ioss.h>
#include <sys/syslimits.h>
#include <mach/mach_time.h>
#endif
#ifdef __linux__
#include <dirent.h>
/* Android only has linux/serial.h from platform 21 onwards. */
#if !(defined(__ANDROID__) && (__ANDROID_API__ < 21))
#include <linux/serial.h>
#endif
#include "linux_termios.h"

/* TCGETX/TCSETX is not available everywhere. */
#if defined(TCGETX) && defined(TCSETX) && defined(HAVE_STRUCT_TERMIOX)
#define USE_TERMIOX
#endif
#endif

/* TIOCINQ/TIOCOUTQ is not available everywhere. */
#if !defined(TIOCINQ) && defined(FIONREAD)
#define TIOCINQ FIONREAD
#endif
#if !defined(TIOCOUTQ) && defined(FIONWRITE)
#define TIOCOUTQ FIONWRITE
#endif

/*
 * O_CLOEXEC is not available everywhere, fallback to not setting the
 * flag on those systems.
 */
#ifndef _WIN32
#ifndef O_CLOEXEC
#define O_CLOEXEC 0
#endif
#endif

/* Non-standard baudrates are not available everywhere. */
#if (defined(HAVE_TERMIOS_SPEED) || defined(HAVE_TERMIOS2_SPEED)) && HAVE_DECL_BOTHER
#define USE_TERMIOS_SPEED
#endif


#ifdef _WIN32
typedef HANDLE event_handle;
#else
typedef int event_handle;
#endif

/* Standard baud rates. */
#ifdef _WIN32
#define BAUD_TYPE DWORD
#define BAUD(n) {CBR_##n, n}
#else
#define BAUD_TYPE speed_t
#define BAUD(n) {B##n, n}
#endif

typedef struct std_baudrate {
	BAUD_TYPE index;
	int value;
}std_baudrate_t;

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

extern void (*sp_debug_handler)(const char *format, ...);

/* Debug output macros. */
#define DEBUG_FMT(fmt, ...) do { \
	if (sp_debug_handler) \
		sp_debug_handler(fmt ".\n", __VA_ARGS__); \
} while (0)
#define DEBUG(msg) DEBUG_FMT(msg, NULL)
#define DEBUG_ERROR(err, msg) DEBUG_FMT("%s returning " #err ": " msg, __func__)
#define DEBUG_FAIL(msg) do {               \
	char *errmsg = sp_last_error_message(); \
	DEBUG_FMT("%s returning SP_ERR_FAIL: " msg ": %s", __func__, errmsg); \
	sp_free_error_message(errmsg); \
} while (0);
#define RETURN() do { \
	DEBUG_FMT("%s returning", __func__); \
	return; \
} while (0)
#define RETURN_CODE(x) do { \
	DEBUG_FMT("%s returning " #x, __func__); \
	return x; \
} while (0)
#define RETURN_CODEVAL(x) do { \
	switch (x) { \
	case SP_OK: RETURN_CODE(SP_OK); \
	case SP_ERR_ARG: RETURN_CODE(SP_ERR_ARG); \
	case SP_ERR_FAIL: RETURN_CODE(SP_ERR_FAIL); \
	case SP_ERR_MEM: RETURN_CODE(SP_ERR_MEM); \
	case SP_ERR_SUPP: RETURN_CODE(SP_ERR_SUPP); \
	default: RETURN_CODE(SP_ERR_FAIL); \
	} \
} while (0)
#define RETURN_OK() RETURN_CODE(SP_OK);
#define RETURN_ERROR(err, msg) do { \
	DEBUG_ERROR(err, msg); \
	return err; \
} while (0)
#define RETURN_FAIL(msg) do { \
	DEBUG_FAIL(msg); \
	return SP_ERR_FAIL; \
} while (0)
#define RETURN_INT(x) do { \
	int _x = x; \
	DEBUG_FMT("%s returning %d", __func__, _x); \
	return _x; \
} while (0)
#define RETURN_STRING(x) do { \
	char *_x = x; \
	DEBUG_FMT("%s returning %s", __func__, _x); \
	return _x; \
} while (0)
#define RETURN_POINTER(x) do { \
	void *_x = x; \
	DEBUG_FMT("%s returning %p", __func__, _x); \
	return _x; \
} while (0)
#define SET_ERROR(val, err, msg) do { DEBUG_ERROR(err, msg); val = err; } while (0)
#define SET_FAIL(val, msg) do { DEBUG_FAIL(msg); val = SP_ERR_FAIL; } while (0)
#define TRACE(fmt, ...) DEBUG_FMT("%s(" fmt ") called", __func__, __VA_ARGS__)
#define TRACE_VOID() DEBUG_FMT("%s() called", __func__)

#define TRY(x) do { int retval = x; if (retval != SP_OK) RETURN_CODEVAL(retval); } while (0)

/* Timing abstraction */

struct time {
#ifdef _WIN32
	int64_t ticks;
#else
	struct timeval tv;
#endif
};

typedef struct {
	unsigned int ms, limit_ms;
	struct time start, now, end, delta, delta_max;
	struct timeval delta_tv;
	bool calls_started, overflow;
}timeout_t;

SP_PRIV void time_get(struct time *time);
SP_PRIV void time_set_ms(struct time *time, unsigned int ms);
SP_PRIV void time_add(const struct time *a, const struct time *b, struct time *result);
SP_PRIV void time_sub(const struct time *a, const struct time *b, struct time *result);
SP_PRIV bool time_greater(const struct time *a, const struct time *b);
SP_PRIV void time_as_timeval(const struct time *time, struct timeval *tv);
SP_PRIV unsigned int time_as_ms(const struct time *time);
SP_PRIV void timeout_start(timeout_t *timeout, unsigned int timeout_ms);
SP_PRIV void timeout_limit(timeout_t *timeout, unsigned int limit_ms);
SP_PRIV bool timeout_check(timeout_t *timeout);
SP_PRIV void timeout_update(timeout_t *timeout);
SP_PRIV struct timeval *timeout_timeval(timeout_t *timeout);
SP_PRIV unsigned int timeout_remaining_ms(timeout_t *timeout);

#endif
