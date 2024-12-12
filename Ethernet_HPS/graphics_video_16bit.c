///////////////////////////////////////
/// 640x480 version! 16-bit color
/// This code will segfault the original
/// DE1 computer
/// compile with
/// gcc graphics_video_16bit.c -o gr -O2 -lm
///
///////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h> 
#include <sys/shm.h> 
#include <sys/mman.h>
#include <sys/time.h> 
#include <math.h>
//#include "address_map_arm_brl4.h"

#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>    // For errno

// video display
#define SDRAM_BASE            0xC0000000
#define SDRAM_END             0xC3FFFFFF
#define SDRAM_SPAN			  0x04000000
// characters
#define FPGA_CHAR_BASE        0xC9000000 
#define FPGA_CHAR_END         0xC9001FFF
#define FPGA_CHAR_SPAN        0x00002000
/* Cyclone V FPGA devices */
#define HW_REGS_BASE          0xff200000
//#define HW_REGS_SPAN        0x00200000 
#define HW_REGS_SPAN          0x00005000 


//PIO base addresses
#define reset_base 		  0x1000

#define action_a_base		  0x00004060
#define action_b_base		  0x00004080
#define action_c_base		  0x000040A0

#define price_a_base		  0x00004000
#define price_b_base		  0x00004020
#define price_c_base		  0x00004040

#define SCREEN_WIDTH 319
#define SCREEN_HEIGHT 239
#define Q2_X_MIN 0
#define Q2_X_MAX 159
#define Q2_Y_MIN 120
#define Q2_Y_MAX 239

// graphics primitives
void VGA_text (int, int, char *);
void VGA_text_clear();
void VGA_box (int, int, int, int, short);
void VGA_rect (int, int, int, int, short);
void VGA_line(int, int, int, int, short) ;
void VGA_Vline(int, int, int, short) ;
void VGA_Hline(int, int, int, short) ;
void VGA_disc (int, int, int, short);
//void VGA_circle (int, int, int, int);
// 16-bit primary colors
#define red  (0+(0<<5)+(31<<11))
#define dark_red (0+(0<<5)+(15<<11))
#define green (0+(63<<5)+(0<<11))
#define dark_green (0+(31<<5)+(0<<11))
#define blue (31+(0<<5)+(0<<11))
#define dark_blue (15+(0<<5)+(0<<11))
#define yellow (0+(63<<5)+(31<<11))
#define cyan (31+(63<<5)+(0<<11))
#define magenta (31+(0<<5)+(31<<11))
#define black (0x0000)
#define gray (15+(31<<5)+(51<<11))
#define white (0xffff)
int colors[] = {red, dark_red, green, dark_green, blue, dark_blue, 
		yellow, cyan, magenta, gray, black, white};

// pixel macro
#define VGA_PIXEL(x,y,color) do{\
	int  *pixel_ptr ;\
	pixel_ptr = (int*)((char *)vga_pixel_ptr + (((y)*640+(x))<<1)) ; \
	*(short *)pixel_ptr = (color);\
} while(0)

// the light weight buss base
void *h2p_lw_virtual_base;

// pixel buffer
volatile unsigned int * vga_pixel_ptr = NULL ;
void *vga_pixel_virtual_base;

// character buffer
volatile unsigned int * vga_char_ptr = NULL ;
void *vga_char_virtual_base;

//reset signal
volatile unsigned int *reset_pio_output = NULL;

// actions buffer
volatile unsigned int * lw_action_a_ptr = NULL ;
volatile unsigned int * lw_action_b_ptr = NULL ;
volatile unsigned int * lw_action_c_ptr = NULL ;

volatile unsigned int * lw_price_a_ptr = NULL ;
volatile unsigned int * lw_price_b_ptr = NULL ;
volatile unsigned int * lw_price_c_ptr = NULL ;

// /dev/mem file id
int fd;

// measure time
struct timeval t1, t2;
double elapsedTime;

char color_index = 0 ; //declared as global variable so it can be incremented in while(1) of main

