
#ifndef _BEEPDRV_H
#define _BEEPDRV_H

#include "beep_opr.h"

void beep_class_create_device(int minor);
void beep_class_destroy_device(int minor);
void register_beep_operations(struct beep_operations *opr);

#endif /* _BEEPDRV_H */

