/*
 * Copyright (C) Tildeslash Ltd. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 *
 * You must obey the GNU Affero General Public License in all respects
 * for all of the code used other than OpenSSL.  
 */


#ifndef CONFIG_INCLUDED
#define CONFIG_INCLUDED


/**
 * Global defines, macros and types
 *
 * @author http://www.tildeslash.com/
 * @see http://www.mmonit.com/
 * @file
 */


#include "xconfig.h"

#include <errno.h>
#include <limits.h>
#include <string.h>
#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>

#include "assert.h"
#include "system/Mem.h"


/* ----------------------------------- Error, Exceptions and report macros */


/**
 * The standard abort routine
 */
#define ABORT	System_abort


/**
 * The standard error routine
 */
#define ERROR   System_error


/**
 * The standard debug routine
 */
#define DEBUG   System_debug


/* ------------------------------------------------------------ Exceptions */


#include "AssertException.h"
#include "IOException.h"
#include "NumberFormatException.h"


/* ------------------------------------------ General Purpose value macros */


/**
 * Standard String length
 */
#define STRLEN 256


/* ------------------------------------------------------ Type definitions */


/**
 * The internal 8-bit char type
 */
#ifndef HAVE_UCHAR_T
typedef unsigned char uchar_t;
#endif


/* -------------------------------------------------------------- Globals  */


/**
 * Library Debug flag. If set to true, emit debug output
 */
extern bool MonitDebug;


#endif