// Function to check for packets and return the received buffer
char* check_for_packets(int sockfd, struct sockaddr_in* client_addr, socklen_t* addr_len) {
    static char buffer[1024]; // Static buffer to persist after function returns
    int len = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)client_addr, addr_len);
    if (len > 0) {
        buffer[len] = '\0'; // Null-terminate the received string
        return buffer;      // Return the buffer with data
    } else if (len == -1 && (errno == EWOULDBLOCK || errno == EAGAIN)) {
        return NULL;        // No data available
    } else if (len == -1) {
        perror("recvfrom error");
        return NULL;        // Error case
    }
    return NULL;            // Default case (should not occur)
}

void display_unit(int *x, int* y1, int* y2, int* y3, int* y4, int size){
	// Display unit

	// cycle thru the colors
	if (color_index++ == 11) color_index = 0;

	int i;

	int max_x = x[size-1];
	int min_x = x[0];
	int max_y1 = y1[0];
	int min_y1 = y1[0];
	// int max_y2 = y2[0];
	// int min_y2 = y2[0];
	// int max_y3 = y3[0];
	// int min_y3 = y3[0];
	// int max_y4 = y4[0];
	// int min_y4 = y4[0];

	for (i = 1; i < size; i++){
		max_y1 = (y1[i] > max_y1) ? y1[i] : max_y1;
		min_y1 = (y1[i] < min_y1) ? y1[i] : min_y1;
		// max_y2 = (y2[i] > max_y2) ? y2[i] : max_y2;
		// min_y2 = (y2[i] < min_y2) ? y2[i] : min_y2;
		// max_y3 = (y3[i] > max_y3) ? y3[i] : max_y3;
		// min_y3 = (y3[i] < min_y3) ? y3[i] : min_y3;
		// max_y4 = (y4[i] > max_y4) ? y4[i] : max_y4;
		// min_y4 = (y4[i] < min_y4) ? y4[i] : min_y4;
	}


	int screen_x1, screen_y1, screen_x1_prev, screen_y1_prev;
	// int screen_x2, screen_y2, screen_x2_prev, screen_y2_prev;
	// int screen_x3, screen_y3, screen_x3_prev, screen_y3_prev;
	// int screen_x4, screen_y4, screen_x4_prev, screen_y4_prev;

	int padding = 2;
	float x_scale1 = 319.0 / (max_x - min_x);
	float y_scale1 = 239.0 / (max_y1 - min_y1);	
	// float x_scale2 = 319.0 / (max_x - min_x);
	// float y_scale2 = 239.0 / (max_y2 - min_y2);
	// float x_scale3 = 319.0 / (max_x - min_x);
	// float y_scale3 = 239.0 / (max_y3 - min_y3);
	// float x_scale4 = 319.0 / (max_x - min_x);
	// float y_scale4 = 239.0 / (max_y4 - min_y4);

	for (i = 1; i < size; i++) {
		// Quadrant 1
		int screen_x1 = (x[i] - min_x) * x_scale1;
		int screen_y1 = (y1[i] - min_y1) * y_scale1;
		int screen_x1_prev = (x[i - 1] - min_x) * x_scale1;
		int screen_y1_prev = (y1[i - 1] - min_y1) * y_scale1;
		VGA_line(screen_x1_prev, screen_y1_prev, screen_x1, screen_y1, colors[color_index]);
		VGA_PIXEL(screen_x1, screen_y1, green);

		// Quadrant 2
		// int screen_x2 = (x[i] * 0.5) + 319;
		// int screen_y2 = (y2[i] * 0.5);
		// int screen_x2_prev = (x[i - 1] * 0.5) + 319;
		// int screen_y2_prev = (y2[i - 1] * 0.5);
		// VGA_line(screen_x2_prev, screen_y2_prev, screen_x2, screen_y2, colors[color_index]);
		// VGA_PIXEL(screen_x2, screen_y2, green);

		// // Quadrant 3
		// int screen_x3 = x[i] * 0.5;
		// int screen_y3 = (y3[i] * 0.5) + 239;
		// int screen_x3_prev = x[i - 1] * 0.5;
		// int screen_y3_prev = (y3[i - 1] * 0.5) + 239;
		// VGA_line(screen_x3_prev, screen_y3_prev, screen_x3, screen_y3, colors[color_index]);
		// VGA_PIXEL(screen_x3, screen_y3, green);

		// // Quadrant 4
		// int screen_x4 = (x[i] * 0.5) + 319;
		// int screen_y4 = (y4[i] * 0.5) + 239;
		// int screen_x4_prev = (x[i - 1] * 0.5) + 319;
		// int screen_y4_prev = (y4[i - 1] * 0.5) + 239;
		// VGA_line(screen_x4_prev, screen_y4_prev, screen_x4, screen_y4, colors[color_index]);
		// VGA_PIXEL(screen_x4, screen_y4, green);
	}	 
	// Draw X-axis
	VGA_line(0, 240, 639, 240, white);  // Horizontal center line

	// Draw Y-axis
	VGA_line(320, 0, 320, 479, white);  // Vertical center line	
}


