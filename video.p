// \file
/* PRU based IBM 5291 video driver.
 *
 * The ARM writes a 1-bit color 512x384 bitmap into the shared RAM and the
 * PRU writes it to the video output continuously.
 *
 * During the horizontal blanking interval, the PRU loads an entire 512
 * bits of data (16x 32-bit registers).
 */
.origin 0
.entrypoint START

#include "ws281x.hp"
/** Mappings of the GPIO devices */
#define GPIO0 (0x44E07000 + 0x100)
#define GPIO1 (0x4804c000 + 0x100)
#define GPIO2 (0x481AC000 + 0x100)
#define GPIO3 (0x481AE000 + 0x100)

/** Offsets for the clear and set registers in the devices.
 * Since the offsets can only be 0xFF, we deliberately add offsets
 */
#define GPIO_CLRDATAOUT (0x190 - 0x100)
#define GPIO_SETDATAOUT (0x194 - 0x100)

/** Register map */
#define data_addr r0
#define row r1
#define col r2
#define video_pin r3
#define hsync_pin r4
#define vsync_pin r5
#define gpio2_base r6
#define timer_ptr r8
#define pixel_data r10 // the next 16 registers, too
#define tmp1 r28
#define tmp2 r29

/** GPIO2 pin numbers for our outputs */
#define VIDEO_PIN 5
#define HSYNC_PIN 2
#define VSYNC_PIN 3
#define BC_PIN 4
#define BE_PIN 1

.macro GPIO_LO
.mparam pin
	mov tmp1, 1<<pin
	sbbo tmp1, gpio2_base, GPIO_CLRDATAOUT, 4
.endm

.macro GPIO_HI
.mparam pin
	mov tmp1, 1<<pin
	sbbo tmp1, gpio2_base, GPIO_SETDATAOUT, 4
.endm
	
#define NOP ADD r0, r0, 0

#define DATA_ROWS 340
#define RETRACE_ROWS 14

/** Reset the cycle counter. Should be invoked once at the start
    of each row.
*/
.macro resetcounter
	// Disable the counter and clear it, then re-enable it
	// This starts our clock at the start of the row.
	LBBO tmp2, timer_ptr, 0, 4
	CLR tmp2, tmp2, 3 // disable counter bit
	SBBO tmp2, timer_ptr, 0, 4 // write it back

	MOV r10, 20 // 20: compensate for cycles in macro
	SBBO r10, timer_ptr, 0xC, 4 // clear the timer

	SET tmp2, tmp2, 3 // enable counter bit
	SBBO tmp2, timer_ptr, 0, 4 // write it back
.endm

/** Wait for the cycle counter to hit the given absolute value.
    The counter is reset at the start of each row.
*/
.macro waitforns
.mparam ns
	MOV tmp1, (ns)/5;
waitloop:
	LBBO tmp2, timer_ptr, 0xC, 4; /* read the cycle counter */
	QBGT waitloop, tmp2, tmp1;
.endm

START:
    // Enable OCP master port
    // clear the STANDBY_INIT bit in the SYSCFG register,
    // otherwise the PRU will not be able to write outside the
    // PRU memory space and to the BeagleBon's pins.
    LBCO	r0, C4, 4, 4
    CLR		r0, r0, 4
    SBCO	r0, C4, 4, 4

    // Configure the programmable pointer register for PRU0 by setting
    // c28_pointer[15:0] field to 0x0120.  This will make C28 point to
    // 0x00012000 (PRU shared RAM).
    MOV		r0, 0x00000120
    MOV		r1, CTPPR_0
    ST32	r0, r1

    // Configure the programmable pointer register for PRU0 by setting
    // c31_pointer[15:0] field to 0x0010.  This will make C31 point to
    // 0x80001000 (DDR memory).
    MOV		r0, 0x00100000
    MOV		r1, CTPPR_1
    ST32	r0, r1

    // Write a 0x1 into the response field so that they know we have started
    MOV r2, #0x1
    SBCO r2, CONST_PRUDRAM, 12, 4

    MOV timer_ptr, 0x22000 /* control register */

    // Configure our output pins
    MOV gpio2_base, GPIO2
    MOV video_pin, 1 << VIDEO_PIN
    MOV hsync_pin, 1 << HSYNC_PIN
    MOV vsync_pin, 1 << VSYNC_PIN

    GPIO_HI VIDEO_PIN
    GPIO_HI HSYNC_PIN
    GPIO_HI VSYNC_PIN

    // Wait for the start condition from the main program to indicate
    // that we have a rendered frame ready to clock out.  This also
    // handles the exit case if an invalid value is written to the start
    // start position.
