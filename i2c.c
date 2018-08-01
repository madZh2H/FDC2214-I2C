#include "userconfig.h"

//7.7 made by ZH
//soft i2c
void I2C_Start(void)
{
    DELAY_US(4);
    SDA_OUT();
    SDA_H();
    SCL_H();
    DELAY_US(1);
    SDA_L();
    DELAY_US(2);
    SCL_L();
}

void I2C_Stop(void)
{
    DELAY_US(5);
    SDA_OUT();
    SDA_L();
    SCL_H();
    DELAY_US(5);
    SDA_H();
    DELAY_US(3);
}

void I2C_SendACK(uchar ack)
{
    DELAY_US(3);
    SDA_OUT();
    if(ack)
        SDA_H();
    else
        SDA_L();
    SCL_H();
    DELAY_US(3);
    SCL_L();
}

uchar I2C_RecvACK(void)
{
    uchar cy;
    DELAY_US(2);
    SDA_IN();
    DELAY_US(3);
    SCL_H();
    if(SDA_DAT())
    {
        cy=1;
    }
    else
    {
      cy=0;
    }
    DELAY_US(3);
    SCL_L();
    return cy;
}

void I2C_SendByte(uchar dat)
{
    uchar i;
    SDA_OUT();
    for (i=0; i<8; i++)
    {
        DELAY_US(2);
        if((dat<<i)&0x80)
        {
            SDA_H();
        }
        else
        {
            SDA_L();
        }
        DELAY_US(3);
        SCL_H();
        DELAY_US(3);
        SCL_L();
    }
    I2C_RecvACK();
}

uchar I2C_RecvByte(void)
{
    uchar i;
    uchar dat = 0,cy;
    SDA_IN();
    for (i=0; i<8; i++)
    {
        DELAY_US(5);
        dat <<= 1;
        SCL_H();
        DELAY_US(3);
        if(SDA_DAT())
        {
            cy=1;
        }
        else
        {
          cy=0;
        }
        dat |= cy;
        SCL_L();
    }
    SDA_OUT();
    return dat;
}

void ByteWrite(uchar Slave_Address,uchar REG_Address,uchar REG_data)
{
    I2C_Start();
    I2C_SendByte(Slave_Address);
    I2C_SendByte(REG_Address);
    I2C_SendByte(REG_data);
    I2C_Stop();
}

void WordWrite(uchar Slave_Address,uchar REG_Address,uchar REG_dataM,uchar REG_dataL)
{
    I2C_Start();
    I2C_SendByte(Slave_Address);
    I2C_SendByte(REG_Address);
    I2C_SendByte(REG_dataM);
    I2C_SendByte(REG_dataL);
    I2C_Stop();
}

uchar ByteRead(uchar Slave_Address,uchar REG_Address)
{
    uchar REG_data;
    I2C_Start();
    I2C_SendByte(Slave_Address);
    I2C_SendByte(REG_Address);
    I2C_Start();
    I2C_SendByte(Slave_Address+1);
    REG_data=I2C_RecvByte();
    I2C_SendACK(1);
    I2C_Stop();
    return REG_data;
}

int WordRead(uchar Slave_Address,uchar REG_Address)
{
    char H,L;
    I2C_Start();
    I2C_SendByte(Slave_Address);
    I2C_SendByte(REG_Address);
    I2C_Start();
    I2C_SendByte(Slave_Address+1);
    H=I2C_RecvByte();
    I2C_SendACK(0);
    L=I2C_RecvByte();
    I2C_SendACK(1);
    I2C_Stop();
    return (H<<8)+L;
}

//hard i2c made by ZH 7.8

unsigned char TXByteCtr;
unsigned char *PTxData;

void I2C_Init(uchar Slave_Address)
{
    P3SEL |= 0x03;                            // Assign I2C pins to USCI_B0
    UCB0CTL1 |= UCSWRST;                      // Enable SW reset
    UCB0CTL0 = UCMST + UCMODE_3 + UCSYNC;     // I2C Master, synchronous mode
    UCB0CTL1 = UCSSEL_2 + UCSWRST;            // Use SMCLK, keep SW reset
    UCB0BR0 = 12;                             // fSCL = SMCLK/12 = ~100kHz
    UCB0BR1 = 0;
    UCB0I2CSA = Slave_Address;        // Slave Address is 048h
    UCB0CTL1 &= ~UCSWRST;           // Clear SW reset, resume operation
}

void I2C_Hardware_SendByte(uchar REG_Address,uchar REG_data)
{
    while( UCB0CTL1& UCTXSTP );
    UCB0CTL1 |= UCTR;
    UCB0CTL1 |= UCTXSTT;

    UCB0TXBUF = REG_Address;
    while(!(UCB0IFG& UCTXIFG));

    UCB0TXBUF = REG_data;
    while(!(UCB0IFG& UCTXIFG));

    UCB0CTL1 |= UCTXSTP;
    while(UCB0CTL1& UCTXSTP);
}

uchar I2C_Hardware_ReadByte(uchar REG_Address)
{
    uchar REG_data;

    while( UCB0CTL1& UCTXSTP );
    UCB0CTL1 |= UCTR;
    UCB0CTL1 |= UCTXSTT;

    UCB0TXBUF = REG_Address;
    while(!(UCB0IFG& UCTXIFG));

    UCB0CTL1 &= ~UCTR;
    UCB0CTL1 |= UCTXSTT;

    while(UCB0CTL1& UCTXSTT);
    UCB0CTL1 |= UCTXSTP;

    while(!(UCB0IFG& UCRXIFG));
    REG_data = UCB0RXBUF;

    return REG_data;
}

int I2C_Hardware_ReadWord(uchar REG_Address)
{
    char H,L;
    H=I2C_Hardware_ReadByte(REG_Address);
    L=I2C_Hardware_ReadByte(REG_Address+1);
    return (H<<8)+L;
}



#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector = USCI_B0_VECTOR
__interrupt void USCI_B0_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(USCI_B0_VECTOR))) USCI_B0_ISR (void)
#else
#error Compiler not supported!
#endif
{
  switch(__even_in_range(UCB0IV,12))
  {
  case  0: break;                           // Vector  0: No interrupts
  case  2: break;                           // Vector  2: ALIFG
  case  4: break;                           // Vector  4: NACKIFG
  case  6:                                      // Vector  6: STTIFG
      UCB0IFG &= ~UCSTTIFG;
      break;
  case  8:                                      // Vector  8: STPIFG
      UCB0IFG &= ~UCSTPIFG;
      break;
  case 10: break;                           // Vector 10: RXIFG
  case 12:                                  // Vector 12: TXIFG
      UCB0TXBUF = *PTxData;
      UCB0IFG &= ~UCTXIFG;
    break;
  default: break;
  }
}
