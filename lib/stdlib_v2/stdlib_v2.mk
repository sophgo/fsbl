#
# Copyright (c) 2016-2017, ARM Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

STDLIB_SRCS_v2	:=	$(addprefix lib/stdlib_v2/,	\
			abort.c				\
			assert.c			\
			exit.c				\
			mem.c				\
			printf.c			\
			putchar.c			\
			puts.c				\
			sscanf.c			\
			strchr.c			\
			strcmp.c			\
			strlen.c			\
			strncmp.c			\
			strnlen.c			\
			subr_prf.c			\
			memcpy.S			\
			timingsafe_bcmp.c)

INCLUDES	+=	-Iinclude/lib/stdlib_v2		\
			-Iinclude/lib/stdlib_v2/sys