READ_LOOP:
        // Load the pointer to the buffer from PRU DRAM into r0 and the
        // length (in pixels) into r1.
        LBCO      data_addr, CONST_PRUDRAM, 0, 4

        // Wait for a non-zero command
        QBEQ READ_LOOP, data_addr, #0

        // Command of 0xFF is the signal to exit
        QBEQ EXIT, data_addr, #0xFF

	GPIO_LO VSYNC_PIN

	// the hsync keeps running at normal speed for
	// 15 frames
	MOV row, RETRACE_ROWS
	VSYNC_LOOP:
                resetcounter
		GPIO_HI HSYNC_PIN
		waitforns 10000
		GPIO_LO HSYNC_PIN
		waitforns 54400
		SUB row, row, 1
		QBNE VSYNC_LOOP, row, 0
	GPIO_HI VSYNC_PIN
		
        MOV row, DATA_ROWS

	ROW_LOOP:
                resetcounter
		// start the new row
		GPIO_HI HSYNC_PIN

		// Load the sixteen pixels worth of data outputs into
		// This takes about 250 ns
		LBBO pixel_data, data_addr, 0, 512/8

		MOV col, 0

		waitforns 10000

                GPIO_LO HSYNC_PIN

#define OUTPUT_COLUMN(rN) \
		MOV tmp1, 1<<VIDEO_PIN; \
		QBBC clr_##rN, rN, col; \
			sbbo tmp1, gpio2_base, GPIO_CLRDATAOUT, 4; \
			QBA skip_##rN; \
	col_##rN: ; \
		NOP; \
		NOP; \
		NOP; NOP; NOP; NOP; \
		QBBC clr_##rN, rN, col; \
			sbbo tmp1, gpio2_base, GPIO_CLRDATAOUT, 4; \
			QBA skip_##rN; \
		clr_##rN:; \
			NOP; \
			sbbo tmp1, gpio2_base, GPIO_SETDATAOUT, 4; \
		skip_##rN:; \
		ADD col, col, 1; \
		AND col, col, 31; \
		QBNE col_##rN, col, 0; \
		NOP; NOP; NOP; NOP; \
		NOP; NOP; NOP; NOP; \

		OUTPUT_COLUMN(r10); NOP; NOP;
		OUTPUT_COLUMN(r11); NOP; NOP;
		OUTPUT_COLUMN(r12); NOP; NOP;
		OUTPUT_COLUMN(r13); NOP; NOP;
		OUTPUT_COLUMN(r14); NOP; NOP;
		OUTPUT_COLUMN(r15); NOP; NOP;
		OUTPUT_COLUMN(r16); NOP; NOP;
		OUTPUT_COLUMN(r17); NOP; NOP;
		OUTPUT_COLUMN(r18); NOP; NOP;
		OUTPUT_COLUMN(r19); NOP; NOP;
		OUTPUT_COLUMN(r20); NOP; NOP;
		OUTPUT_COLUMN(r21); NOP; NOP;
		OUTPUT_COLUMN(r22); NOP; NOP;
		OUTPUT_COLUMN(r23); NOP; NOP;
		OUTPUT_COLUMN(r24); NOP; NOP;
		OUTPUT_COLUMN(r25); NOP; NOP;
		GPIO_LO VIDEO_PIN 

		// Increment our data_offset to point to the next row
		ADD data_addr, data_addr, 512/8

                SUB row, row, 1

		// Be sure that we wait for the right length of time
		// Force each line to be 54.4 usec
		waitforns 54400

                QBNE ROW_LOOP, row, 0
		// WAITNS(5000, wait_hsync_end2)
	QBA READ_LOOP
	
EXIT:
#ifdef AM33XX
    // Send notification to Host for program completion
    MOV R31.b0, PRU0_ARM_INTERRUPT+16
#else
    MOV R31.b0, PRU0_ARM_INTERRUPT
#endif

    HALT