void plot_stock_price_trends(int* x, int* price_a, int* price_b, int* price_c, int size) {
    // Calculate scales for Quadrant 2
    float x_scale = (float)(Q2_X_MAX - Q2_X_MIN) / (x[size - 1] - x[0]); // Based on time range
    float y_scale = (float)(Q2_Y_MAX - Q2_Y_MIN) / 2000; // Assuming max price is 2000 (adjust if needed)
	int i;

    // Draw lines for price_a, price_b, and price_c
    for (i = 1; i < size; i++) {
        // Calculate screen coordinates for price_a
        int screen_x1_a = Q2_X_MIN + (x[i - 1] - x[0]) * x_scale;
        int screen_y1_a = Q2_Y_MAX - (price_a[i - 1] * y_scale);
        int screen_x2_a = Q2_X_MIN + (x[i] - x[0]) * x_scale;
        int screen_y2_a = Q2_Y_MAX - (price_a[i] * y_scale);

        // Calculate screen coordinates for price_b
        int screen_x1_b = Q2_X_MIN + (x[i - 1] - x[0]) * x_scale;
        int screen_y1_b = Q2_Y_MAX - (price_b[i - 1] * y_scale);
        int screen_x2_b = Q2_X_MIN + (x[i] - x[0]) * x_scale;
        int screen_y2_b = Q2_Y_MAX - (price_b[i] * y_scale);

        // Calculate screen coordinates for price_c
        int screen_x1_c = Q2_X_MIN + (x[i - 1] - x[0]) * x_scale;
        int screen_y1_c = Q2_Y_MAX - (price_c[i - 1] * y_scale);
        int screen_x2_c = Q2_X_MIN + (x[i] - x[0]) * x_scale;
        int screen_y2_c = Q2_Y_MAX - (price_c[i] * y_scale);

        // Plot lines for price_a
        VGA_line(screen_x1_a, screen_y1_a, screen_x2_a, screen_y2_a, red);
        // VGA_PIXEL(screen_x2_a, screen_y2_a, white);

        // Plot lines for price_b
        VGA_line(screen_x1_b, screen_y1_b, screen_x2_b, screen_y2_b, green);
        // VGA_PIXEL(screen_x2_b, screen_y2_b, white);

        // Plot lines for price_c
        VGA_line(screen_x1_c, screen_y1_c, screen_x2_c, screen_y2_c, blue);
        // VGA_PIXEL(screen_x2_c, screen_y2_c, white);
    }
}


	
int main(void)
{
  	
	// === need to mmap: =======================
	// FPGA_CHAR_BASE
	// FPGA_ONCHIP_BASE      
	// HW_REGS_BASE        
  
	// === get FPGA addresses ==================
    // Open /dev/mem
	if( ( fd = open( "/dev/mem", ( O_RDWR | O_SYNC ) ) ) == -1 ) 	{
		printf( "ERROR: could not open \"/dev/mem\"...\n" );
		return( 1 );
	}
    
    // get virtual addr that maps to physical
	h2p_lw_virtual_base = mmap( NULL, HW_REGS_SPAN, ( PROT_READ | PROT_WRITE ), MAP_SHARED, fd, HW_REGS_BASE );	
	if( h2p_lw_virtual_base == MAP_FAILED ) {
		printf( "ERROR: mmap1() failed...\n" );
		close( fd );
		return(1);
	}
    

	// === get VGA char addr =====================
	// get virtual addr that maps to physical
	vga_char_virtual_base = mmap( NULL, FPGA_CHAR_SPAN, ( 	PROT_READ | PROT_WRITE ), MAP_SHARED, fd, FPGA_CHAR_BASE );	
	if( vga_char_virtual_base == MAP_FAILED ) {
		printf( "ERROR: mmap2() failed...\n" );
		close( fd );
		return(1);
	}
    
    // Get the address that maps to the FPGA LED control 
	vga_char_ptr =(unsigned int *)(vga_char_virtual_base);

	// === get VGA pixel addr ====================
	// get virtual addr that maps to physical
	vga_pixel_virtual_base = mmap( NULL, SDRAM_SPAN, ( PROT_READ | PROT_WRITE ), MAP_SHARED, fd, SDRAM_BASE);	
	if( vga_pixel_virtual_base == MAP_FAILED ) {
		printf( "ERROR: mmap3() failed...\n" );
		close( fd );
		return(1);
	}
    
    // Get the address that maps to the FPGA pixel buffer
	vga_pixel_ptr =(unsigned int *)(vga_pixel_virtual_base);

	reset_pio_output = (unsigned int *)(h2p_lw_virtual_base + reset_base);

	//mapping actions
	lw_action_a_ptr = (unsigned int*)(h2p_lw_virtual_base + action_a_base);
	lw_action_b_ptr = (unsigned int*)(h2p_lw_virtual_base + action_b_base);
	lw_action_c_ptr = (unsigned int*)(h2p_lw_virtual_base + action_c_base);

	
	//mapping prices
	lw_price_a_ptr = (unsigned int*)(h2p_lw_virtual_base + price_a_base);
	lw_price_b_ptr = (unsigned int*)(h2p_lw_virtual_base + price_b_base);
	lw_price_c_ptr = (unsigned int*)(h2p_lw_virtual_base + price_c_base);


	// ===========================================

	/* create a message to be displayed on the VGA 
          and LCD displays */
	char quad1_title[40] = "Title 1\0";
	char quad2_title[40] = "Title 2\0";
	char quad3_title[40] = "Title 3\0";
	char quad4_title[40] = "Title 4\0";
	char num_string[20], time_string[20] ;
	int color_counter = 0 ;
	
	// position of disk primitive
	int disc_x = 0;
	// position of circle primitive
	int circle_x = 0 ;
	// position of box primitive
	int box_x = 5 ;
	// position of vertical line primitive
	int Vline_x = 350;
	// position of horizontal line primitive
	int Hline_y = 250;

	//VGA_text (34, 1, quad1_title);
	//VGA_text (34, 2, quad4_title);
	// clear the screen
	VGA_box (0, 0, 639, 479, 0x0000);
	// clear the text
	VGA_text_clear();
	// write text
	VGA_text (15, 1, quad1_title);
	VGA_text (55, 1, quad2_title);
	VGA_text (15, 31, quad3_title);
	VGA_text (55, 31, quad4_title);
	// VGA_text (64, 58, quad4_title);
	
	// R bits 11-15 mask 0xf800
	// G bits 5-10  mask 0x07e0
	// B bits 0-4   mask 0x001f
	// so color = B+(G<<5)+(R<<11);

	//code for UDP/Ethernet

    // Create a UDP socket
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    // Create a UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        return -1;
    }

    // Set the socket to non-blocking mode
    if (fcntl(sockfd, F_SETFL, O_NONBLOCK) < 0) {
        perror("Failed to set non-blocking mode");
        close(sockfd);
        return -1;
    }

    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(5000); // Port number
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket to the address
    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        return -1;
    }

    printf("Waiting for packets...\n");
	char buffer[1024];


	//printf("action_a = %d:\n", *lw_action_a_ptr); // received from FPGA
	*lw_price_a_ptr = 0;
	*lw_price_b_ptr = 0;
	*lw_price_c_ptr = 0;


	// Constant x values for time stamps
	int x[] = { 100, 120, 140, 160, 180, 200, 220, 240, 260, 280, 300, 320, 340, 360, 380, 400 };
	int num_timestamps = 16;
	
	// Dynamic arrays for asset and profit tracking
	int *y1 = (int *)malloc(num_timestamps*sizeof(int));
	if (!y1) {
		perror("Memory allocation failed for y1");
		return -1;
	}
	
	// Initialize y1 with initial asset value (e.g., beginning balance)
	y1[0] = 1000; // Beginning balance

	int y2[] = { 100, 120, 140, 160, 180, 200, 220, 240, 260, 280, 300, 320, 340, 360, 380, 400 };
	int y3[] = { 100, 120, 140, 160, 180, 200, 220, 240, 260, 280, 300, 320, 340, 360, 380, 400 };
	int y4[] = { 100, 120, 140, 160, 180, 200, 220, 240, 260, 280, 300, 320, 340, 360, 380, 400 };

	int price_a, price_b, price_c;
	int action_a, action_b, action_c;
	int size = 16;
	// int *a_array = (int *)malloc(size*sizeof(int));
	// int *b_array = (int *)malloc(size*sizeof(int));
	// int *c_array = (int *)malloc(size*sizeof(int));

	int a_array[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	int b_array[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	int c_array[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

	
	while(1) 
	{
		// Call the function to check for incoming packets
		char* data = check_for_packets(sockfd, &client_addr, &addr_len);
		static int count = 0; 
		static char flag16 = 0; 
		int profit = 0; // Reset profit for every iteration
		int i;

		if (data) {
			printf("Received: %s\n", data);
			
			// Logic to extract price_a, price_b, and price_c
			char stock_name[10];
			sscanf(data, "%[^,],%u,%u,%u", stock_name, &price_a, &price_b, &price_c);
			*lw_price_a_ptr = (unsigned int)price_a;
			*lw_price_b_ptr = (unsigned int)price_b;
			*lw_price_c_ptr = (unsigned int)price_c; 
			printf("price_a = %u, price_b = %u, price_c = %u\n", *lw_price_a_ptr, *lw_price_b_ptr, *lw_price_c_ptr); // To send to FPGA

			action_a = *lw_action_a_ptr;
			action_b = *lw_action_b_ptr;
			action_c = *lw_action_c_ptr;
			printf("action_a = %u, action_b = %u, action_c = %u\n", action_a, action_b, action_c); // Received from FPGA

			if (action_a == 2) profit += price_a;        // Selling at high price
			else if (action_b == 2) profit += price_b;
			else if (action_c == 2) profit += price_c; 

			if (action_a == 1) profit -= price_a;        // Buying at low price
			else if (action_b == 1) profit -= price_b;
			else if (action_c == 1) profit -= price_c;

			// Update y1 array with the new transaction result
			if (count > 0) {
				y1[count] = y1[count - 1] + profit;
			} else {
				y1[count] = 100 + profit; // Starting balance is 100
			}

			if (count == 15) {
				flag16 = 1;
			} else {
				count++;
			}

			// Shift y1 values back when buffer is full
			if (flag16) {
				VGA_box (0, 0, 319, 239, 0x0000);
				for (i = 0; i < size - 1; i++) {
					y1[i] = y1[i + 1];
				}
				y1[15] = y1[14] + profit; // Add the latest profit
			}

			for (i = 0; i < size-1; i++) {
				a_array[i] = a_array[i + 1];
				b_array[i] = b_array[i + 1];
				c_array[i] = c_array[i + 1];
			}
			// Add the new value at the end
			a_array[size-1] = price_a;
			b_array[size-1] = price_b;
			c_array[size-1] = price_c;
		

			// Clear the buffer after processing
			memset(data, 0, 1024);
		}

		// Placeholder y2, y3, y4 arrays
		int y2[16] = {100, 120, 140, 160, 180, 200, 220, 240, 260, 280, 300, 320, 340, 360, 380, 400};
		int y3[16] = {100, 120, 140, 160, 180, 200, 220, 240, 260, 280, 300, 320, 340, 360, 380, 400};
		int y4[16] = {100, 120, 140, 160, 180, 200, 220, 240, 260, 280, 300, 320, 340, 360, 380, 400};

		// Update the display with the new data
		display_unit(x, y1, y2, y3, y4, size);
		plot_stock_price_trends(x, a_array, b_array, c_array, size);
	} // end while(1)

} // end main

/****************************************************************************************
 * Subroutine to send a string of text to the VGA monitor 
****************************************************************************************/
void VGA_text(int x, int y, char * text_ptr)
{
  	volatile char * character_buffer = (char *) vga_char_ptr ;	// VGA character buffer
	int offset;
	/* assume that the text string fits on one line */
	offset = (y << 7) + x;
	while ( *(text_ptr) )
	{
		// write to the character buffer
		*(character_buffer + offset) = *(text_ptr);	
		++text_ptr;
		++offset;
	}
}

/****************************************************************************************
 * Subroutine to clear text to the VGA monitor 
****************************************************************************************/
void VGA_text_clear()
{
  	volatile char * character_buffer = (char *) vga_char_ptr ;	// VGA character buffer
	int offset, x, y;
	for (x=0; x<79; x++){
		for (y=0; y<59; y++){
	/* assume that the text string fits on one line */
			offset = (y << 7) + x;
			// write to the character buffer
			*(character_buffer + offset) = ' ';		
		}
	}
}

/****************************************************************************************
 * Draw a filled rectangle on the VGA monitor 
****************************************************************************************/
#define SWAP(X,Y) do{int temp=X; X=Y; Y=temp;}while(0) 

void VGA_box(int x1, int y1, int x2, int y2, short pixel_color)
{
	char  *pixel_ptr ; 
	int row, col;

	/* check and fix box coordinates to be valid */
	if (x1>639) x1 = 639;
	if (y1>479) y1 = 479;
	if (x2>639) x2 = 639;
	if (y2>479) y2 = 479;
	if (x1<0) x1 = 0;
	if (y1<0) y1 = 0;
	if (x2<0) x2 = 0;
	if (y2<0) y2 = 0;
	if (x1>x2) SWAP(x1,x2);
	if (y1>y2) SWAP(y1,y2);
	for (row = y1; row <= y2; row++)
		for (col = x1; col <= x2; ++col)
		{
			//640x480
			//pixel_ptr = (char *)vga_pixel_ptr + (row<<10)    + col ;
			// set pixel color
			//*(char *)pixel_ptr = pixel_color;	
			VGA_PIXEL(col,row,pixel_color);	
		}
}

/****************************************************************************************
 * Draw a outline rectangle on the VGA monitor 
****************************************************************************************/
#define SWAP(X,Y) do{int temp=X; X=Y; Y=temp;}while(0) 

void VGA_rect(int x1, int y1, int x2, int y2, short pixel_color)
{
	char  *pixel_ptr ; 
	int row, col;

	/* check and fix box coordinates to be valid */
	if (x1>639) x1 = 639;
	if (y1>479) y1 = 479;
	if (x2>639) x2 = 639;
	if (y2>479) y2 = 479;
	if (x1<0) x1 = 0;
	if (y1<0) y1 = 0;
	if (x2<0) x2 = 0;
	if (y2<0) y2 = 0;
	if (x1>x2) SWAP(x1,x2);
	if (y1>y2) SWAP(y1,y2);
	// left edge
	col = x1;
	for (row = y1; row <= y2; row++){
		//640x480
		//pixel_ptr = (char *)vga_pixel_ptr + (row<<10)    + col ;
		// set pixel color
		//*(char *)pixel_ptr = pixel_color;	
		VGA_PIXEL(col,row,pixel_color);		
	}
		
	// right edge
	col = x2;
	for (row = y1; row <= y2; row++){
		//640x480
		//pixel_ptr = (char *)vga_pixel_ptr + (row<<10)    + col ;
		// set pixel color
		//*(char *)pixel_ptr = pixel_color;	
		VGA_PIXEL(col,row,pixel_color);		
	}
	
	// top edge
	row = y1;
	for (col = x1; col <= x2; ++col){
		//640x480
		//pixel_ptr = (char *)vga_pixel_ptr + (row<<10)    + col ;
		// set pixel color
		//*(char *)pixel_ptr = pixel_color;	
		VGA_PIXEL(col,row,pixel_color);
	}
	
	// bottom edge
	row = y2;
	for (col = x1; col <= x2; ++col){
		//640x480
		//pixel_ptr = (char *)vga_pixel_ptr + (row<<10)    + col ;
		// set pixel color
		//*(char *)pixel_ptr = pixel_color;
		VGA_PIXEL(col,row,pixel_color);
	}
}

/****************************************************************************************
 * Draw a horixontal line on the VGA monitor 
****************************************************************************************/
#define SWAP(X,Y) do{int temp=X; X=Y; Y=temp;}while(0) 

void VGA_Hline(int x1, int y1, int x2, short pixel_color)
{
	char  *pixel_ptr ; 
	int row, col;

	/* check and fix box coordinates to be valid */
	if (x1>639) x1 = 639;
	if (y1>479) y1 = 479;
	if (x2>639) x2 = 639;
	if (x1<0) x1 = 0;
	if (y1<0) y1 = 0;
	if (x2<0) x2 = 0;
	if (x1>x2) SWAP(x1,x2);
	// line
	row = y1;
	for (col = x1; col <= x2; ++col){
		//640x480
		//pixel_ptr = (char *)vga_pixel_ptr + (row<<10)    + col ;
		// set pixel color
		//*(char *)pixel_ptr = pixel_color;	
		VGA_PIXEL(col,row,pixel_color);		
	}
}

/****************************************************************************************
 * Draw a vertical line on the VGA monitor 
****************************************************************************************/
#define SWAP(X,Y) do{int temp=X; X=Y; Y=temp;}while(0) 

void VGA_Vline(int x1, int y1, int y2, short pixel_color)
{
	char  *pixel_ptr ; 
	int row, col;

	/* check and fix box coordinates to be valid */
	if (x1>639) x1 = 639;
	if (y1>479) y1 = 479;
	if (y2>479) y2 = 479;
	if (x1<0) x1 = 0;
	if (y1<0) y1 = 0;
	if (y2<0) y2 = 0;
	if (y1>y2) SWAP(y1,y2);
	// line
	col = x1;
	for (row = y1; row <= y2; row++){
		//640x480
		//pixel_ptr = (char *)vga_pixel_ptr + (row<<10)    + col ;
		// set pixel color
		//*(char *)pixel_ptr = pixel_color;	
		VGA_PIXEL(col,row,pixel_color);			
	}
}


/****************************************************************************************
 * Draw a filled circle on the VGA monitor 
****************************************************************************************/

void VGA_disc(int x, int y, int r, short pixel_color)
{
	char  *pixel_ptr ; 
	int row, col, rsqr, xc, yc;
	
	rsqr = r*r;
	
	for (yc = -r; yc <= r; yc++)
		for (xc = -r; xc <= r; xc++)
		{
			col = xc;
			row = yc;
			// add the r to make the edge smoother
			if(col*col+row*row <= rsqr+r){
				col += x; // add the center point
				row += y; // add the center point
				//check for valid 640x480
				if (col>639) col = 639;
				if (row>479) row = 479;
				if (col<0) col = 0;
				if (row<0) row = 0;
				//pixel_ptr = (char *)vga_pixel_ptr + (row<<10) + col ;
				// set pixel color
				//*(char *)pixel_ptr = pixel_color;
				VGA_PIXEL(col,row,pixel_color);	
			}
					
		}
}

/****************************************************************************************
 * Draw a  circle on the VGA monitor 
****************************************************************************************/

/*void VGA_circle(int x, int y, int r, int pixel_color)
{
	char  *pixel_ptr ; 
	int row, col, rsqr, xc, yc;
	int col1, row1;
	rsqr = r*r;
	
	for (yc = -r; yc <= r; yc++){
		//row = yc;
		col1 = (int)sqrt((float)(rsqr + r - yc*yc));
		// right edge
		col = col1 + x; // add the center point
		row = yc + y; // add the center point
		//check for valid 640x480
		if (col>639) col = 639;
		if (row>479) row = 479;
		if (col<0) col = 0;
		if (row<0) row = 0;
		//pixel_ptr = (char *)vga_pixel_ptr + (row<<10) + col ;
		// set pixel color
		//*(char *)pixel_ptr = pixel_color;
		VGA_PIXEL(col,row,pixel_color);	
		// left edge
		col = -col1 + x; // add the center point
		//check for valid 640x480
		if (col>639) col = 639;
		if (row>479) row = 479;
		if (col<0) col = 0;
		if (row<0) row = 0;
		//pixel_ptr = (char *)vga_pixel_ptr + (row<<10) + col ;
		// set pixel color
		//*(char *)pixel_ptr = pixel_color;
		VGA_PIXEL(col,row,pixel_color);	
	}
	for (xc = -r; xc <= r; xc++){
		//row = yc;
		row1 = (int)sqrt((float)(rsqr + r - xc*xc));
		// right edge
		col = xc + x; // add the center point
		row = row1 + y; // add the center point
		//check for valid 640x480
		if (col>639) col = 639;
		if (row>479) row = 479;
		if (col<0) col = 0;
		if (row<0) row = 0;
		//pixel_ptr = (char *)vga_pixel_ptr + (row<<10) + col ;
		// set pixel color
		//*(char *)pixel_ptr = pixel_color;
		VGA_PIXEL(col,row,pixel_color);	
		// left edge
		row = -row1 + y; // add the center point
		//check for valid 640x480
		if (col>639) col = 639;
		if (row>479) row = 479;
		if (col<0) col = 0;
		if (row<0) row = 0;
		//pixel_ptr = (char *)vga_pixel_ptr + (row<<10) + col ;
		// set pixel color
		//*(char *)pixel_ptr = pixel_color;
		VGA_PIXEL(col,row,pixel_color);	
	}
}*/

// =============================================
// === Draw a line
// =============================================
//plot a line 
//at x1,y1 to x2,y2 with color 
//Code is from David Rodgers,
//"Procedural Elements of Computer Graphics",1985
void VGA_line(int x1, int y1, int x2, int y2, short c) {
	int e;
	signed int dx,dy,j, temp;
	signed int s1,s2, xchange;
     signed int x,y;
	char *pixel_ptr ;

	
	/* check and fix line coordinates to be valid */
	if (x1>639) x1 = 639;
	if (y1>479) y1 = 479;
	if (x2>639) x2 = 639;
	if (y2>479) y2 = 479;
	if (x1<0) x1 = 0;
	if (y1<0) y1 = 0;
	if (x2<0) x2 = 0;
	if (y2<0) y2 = 0;
        
	x = x1;
	y = y1;
	
	//take absolute value
	if (x2 < x1) {
		dx = x1 - x2;
		s1 = -1;
	}

	else if (x2 == x1) {
		dx = 0;
		s1 = 0;
	}

	else {
		dx = x2 - x1;
		s1 = 1;
	}

	if (y2 < y1) {
		dy = y1 - y2;
		s2 = -1;
	}

	else if (y2 == y1) {
		dy = 0;
		s2 = 0;
	}

	else {
		dy = y2 - y1;
		s2 = 1;
	}

	xchange = 0;   

	if (dy>dx) {
		temp = dx;
		dx = dy;
		dy = temp;
		xchange = 1;
	} 

	e = ((int)dy<<1) - dx;  
	 
	for (j=0; j<=dx; j++) {
		//video_pt(x,y,c); //640x480
		//pixel_ptr = (char *)vga_pixel_ptr + (y<<10)+ x; 
		// set pixel color
		//*(char *)pixel_ptr = c;
		VGA_PIXEL(x,y,c);			
		 
		if (e>=0) {
			if (xchange==1) x = x + s1;
			else y = y + s2;
			e = e - ((int)dx<<1);
		}

		if (xchange==1) y = y + s2;
		else x = x + s1;

		e = e + ((int)dy<<1);
	}
}
