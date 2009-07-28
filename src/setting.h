
/* setting.h
 *
 * Copyright (C) 2004-2004 Wang Xiaoguang (Chice) <chice_wxg@hotmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 *
 * Authors: Wang Xiaoguang (Chice) <chice_wxg@hotmail.com>
 */

#ifdef __cplusplus
	extern "C"
	{
#endif


#ifndef _SETTING_H_
#define _SETTING_H_

#include "os.h"

VOID setting_init();
VOID setting_cleanup();

STRING *setting_read(CHAR *filename, CHAR *key, CHAR *def);
VOID setting_write(CHAR *filename, CHAR *key, CHAR *value);
INT setting_read_int(CHAR *filename, CHAR *key, INT def);
VOID setting_write_int(CHAR *filename, CHAR *key, INT value);
#endif//_setting_H_




#ifdef __cplusplus
	}
#endif
