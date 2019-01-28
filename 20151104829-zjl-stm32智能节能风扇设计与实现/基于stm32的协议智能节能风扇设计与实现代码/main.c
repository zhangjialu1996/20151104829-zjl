#include <reg52.h> 
#include <intrins.h>
#include <stdio.h>
//#include  <math.h>    //Keil library  
#include "delay.h"
#define OPEN 0x22
#define CLOSE 0x33

unsigned char Rxd_Busy=0;			  //串口判是否跳转
unsigned char record=0;		   //记录串口结束数据长度
unsigned char Tmepbuf[5];		//暂存串口接收数据

unsigned char BasicLater=0;		   //pwm对比时间基础
unsigned char UPLater=0;		  //pwm风扇关闭时间节点
unsigned char DOWNLater=0;		 //PWM风扇打开时间节点
unsigned char ShanFlag=CLOSE;	 //风扇状态

unsigned char DelayStop=0;	//延时停止状态
unsigned long DelayTimN=0;		//延时时间记录

sbit FengShan=P1^0;			  //定义风扇引脚

#define OpenFengshan (FengShan=0)			//打开风扇
#define CloseFengshan (FengShan=1)			//关闭风扇

void Init_Timer0(void);//定时器初始化
void UART_Init(void);

void main (void)
{

	Init_Timer0();        //定时器0初始化
	UART_Init();		   //蓝牙 串口 波特率9600

	DelayMs(10);          //延时有助于稳定

	ShanFlag=OPEN;		//风扇状态
	BasicLater=0;		//pwm对比时间基础
	DOWNLater=7;		 //PWM风扇打开时间节点
	UPLater=12;			 //pwm风扇关闭时间节点
	DelayTimN=0;		//延时时间记录
	DelayStop=0;		//延时停止状态

	while(1)         //主循环
	{
			;//蓝牙的接收处理 均在中断中处理 请查看串口中断
	}
}

void Init_Timer0(void)
{
	TMOD |= 0x01;	  //使用模式1，16位定时器，使用"|"符号可以在使用多个定时器时不受影响		     
	TH0=(65536-10000)/256;		  //重新赋值 20ms
	TL0=(65536-10000)%256;
	EA=1;            //总中断打开
	ET0=1;           //定时器中断打开
	TR0=1;           //定时器开关打开
}

void Timer0_isr(void) interrupt 1 
{
	unsigned char i;
	TH0=(65536-5000)/256;		  //重新赋值 5ms
	TL0=(65536-5000)%256;
	if(ShanFlag==OPEN)	  		//如果是打开风扇 该变量在串口中断中置位的 
	{
		BasicLater++;					//对比基础自加
		if(BasicLater<=DOWNLater)	   //小于 打开风扇
		{
			OpenFengshan;
		}
		else if((BasicLater>=DOWNLater)&&(BasicLater<=UPLater))//中间 关闭风扇
		{
			CloseFengshan;
		}
		else if(BasicLater>UPLater)	   //超出范围 清除 即一个周期
		{
		  	BasicLater=0;
		}
		if(DelayStop==1)	   		//如果延时关闭
		{
			DelayTimN++;			//记录延时次数 每次5ms
			if (DelayTimN>=12000)	   //记录1200次 约1min关闭
			{
		 		CloseFengshan;
				ShanFlag=CLOSE;			  //关闭 并初始化参数
				DelayStop=0;
				DelayTimN=0;
			}

		}
	
	}
	Rxd_Busy++;
	if(Rxd_Busy>=20)		   //判忙判断 防止串口接收出现紊乱
	{
		record=0;
		for(i=0;i<5;i++)	//循环	
		{
		 	Tmepbuf[i]=0x00;   //清空数据
		}
	}

}

void UART_Init(void)
{
    SCON  = 0x50;		        // SCON: 模式 1, 8-bit UART, 使能接收  
    TMOD |= 0x20;               // TMOD: timer 1, mode 2, 8-bit 重装
    TH1   = 0xFD;               // TH1:  重装值 9600 波特率 晶振 11.0592MHz
	TL1 = TH1;  
    TR1   = 1;                  // TR1:  timer 1 打开                         
    EA    = 1;                  //打开总中断
    ES    = 1;                  //打开串口中断
} 

void UART_SER (void) interrupt 4 	//串行中断服务程序
{
	unsigned char R_buf;
	Rxd_Busy=0;
	if(RI)                        //判断是接收中断产生
	{
		RI=0;                      //标志位清零
		R_buf=SBUF;
		Tmepbuf[record]=R_buf;		  //存储接收
		record++;
		if(record>=4)
		{
			if((Tmepbuf[0]=='T')&&(Tmepbuf[1]=='Z')&&(Tmepbuf[2]=='0')&&(Tmepbuf[3]=='0'))		//如果接收到TZ00 关闭风扇
			{
			 		CloseFengshan;
					ShanFlag=CLOSE;
			}
			else if((Tmepbuf[0]=='G')&&(Tmepbuf[1]=='S')&&(Tmepbuf[2]=='0'))	   //如果接收到GS0	 下同
			{
					ShanFlag=OPEN;							 //打开 并初始化PWM相关参数		   下同
					BasicLater=0;
					DOWNLater=15;  //pwm调整参数
					UPLater=12;
					DelayTimN=0;
					if(Tmepbuf[3]=='0')		//接到到的是00 表示长期有效				   下同
					{
						DelayStop=0;   //延时停止标志
					}
					else if(Tmepbuf[3]=='1')	 //接收到的是01 表示1min后停止 标志位置位	下同
					{
						DelayStop=1;   //延时停止标志
					}
			}
			else if((Tmepbuf[0]=='Z')&&(Tmepbuf[1]=='S')&&(Tmepbuf[2]=='0'))
			{
					ShanFlag=OPEN;	//风扇状态
					BasicLater=0;
					DOWNLater=9;   //pwm调整参数
					UPLater=12;
					DelayTimN=0;
					if(Tmepbuf[3]=='0')	 //数据最后是0
					{
						DelayStop=0;	 //延时停止标志
					}
					else if(Tmepbuf[3]=='1')  //数据最后是1
					{
						DelayStop=1;   //延时停止标志
					}
			}
			else if((Tmepbuf[0]=='D')&&(Tmepbuf[1]=='S')&&(Tmepbuf[2]=='0'))  //数据是DS0
			{
					ShanFlag=OPEN;	//风扇状态
					BasicLater=0;
					DOWNLater=7;    //pwm调整参数
					UPLater=12;
					DelayTimN=0;
					if(Tmepbuf[3]=='0')	//数据最后是1
					{
						DelayStop=0;	//延时停止标志
					}
					else if(Tmepbuf[3]=='1') //数据是1
					{
						DelayStop=1;   //延时停止标志
					}
			}
		}
		SBUF=R_buf;
		
	}
	if(TI)  //如果是发送标志位，清零
	TI=0;
} 

