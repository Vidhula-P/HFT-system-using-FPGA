// used to verify-
// crc out= 1617404739


///////////////////////////////////////
/// 640x480 version!
/// test VGA with hardware video input copy to VGA
// compile with
// gcc pio_test_1.c -o pio 
///////////////////////////////////////
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h> 
#include <sys/shm.h> 
#include <sys/mman.h>
#include <sys/time.h> 
#include <math.h> 

// main bus; PIO
#define FPGA_AXI_BASE 	0xC0000000
#define FPGA_AXI_SPAN   0x00001000
// main axi bus base
void *h2p_virtual_base;
volatile unsigned int * axi_pio_ptr = NULL ;
volatile unsigned int * axi_pio_read_ptr = NULL ;

// lw bus; PIO
#define FPGA_LW_BASE 	0xff200000
#define FPGA_LW_SPAN	0x00001000
// the light weight bus base
void *h2p_lw_virtual_base;
// HPS_to_FPGA FIFO status address = 0
volatile unsigned int * lw_pio_ptr = NULL ;
volatile unsigned int * lw_pio_read_ptr = NULL ;

volatile unsigned int * crc_print_ptr = NULL ;

// read offset is 0x10 for both busses
// remember that eaxh axi master bus needs unique address
#define FPGA_PIO_READ	0x10
#define FPGA_PIO_WRITE	0x00

#define PIO_CRC_OUT_OFFSET 0x30


// /dev/mem file id
int fd;	
	
int main(void)
{

	// Declare volatile pointers to I/O registers (volatile 	
	// means that IO load and store instructions will be used 	
	// to access these pointer locations,  
  
	// === get FPGA addresses ==================
    // Open /dev/mem
	if( ( fd = open( "/dev/mem", ( O_RDWR | O_SYNC ) ) ) == -1 ) 	{
		printf( "ERROR: could not open \"/dev/mem\"...\n" );
		return( 1 );
	}
    
	//============================================
    // get virtual addr that maps to physical
	// for light weight AXI bus
	h2p_lw_virtual_base = mmap( NULL, FPGA_LW_SPAN, ( PROT_READ | PROT_WRITE ), MAP_SHARED, fd, FPGA_LW_BASE );	
	if( h2p_lw_virtual_base == MAP_FAILED ) {
		printf( "ERROR: mmap1() failed...\n" );
		close( fd );
		return(1);
	}
	// Get the addresses that map to the two parallel ports on the light-weight bus
	lw_pio_ptr = (unsigned int *)(h2p_lw_virtual_base);
	lw_pio_read_ptr = (unsigned int *)(h2p_lw_virtual_base + FPGA_PIO_READ);
	
	//============================================
	
	// ===========================================
	// get virtual address for
	// AXI bus addr 
	h2p_virtual_base = mmap( NULL, FPGA_AXI_SPAN, ( PROT_READ | PROT_WRITE ), MAP_SHARED, fd, FPGA_AXI_BASE); 	
	if( h2p_virtual_base == MAP_FAILED ) {
		printf( "ERROR: mmap3() failed...\n" );
		close( fd );
		return(1);
	}
    // Get the addresses that map to the two parallel ports on the AXI bus
	axi_pio_ptr =(unsigned int *)(h2p_virtual_base);
	axi_pio_read_ptr =(unsigned int *)(h2p_virtual_base + FPGA_PIO_READ);
	//============================================

	// Get the addresses that map to crc print output register
	crc_print_ptr = (unsigned int *)(h2p_lw_virtual_base + PIO_CRC_OUT_OFFSET);
	
	
	while(1) 
	{
		// int num, pio_read;
		// int junk; 
		// // input a number
		// junk = scanf("%d", &num);
		
		// // send to PIOs
		// *(lw_pio_ptr)  = num ;
		// *(axi_pio_ptr) = num ;
		
		// // receive back and print
		// printf("pio in=%d %d\n\r", *(lw_pio_read_ptr), *(axi_pio_read_ptr)) ;
		printf("crc out= %u \n\r", *(crc_print_ptr)) ;
		// printf("crc out= %d \n\r", 2953);

		
		
	} // end while(1)
} // end main

/// /// ///////////////////////////////////// 
/// end /////////////////////////////////////
