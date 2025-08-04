/*
	core/interrupt/pic.c
*/

#include <ClassiX/interrupt.h>
#include <ClassiX/io.h>

/*
	@brief 初始化 PIC (可编程中断控制器)。
*/
void init_pic(void)
{
	out8(PIC0_IMR,  0b11111111); /* 屏蔽主 PIC 所有中断 */
	out8(PIC1_IMR,  0b11111111); /* 屏蔽从 PIC 所有中断 */

	out8(PIC0_ICW1, 0x11); /* 边沿触发模式 */
	out8(PIC0_ICW2, 0x20); /* IRQ0-7 由 INT20-27接收 */
	out8(PIC0_ICW3, 0x04); /* PIC1 由 IRQ2 连接 */
	out8(PIC0_ICW4, 0x01); /* 无缓冲区模式 */

	out8(PIC1_ICW1, 0x11); /* 边沿触发模式 */
	out8(PIC1_ICW2, 0x28); /* IRQ8-15 由 INT28-2f 接收 */
	out8(PIC1_ICW3, 0x02); /* PIC1 由 IRQ2 连接 */
	out8(PIC1_ICW4, 0x01); /* 无缓冲区模式 */

	out8(PIC0_IMR,  0b11111001); /* 允许 IRQ1 和 IRQ2 */
	out8(PIC1_IMR,  0b11101111); /* 允许 IRQ12*/
}