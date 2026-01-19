	#include <unistd.h>
	#include <stdio.h>
	#include <stdint.h>
	#include <stdbool.h>

	/*******************************************************************************
	 * This file provides address values that exist in the DE10-Lite Computer
	 * This file also works for DE1-SoC, except change #define DE10LITE to 0
	 ******************************************************************************/

	#ifndef __SYSTEM_INFO__
	#define __SYSTEM_INFO__

	#define DE10LITE 0 // change to 0 for CPUlator or DE1-SoC, 1 for DE10-Lite

	/* do not change anything after this line */

	#if DE10LITE
		#define BOARD		"DE10-Lite"
		#define MAX_X		160
		#define MAX_Y		120
		#define YSHIFT		 8
	#else
		#define MAX_X		320
		#define MAX_Y		240
		#define YSHIFT		 9
	#endif


	/* Memory */
	#define SDRAM_BASE			0x00000000
	#define SDRAM_END			0x03FFFFFF
	#define FPGA_PIXEL_BUF_BASE		0x08000000
	#define FPGA_PIXEL_BUF_END		0x0800FFFF
	#define FPGA_CHAR_BASE			0x09000000
	#define FPGA_CHAR_END			0x09001FFF

	/* Devices */
	#define LED_BASE			0xFF200000
	#define LEDR_BASE			0xFF200000
	#define HEX3_HEX0_BASE			0xFF200020
	#define HEX5_HEX4_BASE			0xFF200030
	#define SW_BASE				0xFF200040
	#define KEY_BASE			0xFF200050
	#define JP1_BASE			0xFF200060
	#define ARDUINO_GPIO			0xFF200100
	#define ARDUINO_RESET_N			0xFF200110
	#define JTAG_UART_BASE			0xFF201000
	#define TIMER_BASE			0xFF202000
	#define TIMER_2_BASE			0xFF202020
	#define MTIMER_BASE			0xFF202100
	#define RGB_RESAMPLER_BASE 	 	0xFF203010
	#define PIXEL_BUF_CTRL_BASE		0xFF203020
	#define CHAR_BUF_CTRL_BASE		0xFF203030
	#define ADC_BASE			0xFF204000
	#define ACCELEROMETER_BASE		0xFF204020
	
	//interrupts
	
	#define MSTATUS_REG "mstatus"
	#define MIE_REG     "mie"
	#define MCAUSE_REG  "mcause"
	#define MTVEC_REG   "mtvec"

	#define MIE_MTIE_BIT (1 << 7)
	#define MIE_KEY_BIT  (1 << 18)
	#define MSTATUS_MIE  (1 << 3)
	
	#define CSR_READ(reg, val) \
    __asm__ volatile ("csrr %0, " reg : "=r"(val))

	#define CSR_WRITE(reg, val) \
    __asm__ volatile ("csrw " reg ", %0" :: "r"(val))

	#define CSR_SET_BITS(reg, mask) \
    __asm__ volatile ("csrs " reg ", %0" :: "r"(mask))

	#define CSR_CLEAR_BITS(reg, mask) \
    __asm__ volatile ("csrc " reg ", %0" :: "r"(mask))
	
	
	/* Nios V memory-mapped registers */
	// Time Interrupts
	#define MTIME_LOW       ((volatile uint32_t *)(MTIMER_BASE + 0))
	#define MTIME_HIGH      ((volatile uint32_t *)(MTIMER_BASE + 4))
	#define MTIMECMP_LOW    ((volatile uint32_t *)(MTIMER_BASE + 8))
	#define MTIMECMP_HIGH   ((volatile uint32_t *)(MTIMER_BASE + 12))

	// Key Interrupts
	#define KEY_DATA        ((volatile int *)(KEY_BASE + 0))
	#define KEY_MASK        ((volatile int *)(KEY_BASE + 8))
	#define KEY_EDGECAP     ((volatile int *)(KEY_BASE + 12))
	
	//Display pointers
	
	#define SW_PTR 		((volatile int*) SW_BASE)
	#define LEDR_PTR 	((volatile int *) LEDR_BASE)
	#define HEX03_PTR ((volatile int *)HEX3_HEX0_BASE) 
	#define HEX45_PTR ((volatile int *)HEX5_HEX4_BASE)
	

	// 7-seg HEX encoding 
	int seven_seg_digits[10] = {
				0x3F, // 0
				0x06, // 1
				0x5B, // 2
				0x4F, // 3
				0x66, // 4
				0x6D, // 5
				0x7D, // 6
				0x07, // 7
				0x7F, // 8
				0x6F  // 9
	};

	typedef uint16_t pixel_t;

	//VGA Monitor pointer
	volatile pixel_t *pVGA = ((pixel_t *)FPGA_PIXEL_BUF_BASE);

	//Push Button (KEY0-3)
	volatile int *pKEY = (int *) 0xFF200050;
	

	const pixel_t blk = 0x0000; // Black
	const pixel_t wht = 0xffff; // White (Wall/Border)
	const pixel_t red = 0xf800; // Red (Human Player)
	const pixel_t grn = 0x07e0; // Green (Obstacle)
	const pixel_t blu = 0x001f; // Blue (Robot Player)
		
	volatile int p1_score = 0; volatile int p2_score = 0; volatile int game_over_flag = 0; 
	volatile int pending_turn = 0;
	
	//direction
	#define UP	  1
	#define DOWN  2
	#define LEFT  3
	#define RIGHT 4
	
	//border
	#define BORDER 			20

	#endif

	// Create a 16-bit color pixel from 8-bit RGB components (R5G6B5 format)
	pixel_t makePixel( uint8_t r8, uint8_t g8, uint8_t b8 )
		{
			const uint16_t r5 = (r8 & 0xf8)>>3; // keep 5b red
			const uint16_t g6 = (g8 & 0xfc)>>2; // keep 6b green
			const uint16_t b5 = (b8 & 0xf8)>>3; // keep 5b blue
			return (pixel_t)( (r5<<11) | (g6<<5) | b5 );
		}
	
	// Draw a single pixel on the VGA display
	void drawPixel(int y,int x,pixel_t c){
		if(y>=0 && y<MAX_Y && x>=0 && x<MAX_X)
			*(pVGA + (y<<YSHIFT) + x) = c;
	}

	// Read the color of a pixel at a given coordinate
	pixel_t readPixel(int y, int x) {
		if(y<0 || y>=MAX_Y || x<0 || x>=MAX_X) return wht;
		return *(pVGA + (y<<YSHIFT) + x);
	}

	// Draw a filled rectangle
	void rect( int y1, int y2, int x1, int x2, pixel_t c )
		{
			for( int y=y1  ; y < y2; y++ )
				for( int x = x1; x < x2; x++ )
					drawPixel( y, x, c );
		}

	void displayScores(){
		// Clamp scores to 0 9
		int s1 = (p1_score > 9 ? 9 : (p1_score < 0 ? 0 : p1_score));
		int s2 = (p2_score > 9 ? 9 : (p2_score < 0 ? 0 : p2_score));

		volatile int *HEX03 = (int*) HEX03_PTR;  // HEX0 HEX3 register

		// HEX0 -> player 1's score
		*HEX03 = (*HEX03 & ~0x00FF) | (seven_seg_digits[s1]);

		// HEX1 -> player 2's score score
		*HEX03 = (*HEX03 & ~0xFF00) | (seven_seg_digits[s2] << 8);
	}
	
	
	
	
	uint64_t read_mtime(){
    uint32_t hi,lo,hi2;
    do{
        hi = *MTIME_HIGH;
        lo = *MTIME_LOW;
        hi2 = *MTIME_HIGH;
    } while(hi != hi2);
    return ((uint64_t)hi<<32) | lo;
	}
	
	// mtime/mtimecmp timer mechanism
	// set the next timer interrupt
	void set_next_timer(){
		//read the switches
    uint32_t sw = *SW_PTR & 0x3FF;
		//compute the delay
    uint32_t base = 1000000; 
	
    uint32_t delay = base + (sw * 50000);
		//read the timer
    uint64_t now = read_mtime();
		//set the next timer
    uint64_t next = now + delay;

    *MTIMECMP_HIGH = 0xFFFFFFFF;
    *MTIMECMP_LOW = (uint32_t)(next & 0xFFFFFFFF);
    *MTIMECMP_HIGH = (uint32_t)(next >> 32);
	}


	volatile int p1_x, p1_y, p1_dx, p1_dy; 
	volatile int p2_x, p2_y, p2_dx, p2_dy;
	pixel_t p1_color, p2_color;
	
	// reset the game and players
	void resetGame() {
		game_over_flag = 0;
		pending_turn = 0;
		*LEDR_PTR = 0;
		
		// player 1
		p1_x = MAX_X/3; 
		p1_y = MAX_Y/2; 
		p1_dx = 1; //RIGHT
		p1_dy = 0;
		p1_color = makePixel(255,0,0); // Red
		
		// player 2
		p2_x = (MAX_X*2)/3; 
		p2_y = MAX_Y/2; 
		p2_dx = -1; // LEFT
		p2_dy = 0;
		p2_color = makePixel(0,0,255); // Blue

		//draw a black background
		rect( 0, MAX_Y, 0, MAX_X, blk );

		//draw a white border
		rect( 0, BORDER, 0, MAX_X, wht); 	     //top
		rect( 0, MAX_Y, MAX_X - BORDER, MAX_X, wht); //right
		rect( 0, MAX_Y , 0, BORDER, wht); 	     //left
		rect(MAX_Y - BORDER, MAX_Y, 0, MAX_X, wht);  // bottom

		//draw obstacles (green)
		rect(MAX_Y/2 + 20, MAX_Y/2 + 30, MAX_X/2 + 50, MAX_X/2 + 80, grn);
		rect(MAX_Y/2 + 10, MAX_Y/2 + 50, MAX_X/2 + 20, MAX_X/2 + 30, grn);
		
		displayScores();
		set_next_timer();

	}
	
	// Key interrupts
		
		void KEY_ISR(){ 
			// store key[0] or key[1]
		int edge = *KEY_EDGECAP;
		*KEY_EDGECAP = edge; 
			// key [1] pressed
		if(edge & 0x2){ 
			// no turning left if already pending
			if(pending_turn == -1) pending_turn = 0; 
			// left turn
			else pending_turn = -1; 
		} 
			// key[0] pressed
		if(edge & 0x1){ 
			// no turning right if already pending
			if(pending_turn == +1) pending_turn = 0;
			// turning right
			else pending_turn = +1; 
		
		} 
		
		// display LEDs
			int L = 0; 
		if(pending_turn == -1) 
			L = 0x2; // LEFT, display on LED1
		if(pending_turn == +1) 
			L = 0x1; // RIGHT, display on LED0
		*LEDR_PTR = L; 
		}


	void TIMER_ISR(){
		
		set_next_timer();
		
		// game over!

		if(game_over_flag) return;

		//if LEFT 
		if(pending_turn == -1){
			// LEFT rotate CCW
			int t = p1_dx; p1_dx = -p1_dy; p1_dy = t; 
		}
		else if(pending_turn == +1){ 
			// RIGHT rotate CW
			int t = p1_dx; p1_dx = p1_dy; p1_dy = -t;
		}
		// reset pending = 0
		pending_turn = 0;
		// reset LEDs
		*LEDR_PTR = 0;


		bool blocked =
			(readPixel(p2_y+p2_dy, p2_x+p2_dx)!=blk) ||
			(readPixel(p2_y+2*p2_dy,p2_x+2*p2_dx)!=blk);
			
		if(blocked){
			int ldx =  p2_dy, ldy = -p2_dx;
			int rdx = -p2_dy, rdy =  p2_dx;
			// if blk in LEFT -> turning to that direction
			if(readPixel(p2_y+ldy, p2_x+ldx)==blk){
				p2_dx = ldx; p2_dy = ldy;
			}
			// if blk in RIGHT -> turning to that direction
			else if(readPixel(p2_y+rdy, p2_x+rdx)==blk){
				p2_dx = rdx; p2_dy = rdy;
			}
		}

		int nx1 = p1_x + p1_dx; int ny1 = p1_y + p1_dy;
		int nx2 = p2_x + p2_dx; int ny2 = p2_y + p2_dy;

		bool c1 = (readPixel(ny1,nx1)!=blk); // player 1 is gonna hit something
		bool c2 = (readPixel(ny2,nx2)!=blk); // player 2 is gonna hit something

		if(c1 || c2){
			if(c1 && c2) game_over_flag = 3; // neither win
			else if(c1)  game_over_flag = 1; // player 2 wins
			else         game_over_flag = 2; // player 1 wins
			return;		 // one round is over
		}

		p1_x = nx1; p1_y = ny1; // move player 1
		p2_x = nx2; p2_y = ny2; // move player 2

		drawPixel(p1_y,p1_x,p1_color);
		drawPixel(p2_y,p2_x,p2_color);
	}

	//tells when the code hits interrupts (like an alarm)

    void __attribute__ ((interrupt)) interrupt_handler() {
		uint32_t cause;
		CSR_READ(MCAUSE_REG, cause);

		if(cause & 0x80000000){
			int code = cause & 0x7FFFFFFF;
			if(code == 7)  TIMER_ISR();
			if(code == 18) KEY_ISR(); 
		}
	}

	int main(){
    
	// Disable all interrupts while setting up
    CSR_WRITE(MIE_REG, 0);

	// Set the address of the interrupt handler function
    CSR_WRITE(MTVEC_REG, (uint32_t)interrupt_handler);

	// Initialize player scores to 0
    p1_score = 0;
    p2_score = 0;

	// Reset the game
    resetGame(); 
	
	// Enable KEY0 and KEY1 interrupts (mask bits 0 and 1)
    *KEY_MASK = 0x3;

	// Clear any existing key press events and enable edge capture for all keys
    *KEY_EDGECAP = 0xF;

	// Enable machine timer interrupt and key/button interrupt
    CSR_SET_BITS(MIE_REG, MIE_MTIE_BIT | MIE_KEY_BIT);

	// Enable global interrupts
    CSR_SET_BITS(MSTATUS_REG, MSTATUS_MIE);

	// Infinite main loop; runs forever
    while(1){
	// If a player has lost (game over)
    
	if(game_over_flag){
            
	// Simple delay to pause before resetting the game
            for(volatile int i=0;i < 2000000; i++);

        // Update scores
            
		if(game_over_flag==1) p2_score++;  // Player 1 lost, Player 2 gets 1 pt
            
			else if(game_over_flag==2) p1_score++; // Player 2 lost, Player 1 gets 1 pt

        // Display scores

            displayScores();

        // Win condition

            if(p1_score>=9 || p2_score>=9){
                
	// Fill the entire screen with the winner's color
                
		pixel_t c = (p1_score>=9)? p1_color: p2_color;
                
		rect(0,MAX_Y,0,MAX_X,c);

                // Stop the game indefinitely
                while(1); 
            }

        // Reset game state for next round
            resetGame();
        }
    }

		return 0;
	}