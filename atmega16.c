/*
 * atmega8.c
 *
 * Created: 6/20/2020 10:06:18 PM
 * Author: trung
 */

#include <mega16.h>
#include <delay.h>
#include <stdio.h>

#define dht_addr 0xB8 

int j = 0;
char receiv = '1';

char value[10];              //--- luu gia tri phan nguyen do am

void WDT_ON(){
    WDTCR = (1 << WDE) | (1 << WDP2) | (1 << WDP1) | (1<<WDP0);
}

void WDT_OFF(){
    WDTCR = (1 << WDTOE)|(1<<WDE);
    WDTCR = 0x00;
}

void init(){
    UBRRH = 0;
    UBRRL = 25; // baud rate = 2400
    
    UCSRA = 0x00;
    UCSRB = (1 << RXEN)|(1 << TXEN)|(1 << RXCIE)|(1 << TXCIE); // transform and recive and interrupt
    UCSRC = (1 << URSEL)|(1 << UCSZ1)|(1 << UCSZ0); // data = 8 bit
    
    
    TCCR1B = (1 << CS12)|(1 << CS10);   // chon prescaler = 1024
    TCNT1 = 65047;    // dat gia tri khoi dau 
    TIMSK = (1 << TOIE1);  // cho phep ngat
                 
    #asm("sei")   // enable interrupt      
  
    // set init for i2c 
    TWBR = 0x20;    /* Get bit rate register value by formula */
    
}


//--- i2c start condition
void I2C_Start(char write_address)/* I2C start function */
{
    TWCR = (1<<TWSTA)|(1<<TWEN)|(1<<TWINT); /* Enable TWI, generate START */
    while(!(TWCR&(1<<TWINT)));    /* Wait until TWI finish its current job */

    TWDR = write_address;        /* Write SLA+W in TWI data register */
    TWCR = (1<<TWEN)|(1<<TWINT);    /* Enable TWI & clear interrupt flag */
    while(!(TWCR&(1<<TWINT)));    /* Wait until TWI finish its current job */
}

void I2C_Repeated_Start(char read_address) /* I2C repeated start function */
{
    TWCR=(1<<TWSTA)|(1<<TWEN)|(1<<TWINT);/* Enable TWI, generate start */
    while(!(TWCR&(1<<TWINT)));    /* Wait until TWI finish its current job */

    TWDR=read_address;        /* Write SLA+R in TWI data register */
    TWCR=(1<<TWEN)|(1<<TWINT);    /* Enable TWI and clear interrupt flag */
    while(!(TWCR&(1<<TWINT)));    /* Wait until TWI finish its current job */
}

void I2C_Write(char data)    /* I2C write function */
{
    TWDR=data;            /* Copy data in TWI data register */
    TWCR=(1<<TWEN)|(1<<TWINT);    /* Enable TWI and clear interrupt flag */
    while(!(TWCR&(1<<TWINT)));    /* Wait until TWI finish its current job */
}

char I2C_Read_Ack()        /* I2C read ack function */
{
    TWCR=(1<<TWEN)|(1<<TWINT)|(1<<TWEA); /* Enable TWI, generation of ack */
    while(!(TWCR&(1<<TWINT)));    /* Wait until TWI finish its current job */
    return TWDR;            /* Return received data */
}

char I2C_Read_Nack()        /* I2C read nack function */
{
    TWCR=(1<<TWEN)|(1<<TWINT);    /* Enable TWI and clear interrupt flag */
    while(!(TWCR&(1<<TWINT)));    /* Wait until TWI finish its current job */
    return TWDR;        /* Return received data */
}

void I2C_Stop()            /* I2C stop function */
{
    TWCR=(1<<TWSTO)|(1<<TWINT)|(1<<TWEN);/* Enable TWI, generate stop */
    while(TWCR&(1<<TWSTO));    /* Wait until stop condition execution */
}


// read data from sensor dht12
void read(){
   /*-------------------------------- Doc do am  ------------------------------------------*/
    I2C_Start(dht_addr); /* Start I2C with device write address */
    I2C_Write(0x00);        /* Write start memory address */ 
    I2C_Repeated_Start(dht_addr | 1);/* Repeat start I2C SLA+R */ 
    value[0] = I2C_Read_Ack();    /* read humidity */
    value[1] = I2C_Read_Nack();   /* read humidity scale */
    I2C_Stop(); 
             
    /*------------------------------- Doc nhiet do -----------------------------------------*/
    I2C_Start(dht_addr); /* Start I2C with device write address */
    I2C_Write(0x02);        /* Write start memory address */    
    I2C_Repeated_Start(dht_addr | 1);/* Repeat start I2C SLA+R */ 
    value[2] = I2C_Read_Ack();    /* read Temperature */
    value[3] = I2C_Read_Nack();   /* read Temperature scale */   
    I2C_Stop();
    /*value[0] = 'a';
    value[1] = 'b';
    value[2] = 'c';
    value[3] = 'd';*/                                                 
    if(UCSRA & (1 << UDRE)){
        UDR = value[j++];
    }
}


//interrupt when transform data
interrupt [USART_TXC] void uart_txc(void)
{
    if(j < 4){
        UDR = value[j++];
    }
}

// interrupt when receive data
interrupt [USART_RXC] void uart_rxc(void)
{
    receiv = UDR; 
    if(receiv == '0'){    
        PORTB = 0xFF;
        j = 0;
        value[0] = value[1] = value[3] = value[2] = 0;
        read(); 
    }    
    receiv = '1';
}

interrupt [TIM1_OVF] void timer_trans(void)
{
    TCNT1 = 65047;   
    j = 0;
    read();
}




void main(void)
{
    init();
    DDRA = 0xFF;  
    DDRB = 0xFF;
    while (1)
        {         
            WDT_ON();
            PORTA = 0xFF;
            delay_ms(100);   
                        WDT_OFF();

            PORTA = 0x00;
            delay_ms(100);  
        }            
}
