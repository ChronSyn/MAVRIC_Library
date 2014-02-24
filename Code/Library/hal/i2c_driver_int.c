/*
 * i2c_driver.c
 *
 * Created: 16/05/2012 17:31:58
 *  Author: sfx
 */ 
#include "i2c_driver_int.h"
#include "gpio.h"
#include "sysclk.h"
#include "print_util.h"

i2c_packet_t transfer_queue[I2C_SCHEDULE_SLOTS];
int current_slot, last_slot;
i2c_packet_t *current_transfer;

/*!  The I2C interrupt handler.
 */

ISR(i2c_int_handler_i2c0,CONF_TWIM_IRQ_GROUP,CONF_TWIM_IRQ_LEVEL)
//__attribute__((__interrupt__))
//static void i2c_int_handler_i2c0(void)
{
	 volatile avr32_twim_t *twim = &AVR32_TWIM0;
		// get masked status register value
	uint32_t status = twim->sr &(AVR32_TWIM_SR_STD_MASK |AVR32_TWIM_SR_TXRDY_MASK |AVR32_TWIM_SR_RXRDY_MASK) ;
	// this is a NACK
	if (status & AVR32_TWIM_SR_STD_MASK) {
		//if we get a nak, clear the valid bit in cmdr, 
		//otherwise the command will be resent.
		current_transfer->transfer_in_progress = false;
		twim->CMDR.valid = 0;
		twim->scr = ~0UL;
		twim->idr = ~0UL;
	}
	// this is a RXRDY
	else if (status & AVR32_TWIM_SR_RXRDY_MASK) {
		
		// get data from Receive Holding Register
		if (current_transfer->data_index < current_transfer->data_size) {
			current_transfer->data[current_transfer->data_index]= twim->rhr;
			current_transfer->data_index++;
		} else {			
			// finish the receive operation
			twim->idr = AVR32_TWIM_IDR_RXRDY_MASK;
			twim->cr = AVR32_TWIM_CR_MDIS_MASK;
			// set busy to false
			current_transfer->transfer_in_progress = false;
		}
	}
	// this is a TXRDY
	else if (status & AVR32_TWIM_SR_TXRDY_MASK) {
		
		// get data from transmit data block
		if (current_transfer->data_index < current_transfer->data_size) {
			
			// put the byte in the Transmit Holding Register
			twim->thr = current_transfer->data[current_transfer->data_index];
			current_transfer->data_index++;
			
		} else { //nothing more to write
			twim->idr = AVR32_TWIM_IDR_TXRDY_MASK;
			
			if (current_transfer->direction==I2C_WRITE1_THEN_READ) {
				// reading should already be set up in next command register...
				
			}	else  { // all done
				twim->cr = AVR32_TWIM_CR_MDIS_MASK;
				// set busy to false
				current_transfer->transfer_in_progress=false;				
			}		
			
		
		}
		
	}
	//return;
	
   // call callback function to process data, at end of transfer
   // to process data, and maybe add some more data
//   schedule[0][current_schedule_slot[0]].transfer_in_progress=0;
   
//   if (schedule[0][current_schedule_slot[0]].callback) schedule[0][current_schedule_slot[0]].callback;
   //putstring(&AVR32_USART0, "!");
}



/*!  The I2C interrupt handler.
 */
//__attribute__((__interrupt__))
//static void i2c_int_handler_i2c1(void)
//{

//}



