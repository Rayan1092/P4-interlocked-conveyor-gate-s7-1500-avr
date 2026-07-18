#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

volatile uint32_t mills_num = 0;

void servoInit(void)
{
    DDRB |= (1 << DDB1);

    TCCR1A |= (1 << WGM11);
    TCCR1B |= (1 << WGM12) | (1 << WGM13);

    TCCR1A |= (1 << COM1A1);

    TCCR1B |= (1 << CS11);

    ICR1 = 39999;
}

void timerInit(void)
{
    TCCR0A |= (1 << WGM01);

    OCR0A = 249;

    TIMSK0 |= (1 << OCIE0A);

    TCCR0B |= (1 << CS00) | (1 << CS01);

    sei();
}

uint32_t mills(void)
{
    cli();
    uint32_t m = mills_num;
    sei();

    return m;
}

void servoWrite(char pos)
{
    if (pos == 'h')
        OCR1A = 3110;
    else
        OCR1A = 2000;
}

int main(void)
{

    servoInit();
    timerInit();
    // strobe 3 data 4
    DDRD &= ~((1 << DDD3) | (1 << DDD4));

    // ack pin
    DDRD |= (1 << DDD5);

    // ultra echo 6 & ultra trig 7
    DDRD &= ~(1 << DDD6);
    DDRD |= (1 << DDD7);

    // ultra detected servo arm alert pin
    DDRD |= (1 << DDD2);

    uint8_t dataMask = (1 << PIND4);
    uint8_t strobeMask = (1 << PIND3);

    bool entered = false;
    uint32_t enteredTime = mills();

    uint8_t prevStrobeVal = 0;

    int timeEchoUs = 0;

    uint8_t echoPinMask = (1 << PIND6);

    while (true)
    {
        timeEchoUs = 0;
        uint8_t strobeVal = PIND & strobeMask;
        uint8_t dataVal = PIND & dataMask;

        if (!prevStrobeVal && strobeVal)
        {
            entered = true;

            if (dataVal)
                servoWrite('h');
            else
                servoWrite('b');
        }

        // ack
        if (entered)
        {
            PORTD |= (1 << PORTD5);

            if (mills() - enteredTime >= 100)
            {
                entered = false;
                PORTD &= ~(1 << PORTD5);
                enteredTime = mills();
            }
        }
        else
            enteredTime = mills();

        // ultra part
        PORTD |= (1 << PORTD7);
        _delay_us(10);
        PORTD &= ~(1 << PORTD7);

        while (PIND & echoPinMask && timeEchoUs < 6000)
        {
            timeEchoUs++;
            _delay_us(1);
        }

        if (timeEchoUs < 900)
            PORTD |= (1 << PORTD2);
        else
            PORTD &= ~(1 << PORTD2);

        prevStrobeVal = strobeVal;
    }
}

ISR(TIMER0_COMPA_vect)
{
    mills_num++;
}
