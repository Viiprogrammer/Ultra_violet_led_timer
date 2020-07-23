/*
 * GccApplication1.c
 *
 * Created: 28.06.2020 3:58:47
 *  Author: Maxim
 */ 


#include <avr/io.h>
#include <stdbool.h>
#include "74hc595.h"
#include <avr/interrupt.h>
uint16_t time = 0;
uint16_t time_delay_first = 0;
uint16_t time_delay_two = 0;
uint16_t time_delay_start = 0;
uint16_t timer = 0;
uint32_t adc_value = 0;
volatile uint32_t ovrf = 0;
bool started = false;
volatile uint8_t led = 0;
volatile uint8_t display[3] = {0, 0, 0};
uint8_t seg_table[] = {
	//pgfedcba
	0b00111111,	 // 0
	0b00000110,	 // 1
	0b01011011,	 // 2
	0b01001111,	 // 3
	0b01100110,	 // 4
	0b01101101,	 // 5
	0b01111101,	 // 6
	0b00000111,	 // 7
	0b01111111,	 // 8
	0b01101111	 // 9
};

ISR(TIM0_OVF_vect){
	ovrf++; //Increment counter every 256 clock cycles
	display[0] = time/100;
	display[1] = (time%100)/10;
	display[2] = time%10;

	ShiftDigitalWritePort(~(1 << led), 1);
	ShiftDigitalWritePort(seg_table[display[led]], 0);
	led++;
	if(led == 3) led = 0;
		
	strobLatch();
}

uint32_t millis(){
	unsigned long x;
	asm("cli");
	x = ovrf / 37;
	asm("sei");
	return x;
}

void timer0(){
	TIMSK0 &= ~(1 << TOIE0);
	ovrf = 0;
	TIMSK0 |= (1 << TOIE0);
}

uint16_t readADC(uint32_t channel) {
	uint8_t l,h;
	ADMUX = (ADMUX & (1 << REFS0)) | channel & 3; //Setup ADC, preserve REFS0
	ADCSRA |= ((1 <<  ADSC) | (1 << ADEN));
	while(ADCSRA & (1<<ADSC)); //Wait for conversion
	l = ADCL;  //Read and return 10 bit result
	h = ADCH;
	return (h << 8)|l;
}

int main(void)
{
	ShiftRegisterInit();
	//Setup timer interrupt and PWM pins
	TCCR0B |= (1 << CS00);
	TCCR0A |= (1 << WGM00) | (1 << WGM01);
	TCNT0 = 0;
	TIMSK0 |= (1 << TOIE0);
	
	DDRB |= (1 << PB1);
	PORTB &=~ (1 << PB1);
	DDRB &=~ (1 << PB0);
	PORTB |= (1 << PB0);
	
	asm("sei");

    while(1)
    {			
		adc_value = readADC(0);
        uint32_t time_millis = millis();
		if(started && (time_millis > timer+1000)){
			time--;
			if(time == 0){
				started = !started;
				timer0();
				PORTB &=~ (1 << PB1);
			}
			timer = time_millis;
		}		
		
		if((adc_value > 678 && adc_value < 687) && (time_millis > time_delay_start+1000)){
			started = !started;
			timer0();
            if(started){
				PORTB |= (1 << PB1);
			}
			time_delay_start = time_millis;
		}

		if(!started){
			if(adc_value < 20 && (time_millis > time_delay_first+300)){
				if(time != 990){
					time += 15;
					} else {
					time = 0;
				}
				time_delay_first = time_millis;
			}
			
			if((adc_value > 505 && adc_value < 515) && (time_millis > time_delay_two+300)){
				if(time > 0){
					time -= 15;
					} else {
					time = 990;
				}
				time_delay_two = time_millis;
			}
		}
    }
}