int init_i2c(unsigned char i2c_device) {
	// int i;
	volatile avr32_twim_t *twim;
	switch (i2c_device) {
	case 0: 
		twim=&AVR32_TWIM0;
		// Register PDCA IRQ interrupt.
		INTC_register_interrupt( (__int_handler) &i2c_int_handler_i2c0, AVR32_TWIM0_IRQ, AVR32_INTC_INT1);
		gpio_enable_module_pin(AVR32_TWIMS0_TWCK_0_0_PIN, AVR32_TWIMS0_TWCK_0_0_FUNCTION);
		gpio_enable_module_pin(AVR32_TWIMS0_TWD_0_0_PIN, AVR32_TWIMS0_TWD_0_0_FUNCTION);

	break;
	case 1:
		twim=&AVR32_TWIM1;// Register PDCA IRQ interrupt.
//		INTC_register_interrupt( (__int_handler) &i2c_int_handler_i2c1, AVR32_TWIM1_IRQ, AVR32_INTC_INT1);
		gpio_enable_module_pin(AVR32_TWIMS1_TWCK_0_0_PIN, AVR32_TWIMS1_TWCK_0_0_FUNCTION);
		gpio_enable_module_pin(AVR32_TWIMS1_TWD_0_0_PIN, AVR32_TWIMS1_TWD_0_0_FUNCTION);
		//gpio_enable_pin_pull_up(AVR32_TWIMS1_TWCK_0_0_PIN);
		//gpio_enable_pin_pull_up(AVR32_TWIMS1_TWD_0_0_PIN);
	break;
	default: // invalid device ID
		return -1;
	}		
				
	bool global_interrupt_enabled = cpu_irq_is_enabled ();
	// Disable TWI interrupts
	if (global_interrupt_enabled) {
		cpu_irq_disable ();
	}
	twim->idr = ~0UL;
	// Enable master transfer
	twim->cr = AVR32_TWIM_CR_MEN_MASK;
	// Reset TWI
	twim->cr = AVR32_TWIM_CR_SWRST_MASK;
	twim->cr = AVR32_TWIM_CR_MDIS_MASK;
	
	
	if (global_interrupt_enabled) {
		cpu_irq_enable ();
	}
	// Clear SR
	twim->scr = ~0UL;
	
	
	
	// register Register twim_master_interrupt_handler interrupt on level CONF_TWIM_IRQ_LEVEL
//	irqflags_t flags = cpu_irq_save();
//	irq_register_handler(twim_master_interrupt_handler,
//			CONF_TWIM_IRQ_LINE, CONF_TWIM_IRQ_LEVEL);
//	cpu_irq_restore(flags);
	
	// Select the speed
	if (twim_set_speed(twim, 400000, sysclk_get_pba_hz()) == 
			ERR_INVALID_ARG) {
		
		return ERR_INVALID_ARG;
	}
	return STATUS_OK;				

}



char i2c_reset(unsigned char i2c_device) {
	volatile avr32_twim_t *twim;
	switch (i2c_device) {
	case 0: 
		twim=&AVR32_TWIM0;
	break;
	case 1:
		twim=&AVR32_TWIM1;
	break;
	default: // invalid device ID
		return -1;
	}		
	bool global_interrupt_enabled = cpu_irq_is_enabled ();
	// Disable TWI interrupts
	if (global_interrupt_enabled) {
		cpu_irq_disable ();
	}
	twim->idr = ~0UL;
	// Enable master transfer
	twim->cr = AVR32_TWIM_CR_MEN_MASK;
	// Reset TWI
	twim->cr = AVR32_TWIM_CR_SWRST_MASK;
	if (global_interrupt_enabled) {
		cpu_irq_enable ();
	}
	// Clear SR
	twim->scr = ~0UL;
}


