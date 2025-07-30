/*
	core/int/pic.c
*/

#include <ClassiX/int.h>
#include <ClassiX/io.h>

/*
	@brief 初始化 PIC (可编程中断控制器)。
	@note 配置 PIC0 和 PIC1，禁止所有中断。
*/
void init_pic(void)
{
	out8(PIC0_IMR,  0xff  ); /* 禁止所有中断 */
	out8(PIC1_IMR,  0xff  ); /* 禁止所有中断 */

	out8(PIC0_ICW1, 0x11  ); /* 边沿触发模式 */
	out8(PIC0_ICW2, 0x20  ); /* IRQ0-7 由 INT20-27接收 */
	out8(PIC0_ICW3, 1 << 2); /* PIC1 由 IRQ2 连接 */
	out8(PIC0_ICW4, 0x01  ); /* 无缓冲区模式 */

	out8(PIC1_ICW1, 0x11  ); /* 边沿触发模式 */
	out8(PIC1_ICW2, 0x28  ); /* IRQ8-15 由 INT28-2f 接收 */
	out8(PIC1_ICW3, 2     ); /* PIC1 由 IRQ2 连接 */
	out8(PIC1_ICW4, 0x01  ); /* 无缓冲区模式 */

	out8(PIC0_IMR,  0xfb  ); /* PIC1 以外全部禁止 */
	out8(PIC1_IMR,  0xff  ); /* 禁止所有中断 */

	return;
}