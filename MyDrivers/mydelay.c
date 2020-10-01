#include "stm32f1xx.h"
#include "mydelay.h"
volatile int ticks;

void SysTick_Handler(void) {
	ticks++;
}
void delay_ms(int ms) {
	SystemCoreClockUpdate();
	SysTick_Config(SystemCoreClock / 1000);

	ticks = 0;
	while (ticks < ms)
		;
	SysTick->CTRL &= ~(SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk); //disable systick
}