char i2c_trigger_request(unsigned char i2c_device, i2c_packet_t *transfer) {
	// initiate transfer of given request
	// set up DMA channel
	volatile avr32_twim_t *twim;
	
	bool global_interrupt_enabled = cpu_irq_is_enabled ();
	if (global_interrupt_enabled) {
		cpu_irq_disable ();
	}
	
	switch (i2c_device) {
	case 0: 
		twim=&AVR32_TWIM0;
		twim->cr = AVR32_TWIM_CR_MEN_MASK;
		twim->cr = AVR32_TWIM_CR_SWRST_MASK;
		twim->cr = AVR32_TWIM_CR_MDIS_MASK;
		twim->scr = ~0UL;
		// Clear the interrupt flags
		twim->idr = ~0UL;
		if (twim_set_speed(twim, transfer->i2c_speed, sysclk_get_pba_hz()) == 
			ERR_INVALID_ARG) {
			return ERR_INVALID_ARG;
	    }
		
//		pdca_load_channel(TWI0_DMA_CH, (void *)schedule[i2c_device][schedule_slot].config.write_data, schedule[i2c_device][schedule_slot].config.write_count);
		// Enable pdca interrupt each time the reload counter reaches zero, i.e. each time
		// the whole block was received
		
		//pdca_enable_interrupt_transfer_error(TWI0_DMA_CH);
		
		
		break;
	case 1:
		twim=&AVR32_TWIM1;
		twim->cr = AVR32_TWIM_CR_MEN_MASK;
		twim->cr = AVR32_TWIM_CR_SWRST_MASK;
		twim->cr = AVR32_TWIM_CR_MDIS_MASK;
		twim->scr = ~0UL;
		// Clear the interrupt flags
		twim->idr = ~0UL;
		if (twim_set_speed(twim, transfer->i2c_speed, sysclk_get_pba_hz()) ==
		ERR_INVALID_ARG) {
			return ERR_INVALID_ARG;
		}
	break;
	default: // invalid device ID
		return -1;
	}		

	// set up I2C speed and mode
//	twim_set_speed(twim, 100000, sysclk_get_pba_hz());
    
	switch (1/*conf->direction*/)  {
		case I2C_READ:
 			twim->cmdr = (transfer->slave_address << AVR32_TWIM_CMDR_SADR_OFFSET)
 						| (transfer->data_size << AVR32_TWIM_CMDR_NBYTES_OFFSET)
 						| (AVR32_TWIM_CMDR_VALID_MASK)
 						| (AVR32_TWIM_CMDR_START_MASK)
 						| (AVR32_TWIM_CMDR_STOP_MASK)
 						| (AVR32_TWIM_CMDR_READ_MASK);
			twim->ncmdr=0;					

			twim->ier = AVR32_TWIM_IER_STD_MASK |  AVR32_TWIM_IER_RXRDY_MASK;
			break;	
		case I2C_WRITE1_THEN_READ:
			// set up next command register for the burst read transfer
			// set up command register to initiate the write transfer. The DMA will take care of the reading once this is done.
 			twim->cmdr = (transfer->slave_address << AVR32_TWIM_CMDR_SADR_OFFSET)
 						| ((1)<< AVR32_TWIM_CMDR_NBYTES_OFFSET)
 						| (AVR32_TWIM_CMDR_VALID_MASK)
 						| (AVR32_TWIM_CMDR_START_MASK)
 						//| (AVR32_TWIM_CMDR_STOP_MASK)
 						| (0 << AVR32_TWIM_CMDR_READ_OFFSET);
 						;
						 
 			twim->ncmdr = (transfer->slave_address << AVR32_TWIM_CMDR_SADR_OFFSET)
 						| ((transfer->data_size) << AVR32_TWIM_CMDR_NBYTES_OFFSET)
 						| (AVR32_TWIM_CMDR_VALID_MASK)
 						| (AVR32_TWIM_CMDR_START_MASK)
 						| (AVR32_TWIM_CMDR_STOP_MASK)
 						| (AVR32_TWIM_CMDR_READ_MASK);
						
			twim->ier = AVR32_TWIM_IER_STD_MASK | AVR32_TWIM_IER_RXRDY_MASK | AVR32_TWIM_IER_TXRDY_MASK;
			// set up writing of one byte (usually a slave register index)
			twim->cr = AVR32_TWIM_CR_MEN_MASK;
			twim->thr=transfer->write_then_read_preamble;
			twim->cr = AVR32_TWIM_CR_MEN_MASK;
			
			break;	
		case I2C_WRITE:
 			twim->cmdr = (transfer->slave_address << AVR32_TWIM_CMDR_SADR_OFFSET)
 						| ((transfer->data_size) << AVR32_TWIM_CMDR_NBYTES_OFFSET)
 						| (AVR32_TWIM_CMDR_VALID_MASK)
 						| (AVR32_TWIM_CMDR_START_MASK)
 						| (AVR32_TWIM_CMDR_STOP_MASK)					
 						;
			twim->ncmdr=0;						;	
			twim->ier = AVR32_TWIM_IER_NAK_MASK |  AVR32_TWIM_IER_TXRDY_MASK;
			
		break;	
	}		
	// start transfer
	
	current_transfer=transfer;
	current_transfer->transfer_in_progress=1;
	current_transfer->data_index=0;
	
	if (global_interrupt_enabled) {
			cpu_irq_enable ();
		}	
	twim->cr = AVR32_TWIM_CR_MEN_MASK;

}


int i2c_append_transfer(i2c_packet_t *request) {
	
}
int i2c_append_read_transfer(uint8_t slave_address, uint8_t *data, uint16_t size, task_handle_t *event_handler) {
	
}
int i2c_append_write_transfer(uint8_t slave_address, uint8_t *data, uint16_t size, task_handle_t *event_handler) {
	
}
int i2c_append_register_read_transfer(uint8_t slave_address, uint8_t register_address, uint8_t *data, uint16_t size, task_handle_t *event_handler) {
	
}

int i2c_clear_queue() {
	
}

bool i2c_is_ready() {
	
}
int i2c_get_queued_requests() {
	
}