/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "lib.h"


 #define NUM_IRQS 16

/* Interrupt masks to determine which interrupts
 * are enabled and disabled */
uint8_t master_mask; /* IRQs 0-7 */
uint8_t slave_mask; /* IRQs 8-15 */

/* i8259_init
 * DESCRIPTION: Initialize the 8259 PIC
 * INPUTS: none
 * OUTPUTS: writes several bytes to the PIC
 * RETURN VALUE: none
 * SIDE EFFECTS: after execution, the PIC will be available to trigger interrupts
 */
void i8259_init()
{
	int i;

	outb( ICW1, MASTER_8259_PORT);
	outb( ICW1, SLAVE_8259_PORT);
	outb(ICW2_MASTER, MASTER_DATA );
	outb(ICW2_SLAVE, SLAVE_DATA);
	outb(ICW3_MASTER, MASTER_DATA );
	outb(ICW3_SLAVE, SLAVE_DATA);
	outb(ICW4, MASTER_DATA );
	outb(ICW4, SLAVE_DATA);

	for(i = 0; i < NUM_IRQS; i++){

		disable_irq(i);
	}
	enable_irq(2);				// cascade PIC IRQ

}

/* enable_irq
 * DESCRIPTION: Enable (unmask) the specified IRQ 
 * INPUTS: irq_num - number of the PIC channel to enable
 * OUTPUTS: send bytes to PIC to enable the IRQ
 * RETURN VALUE: none
 * SIDE EFFECTS: none
 */
void enable_irq(uint32_t irq_num)
{
	uint8_t irq_reg_val;
	

	if(irq_num >= 8)						//SLAVE
	{
		irq_num -= 8;				//because we will be accessing slave's ports
		irq_reg_val = inb(SLAVE_DATA);
		irq_reg_val = irq_reg_val & ~(1 << irq_num);
		outb(irq_reg_val, SLAVE_DATA);
	}
	else									//MASTER
	{
		irq_reg_val = inb(MASTER_DATA );
		//printf("irq_reg_val = %d\n", irq_reg_val);
		irq_reg_val = irq_reg_val & ~(1 << irq_num);
		//printf("irq_reg_val = %d\n", irq_reg_val);
		outb(irq_reg_val, MASTER_DATA );
	}
}

/* disable_irq
 * DESCRIPTION: Disable (mask) the specified IRQ
 * INPUTS: irq_num - IRQ channel to be masked
 * OUTPUTS: sends signal to PIC to disable IRQ
 * RETURN VALUE: none
 * SIDE EFFECTS: IRQ is disabled
 */
void disable_irq(uint32_t irq_num)
{
	uint8_t irq_reg_val;
	

	if(irq_num >= 8)						//SLAVE
	{
		irq_num = irq_num - 8;				//because we will be accessing slave's ports
		irq_reg_val = inb(SLAVE_8259_PORT + 1);
	//	printf("irq_reg_val = %d\n", irq_reg_val);
		irq_reg_val = irq_reg_val | (1 << irq_num);
		outb( irq_reg_val, SLAVE_8259_PORT + 1);
	}
	else									//MASTER
	{	
		irq_reg_val = inb(MASTER_DATA );
	//	printf("irq_reg_val = %d\n", irq_reg_val);
		irq_reg_val = irq_reg_val | (1 << irq_num);
		outb(irq_reg_val, MASTER_DATA );
	}
}

/* send_eoi
 * DESCRIPTION: Send end-of-interrupt signal for the specified IRQ
 * INPUTS: irq_num - IRQ to send eoi to
 * OUTPUTS: sends EOI signal to that IRQ on the PIC
 * RETURN VALUE: none
 * SIDE EFFECTS: Given IRQ is reset
 */
void send_eoi(uint32_t irq_num)
{
	if(irq_num >= 8)
	{
		outb(EOI | (irq_num-=8), SLAVE_8259_PORT);
		outb(EOI | 2, MASTER_8259_PORT);
	}
	else
		outb(EOI | irq_num, MASTER_8259_PORT);
}

