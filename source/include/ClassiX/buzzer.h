/*
	include/ClassiX/buzzer.h
*/

#ifndef _CLASSIX_BUZZER_H_
#define _CLASSIX_BUZZER_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <ClassiX/interrupt.h>
#include <ClassiX/typedef.h>

void beep(uint32_t freq, uint32_t duration);
void async_buzzer_init(void);
void async_beep(uint32_t freq, uint32_t duration);

#ifdef __cplusplus
	}
#endif

#endif