/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef DYNAMIC_DEVICES_H
#define DYNAMIC_DEVICES_H
typedef BOOLEAN (*DYNAMIC_DEVICE_LINE)(UINTN Index, CHAR16 *Text, VOID *Context);
VOID DynamicGpus(DYNAMIC_DEVICE_LINE Emit, VOID *Context);
VOID DynamicDisks(DYNAMIC_DEVICE_LINE Emit, VOID *Context);
#endif
