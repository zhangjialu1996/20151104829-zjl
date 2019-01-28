#include <reg52.h> 
#include <intrins.h>
#include <stdio.h>
//#include  <math.h>    //Keil library  
#include "delay.h"
#define OPEN 0x22
#define CLOSE 0x33

unsigned char Rxd_Busy=0;			  //�������Ƿ���ת
unsigned char record=0;		   //��¼���ڽ������ݳ���
unsigned char Tmepbuf[5];		//�ݴ洮�ڽ�������

unsigned char BasicLater=0;		   //pwm�Ա�ʱ�����
unsigned char UPLater=0;		  //pwm���ȹر�ʱ��ڵ�
unsigned char DOWNLater=0;		 //PWM���ȴ�ʱ��ڵ�
unsigned char ShanFlag=CLOSE;	 //����״̬

unsigned char DelayStop=0;	//��ʱֹͣ״̬
unsigned long DelayTimN=0;		//��ʱʱ���¼

sbit FengShan=P1^0;			  //�����������

#define OpenFengshan (FengShan=0)			//�򿪷���
#define CloseFengshan (FengShan=1)			//�رշ���

void Init_Timer0(void);//��ʱ����ʼ��
void UART_Init(void);

void main (void)
{

	Init_Timer0();        //��ʱ��0��ʼ��
	UART_Init();		   //���� ���� ������9600

	DelayMs(10);          //��ʱ�������ȶ�

	ShanFlag=OPEN;		//����״̬
	BasicLater=0;		//pwm�Ա�ʱ�����
	DOWNLater=7;		 //PWM���ȴ�ʱ��ڵ�
	UPLater=12;			 //pwm���ȹر�ʱ��ڵ�
	DelayTimN=0;		//��ʱʱ���¼
	DelayStop=0;		//��ʱֹͣ״̬

	while(1)         //��ѭ��
	{
			;//�����Ľ��մ��� �����ж��д��� ��鿴�����ж�
	}
}

void Init_Timer0(void)
{
	TMOD |= 0x01;	  //ʹ��ģʽ1��16λ��ʱ����ʹ��"|"���ſ�����ʹ�ö����ʱ��ʱ����Ӱ��		     
	TH0=(65536-10000)/256;		  //���¸�ֵ 20ms
	TL0=(65536-10000)%256;
	EA=1;            //���жϴ�
	ET0=1;           //��ʱ���жϴ�
	TR0=1;           //��ʱ�����ش�
}

void Timer0_isr(void) interrupt 1 
{
	unsigned char i;
	TH0=(65536-5000)/256;		  //���¸�ֵ 5ms
	TL0=(65536-5000)%256;
	if(ShanFlag==OPEN)	  		//����Ǵ򿪷��� �ñ����ڴ����ж�����λ�� 
	{
		BasicLater++;					//�ԱȻ����Լ�
		if(BasicLater<=DOWNLater)	   //С�� �򿪷���
		{
			OpenFengshan;
		}
		else if((BasicLater>=DOWNLater)&&(BasicLater<=UPLater))//�м� �رշ���
		{
			CloseFengshan;
		}
		else if(BasicLater>UPLater)	   //������Χ ��� ��һ������
		{
		  	BasicLater=0;
		}
		if(DelayStop==1)	   		//�����ʱ�ر�
		{
			DelayTimN++;			//��¼��ʱ���� ÿ��5ms
			if (DelayTimN>=12000)	   //��¼1200�� Լ1min�ر�
			{
		 		CloseFengshan;
				ShanFlag=CLOSE;			  //�ر� ����ʼ������
				DelayStop=0;
				DelayTimN=0;
			}

		}
	
	}
	Rxd_Busy++;
	if(Rxd_Busy>=20)		   //��æ�ж� ��ֹ���ڽ��ճ�������
	{
		record=0;
		for(i=0;i<5;i++)	//ѭ��	
		{
		 	Tmepbuf[i]=0x00;   //�������
		}
	}

}

void UART_Init(void)
{
    SCON  = 0x50;		        // SCON: ģʽ 1, 8-bit UART, ʹ�ܽ���  
    TMOD |= 0x20;               // TMOD: timer 1, mode 2, 8-bit ��װ
    TH1   = 0xFD;               // TH1:  ��װֵ 9600 ������ ���� 11.0592MHz
	TL1 = TH1;  
    TR1   = 1;                  // TR1:  timer 1 ��                         
    EA    = 1;                  //�����ж�
    ES    = 1;                  //�򿪴����ж�
} 

void UART_SER (void) interrupt 4 	//�����жϷ������
{
	unsigned char R_buf;
	Rxd_Busy=0;
	if(RI)                        //�ж��ǽ����жϲ���
	{
		RI=0;                      //��־λ����
		R_buf=SBUF;
		Tmepbuf[record]=R_buf;		  //�洢����
		record++;
		if(record>=4)
		{
			if((Tmepbuf[0]=='T')&&(Tmepbuf[1]=='Z')&&(Tmepbuf[2]=='0')&&(Tmepbuf[3]=='0'))		//������յ�TZ00 �رշ���
			{
			 		CloseFengshan;
					ShanFlag=CLOSE;
			}
			else if((Tmepbuf[0]=='G')&&(Tmepbuf[1]=='S')&&(Tmepbuf[2]=='0'))	   //������յ�GS0	 ��ͬ
			{
					ShanFlag=OPEN;							 //�� ����ʼ��PWM��ز���		   ��ͬ
					BasicLater=0;
					DOWNLater=15;  //pwm��������
					UPLater=12;
					DelayTimN=0;
					if(Tmepbuf[3]=='0')		//�ӵ�������00 ��ʾ������Ч				   ��ͬ
					{
						DelayStop=0;   //��ʱֹͣ��־
					}
					else if(Tmepbuf[3]=='1')	 //���յ�����01 ��ʾ1min��ֹͣ ��־λ��λ	��ͬ
					{
						DelayStop=1;   //��ʱֹͣ��־
					}
			}
			else if((Tmepbuf[0]=='Z')&&(Tmepbuf[1]=='S')&&(Tmepbuf[2]=='0'))
			{
					ShanFlag=OPEN;	//����״̬
					BasicLater=0;
					DOWNLater=9;   //pwm��������
					UPLater=12;
					DelayTimN=0;
					if(Tmepbuf[3]=='0')	 //���������0
					{
						DelayStop=0;	 //��ʱֹͣ��־
					}
					else if(Tmepbuf[3]=='1')  //���������1
					{
						DelayStop=1;   //��ʱֹͣ��־
					}
			}
			else if((Tmepbuf[0]=='D')&&(Tmepbuf[1]=='S')&&(Tmepbuf[2]=='0'))  //������DS0
			{
					ShanFlag=OPEN;	//����״̬
					BasicLater=0;
					DOWNLater=7;    //pwm��������
					UPLater=12;
					DelayTimN=0;
					if(Tmepbuf[3]=='0')	//���������1
					{
						DelayStop=0;	//��ʱֹͣ��־
					}
					else if(Tmepbuf[3]=='1') //������1
					{
						DelayStop=1;   //��ʱֹͣ��־
					}
			}
		}
		SBUF=R_buf;
		
	}
	if(TI)  //����Ƿ��ͱ�־λ������
	TI=0;
} 

