#ifndef _BEEP_OPR_H
#define _BEEP_OPR_H

struct beep_operations {
	int (*init) (int which); /* 初始化beep, which-哪个beep */       
	int (*ctl) (int which, char status); /* 控制LED, which-哪个beep, status:1-响(驱动写低电平),0-静音 */
};

struct beep_operations *get_board_beep_opr(void);


#endif

