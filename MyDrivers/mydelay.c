#include "stm32f1xx.h"
#include "mydelay.h"

static int count = 0;
void delay_ms(uint16_t ms)
{
	if (count == 0)
	{
		RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
		TIM3->PSC = SystemCoreClock / 1000;
		TIM3->CR1 |= TIM_CR1_ARPE;
		TIM3->CR1 |= TIM_CR1_OPM | TIM_CR1_URS;
		count++;
	}
	RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
	TIM3->ARR = ms;
	TIM3->EGR = 1;
	TIM3->CR1 |= TIM_CR1_CEN;
	while (!(TIM3->SR & TIM_SR_UIF))
		;
	TIM3->SR &= ~TIM_SR_UIF;
	RCC->APB1ENR &= ~RCC_APB1ENR_TIM3EN;
}
