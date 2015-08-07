#define DEFAULT_BAUD_RATE 115200

#define HWREAD32(a) *(volatile unsigned long *)(a)
#define HWREAD16(a) *(volatile unsigned short *)(a)
#define HWREAD8(a)  *(volatile unsigned char *)(a)

#define HWWRITE32(a,b)  *(volatile unsigned long *)(a) = (b)
#define HWWRITE16(a,b)  *(volatile unsigned short *)(a) = (b)
#define HWWRITE8(a,b)   *(volatile unsigned char *)(a) = (b)

#define GPIO1_21    (1<<21)
#define GPIO1_22    (1<<22)
#define GPIO1_23    (1<<23)
#define GPIO1_24    (1<<24)

// Referring to BBB schematic...
// Pg6 shows the 4 user leds, pg3 shows the pins on the Sitara
// The four user leds are on GPIO1, pins 21-24
#define USR0_LED    GPIO1_21        // Ball V15: GPMC_A5 mode7 (D2)
#define USR1_LED    GPIO1_22        // Ball U15: GPMC_A6 mode7 (D3)
#define USR2_LED    GPIO1_23        // Ball T15: GPMC_A7 mode7 (D4)
#define USR3_LED    GPIO1_24        // Ball V16: GPMC_A8 mode7 (D5)
