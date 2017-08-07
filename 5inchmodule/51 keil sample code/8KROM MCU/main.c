#include <reg51.h>
#include <intrins.h>
#include <sys\sys.h>
#include <lcd\lcd.h>
#include <font\font.h>
/* http://ttmcu.taobao.com 雨亭电子
代码测试环境:JME-2核心板+1T指令周期的STC单片机（51内核STC12LE5A60S2)+33M晶振	  单片机工作电压3.3V
程序默认IO连接方式：
控制线：RS-P3^5;    WR-P3^6;   RD-P3^7;   CS-P1^0;   REST-P1^2;
数据线: DB0-DB7依次连接P0^0-P0^7;  DB8-DB15依次连接P2^0-P2^7;（8位模式下DB0-DB7可以不连接）
触摸功能连接方式：(不使用触摸可不连接)
D_CLK-P1^7;  D_CS-P1^4;  D_DIN-P3^0;  D_OUT-P3^1;  D_PENIRQ-P3^4;
*/	
main()
{ 
	while(1)
	{
	    Lcd_Init();   //tft初始化
		LCD_Clear(RED);
		delayms(3000);
		LCD_Clear(GREEN);
		delayms(3000);
		LCD_Clear(BLUE);
		delayms(3000);
    }


}
