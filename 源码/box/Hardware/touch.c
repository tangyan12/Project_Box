#include "touch.h" 
#include "lcd.h"
#include "MyDelay.h"
#include "stdlib.h"
#include "math.h"
#include "24cxx.h" 
//抽象数据结构体初始化
_m_tp_dev tp_dev=
{
	TP_Init,
	TP_Scan,
	TP_Adjust,
	0,
	0, 
	0,
	0,
	0,
	0,	  	 		
	0,
	0,	  	 		
};					
//默认为touchtype=0的数据
//DA转换器的命令字，电阻屏触电输出该触电的电压，然后DA转换得到坐标值
//到底是得到物理坐标转换为电压还是电压转换为坐标
u8 CMD_RDX=0XD0;
u8 CMD_RDY=0X90;

//SPI发送接收
	 			    					   
//SPI写数据
//向触摸屏IC写入1byte数据    
//num:要写入的数据
void TP_Write_Byte(u8 num)    
{  
	u8 count=0;   
	for(count=0;count<8;count++)  
	{ 	  
		if(num&0x80)TDIN=1;  
		else TDIN=0;   
		num<<=1;    
		TCLK=0; 
		delay_us(1);
		TCLK=1;		//上升沿有效	        
	}		 			    
} 		 
//SPI读数据 
//从触摸屏IC读取adc值
//CMD:指令
//返回值:读到的数据	   
u16 TP_Read_AD(u8 CMD)	  
{ 	 
	u8 count=0; 	  
	u16 Num=0; 
	TCLK=0;		//先拉低时钟 	 
	TDIN=0; 	//拉低数据线
	TCS=0; 		//选中触摸屏IC
	TP_Write_Byte(CMD);//发送命令字
	delay_us(6);//ADS7846的转换时间最长为6us
	TCLK=0; 	     	    
	delay_us(1);    	   
	TCLK=1;		//给1个时钟，清除BUSY
	delay_us(1);    
	TCLK=0; 	     	    
	for(count=0;count<16;count++)//读出16位数据,只有高12位有效 
	{ 				  
		Num<<=1; 	 
		TCLK=0;	//下降沿有效  	    	   
		delay_us(1);    
 		TCLK=1;
 		if(DOUT)Num++; 		 
	}  	
	Num>>=4;   	//只有高12位有效.
	TCS=1;		//释放片选	 
	return(Num);   
}
//读取一个坐标值(x或者y)
//连续读取READ_TIMES次数据,对这些数据升序排列,
//然后去掉最低和最高LOST_VAL个数,取平均值 
//xy:指令（CMD_RDX/CMD_RDY）
//返回值:读到的数据
#define READ_TIMES 5 	//读取次数
#define LOST_VAL 1	  	//丢弃值
u16 TP_Read_XOY(u8 xy)
{
	u16 i, j;
	u16 buf[READ_TIMES];
	u16 sum=0;
	u16 temp;
	for(i=0;i<READ_TIMES;i++)buf[i]=TP_Read_AD(xy);		 		    
	for(i=0;i<READ_TIMES-1; i++)//冒泡排序
	{
		for(j=i+1;j<READ_TIMES;j++)
		{
			if(buf[i]>buf[j])//升序排列
			{
				temp=buf[i];
				buf[i]=buf[j];
				buf[j]=temp;
			}
		}
	}	  
	sum=0;
	//去掉最低值和最高值，不用删去元素，而是改变入结点和出节点，可以学习一下
	for(i=LOST_VAL;i<READ_TIMES-LOST_VAL;i++)sum+=buf[i];
	temp=sum/(READ_TIMES-2*LOST_VAL);
	return temp;   
} 
//读取x,y坐标
//最小值不能少于100.
//x,y:读取到的坐标值
//返回值:0,失败;1,成功。
u8 TP_Read_XY(u16 *x,u16 *y)
{
	u16 xtemp,ytemp;			 	 		  
	xtemp=TP_Read_XOY(CMD_RDX);
	ytemp=TP_Read_XOY(CMD_RDY);	  												   
	//if(xtemp<100||ytemp<100)return 0;//读数失败
	*x=xtemp;
	*y=ytemp;
	return 1;//读数成功
}
//连续2次读取触摸屏IC,且这两次的偏差不能超过
//ERR_RANGE,满足条件,则认为读数正确,否则读数错误.	   
//该函数能大大提高准确度
//x,y:读取到的坐标值
//返回值:0,失败;1,成功。
#define ERR_RANGE 50 //误差范围 
u8 TP_Read_XY2(u16 *x,u16 *y) 
{
	u16 x1,y1;
 	u16 x2,y2;
 	u8 flag;    
    flag=TP_Read_XY(&x1,&y1);   
    if(flag==0)return(0);
    flag=TP_Read_XY(&x2,&y2);	   
    if(flag==0)return(0);   
	//一大一小，大的不超过小的+ERR_RANGE
    if(((x2<=x1&&x1<x2+ERR_RANGE)||(x1<=x2&&x2<x1+ERR_RANGE))//前后两次采样在+-50内
    &&((y2<=y1&&y1<y2+ERR_RANGE)||(y1<=y2&&y2<y1+ERR_RANGE)))
    {
        *x=(x1+x2)/2;
        *y=(y1+y2)/2;
        return 1;
    }else return 0;	  
}  
//////////////////////////////////////////////////////////////////////////////////		  
//oled的画图形我都没学，这个暂时不管
//与LCD部分有关的函数  
//画一个触摸点
//用来校准用的
//x,y:坐标
//color:颜色
void TP_Drow_Touch_Point(u16 x,u16 y,u16 color)
{
	POINT_COLOR=color;
	LCD_DrawLine(x-12,y,x+13,y);//横线
	LCD_DrawLine(x,y-12,x,y+13);//竖线
	LCD_DrawPoint(x+1,y+1);
	LCD_DrawPoint(x-1,y+1);
	LCD_DrawPoint(x+1,y-1);
	LCD_DrawPoint(x-1,y-1);
	LCD_Draw_Circle(x,y,6);//画中心圈
}	  
//画一个大点(2*2的点)		   
//x,y:坐标
//color:颜色
void TP_Draw_Big_Point(u16 x,u16 y,u16 color)
{	    
	POINT_COLOR=color;
	LCD_DrawPoint(x,y);//中心点 
	LCD_DrawPoint(x+1,y);
	LCD_DrawPoint(x,y+1);
	LCD_DrawPoint(x+1,y+1);	 	  	
}						  
//////////////////////////////////////////////////////////////////////////////////		  
//触摸按键扫描
//tp:0,屏幕坐标;1,物理坐标(校准等特殊场合用)
//返回值:当前触屏状态.
//0,触屏无触摸;1,触屏有触摸

//按下时执行，会根据需求获得接触点的屏幕坐标或者物理坐标
//没按下时执行，如果之前按下过会清除标志，否则将坐标值都清零
u8 TP_Scan(u8 tp)
{			   
	if(PEN==0)//有按键按下
	{
		if(tp)TP_Read_XY2(&tp_dev.x[0],&tp_dev.y[0]);//读取物理坐标
		else if(TP_Read_XY2(&tp_dev.x[0],&tp_dev.y[0]))//读取屏幕坐标
		{
	 		tp_dev.x[0]=tp_dev.xfac*tp_dev.x[0]+tp_dev.xoff;//将结果转换为屏幕坐标
			tp_dev.y[0]=tp_dev.yfac*tp_dev.y[0]+tp_dev.yoff;  
	 	} 
		//sta的最高位用来记录按键有没有按下过，后一位我感觉也是这个功能，不明白为啥要设两个
		if((tp_dev.sta&TP_PRES_DOWN)==0)//之前没有被按下
		{		 
			tp_dev.sta=TP_PRES_DOWN|TP_CATH_PRES;//按键按下  
			tp_dev.x[4]=tp_dev.x[0];//记录第一次按下时的坐标
			tp_dev.y[4]=tp_dev.y[0];  	   			 
		}			   
	}else
	{
		if(tp_dev.sta&TP_PRES_DOWN)//之前是被按下的
		{
			tp_dev.sta&=~(1<<7);//标记按键松开//很奇怪，没有清楚第6位	
		}else//之前就没有被按下
		{
			tp_dev.x[4]=0;
			tp_dev.y[4]=0;
			tp_dev.x[0]=0xffff;
			tp_dev.y[0]=0xffff;
		}	    
	}
	return tp_dev.sta&TP_PRES_DOWN;//返回当前的触屏状态
}	  
//////////////////////////////////////////////////////////////////////////	 
//保存在EEPROM里面的地址区间基址,占用14个字节(RANGE:SAVE_ADDR_BASE~SAVE_ADDR_BASE+13)
#define SAVE_ADDR_BASE 40
//保存校准参数,xfac,yfac,xoff,yoff，并给之后的一个地址赋值，表示已经校准过										    
void TP_Save_Adjdata(void)
{
	AT24CXX_Write(SAVE_ADDR_BASE,(u8*)&tp_dev.xfac,14);	//强制保存&tp_dev.xfac地址开始的14个字节数据，即保存到tp_dev.touchtype
 	AT24CXX_WriteOneByte(SAVE_ADDR_BASE+14,0X0A);		//在最后，写0X0A标记校准过了
}
//得到保存在EEPROM里面的校准值
//返回值：1，成功获取数据
//        0，获取失败，要重新校准
u8 TP_Get_Adjdata(void)
{					  
	u8 temp;
	temp=AT24CXX_ReadOneByte(SAVE_ADDR_BASE+14);//读取标记字,看是否校准过！ 		 
	if(temp==0X0A)//触摸屏已经校准过了			   
 	{ 
		AT24CXX_Read(SAVE_ADDR_BASE,(u8*)&tp_dev.xfac,14);//读取之前保存的校准数据 
		//因为电容屏不需要校准，所以这里判断的是横屏还是竖屏，然后更改xy读取指令
		if(tp_dev.touchtype)//X,Y方向与屏幕相反
		{
			CMD_RDX=0X90;
			CMD_RDY=0XD0;	 
		}else				   //X,Y方向与屏幕相同
		{
			CMD_RDX=0XD0;
			CMD_RDY=0X90;	 
		}		 
		return 1;	 
	}
	return 0;
}	 
//提示字符串
u8* const TP_REMIND_MSG_TBL="Please use the stylus click the cross on the screen.The cross will always move until the screen adjustment is completed.";
 					  
//提示校准结果(各个参数)
void TP_Adj_Info_Show(u16 x0,u16 y0,u16 x1,u16 y1,u16 x2,u16 y2,u16 x3,u16 y3,u16 fac)
{	  
	POINT_COLOR=RED;
	LCD_ShowString(40,160,lcddev.width,lcddev.height,16,"x1:");
 	LCD_ShowString(40+80,160,lcddev.width,lcddev.height,16,"y1:");
 	LCD_ShowString(40,180,lcddev.width,lcddev.height,16,"x2:");
 	LCD_ShowString(40+80,180,lcddev.width,lcddev.height,16,"y2:");
	LCD_ShowString(40,200,lcddev.width,lcddev.height,16,"x3:");
 	LCD_ShowString(40+80,200,lcddev.width,lcddev.height,16,"y3:");
	LCD_ShowString(40,220,lcddev.width,lcddev.height,16,"x4:");
 	LCD_ShowString(40+80,220,lcddev.width,lcddev.height,16,"y4:");  
 	LCD_ShowString(40,240,lcddev.width,lcddev.height,16,"fac is:");     
	LCD_ShowNum(40+24,160,x0,4,16);		//显示数值
	LCD_ShowNum(40+24+80,160,y0,4,16);	//显示数值
	LCD_ShowNum(40+24,180,x1,4,16);		//显示数值
	LCD_ShowNum(40+24+80,180,y1,4,16);	//显示数值
	LCD_ShowNum(40+24,200,x2,4,16);		//显示数值
	LCD_ShowNum(40+24+80,200,y2,4,16);	//显示数值
	LCD_ShowNum(40+24,220,x3,4,16);		//显示数值
	LCD_ShowNum(40+24+80,220,y3,4,16);	//显示数值
 	LCD_ShowNum(40+56,240,fac,3,16); 	//显示数值,该数值必须在95~105范围之内.

}
		 
//触摸屏校准代码
//得到四个校准参数
void TP_Adjust(void)
{								 
	u16 pos_temp[4][2];//坐标缓存值，存储四个点的xy坐标值
	u8  cnt=0;	//记录四个点已测试数目
	u16 d1,d2;	//两个点xy方向之差
	u32 tem1,tem2; 
	double fac; 	
	u16 outtime=0;
 	cnt=0;				
	POINT_COLOR=BLUE;
	BACK_COLOR =WHITE;
	LCD_Clear(WHITE);//清屏   
	POINT_COLOR=RED;//红色 
	LCD_Clear(WHITE);//清屏 	   
	POINT_COLOR=BLACK;

	// LCD_ShowString(40,40,160,100,16,(u8*)TP_REMIND_MSG_TBL);//显示提示信息

	TP_Drow_Touch_Point(20,20,RED);//画点1 
	tp_dev.sta=0;//消除触发信号 
	tp_dev.xfac=0;//xfac用来标记是否校准过,所以校准之前必须清掉!以免错误	
	
	while(1)//如果连续10秒钟没有按下,则自动退出
	{
		tp_dev.scan(1);//扫描物理坐标
		//如果按键有被按下过，在这里清除标志位，且刷新超时时间，记录坐标
		if((tp_dev.sta&0xc0)==TP_CATH_PRES)//按键按下了一次(此时按键松开了.)
		{	
			outtime=0;		
			tp_dev.sta&=~(1<<6);//标记按键已经被处理过了.
						   			   
			pos_temp[cnt][0]=tp_dev.x[0];
			pos_temp[cnt][1]=tp_dev.y[0];
			cnt++;//已经有一个点被测试过了	  
			switch(cnt)
			{			   
				case 1:						 
					TP_Drow_Touch_Point(20,20,WHITE);				//清除点1 
					TP_Drow_Touch_Point(lcddev.width-20,20,RED);	//画点2
					break;
				case 2:
 					TP_Drow_Touch_Point(lcddev.width-20,20,WHITE);	//清除点2
					TP_Drow_Touch_Point(20,lcddev.height-20,RED);	//画点3
					break;
				case 3:
 					TP_Drow_Touch_Point(20,lcddev.height-20,WHITE);			//清除点3
 					TP_Drow_Touch_Point(lcddev.width-20,lcddev.height-20,RED);	//画点4
					break;
				case 4:	 //全部四个点已经得到
	    		    //对边相等
					tem1=abs(pos_temp[0][0]-pos_temp[1][0]);//x1-x2
					tem2=abs(pos_temp[0][1]-pos_temp[1][1]);//y1-y2
					tem1*=tem1;
					tem2*=tem2;
					d1=sqrt(tem1+tem2);//得到1,2的距离
					
					tem1=abs(pos_temp[2][0]-pos_temp[3][0]);//x3-x4
					tem2=abs(pos_temp[2][1]-pos_temp[3][1]);//y3-y4
					tem1*=tem1;
					tem2*=tem2;
					d2=sqrt(tem1+tem2);//得到3,4的距离
					fac=(float)d1/d2;
					//获取的都是物理坐标，理论上fac=1，下面是判断
					if(fac<0.95||fac>1.05||d1==0||d2==0)//不合格
					{
						cnt=0;
 				    	TP_Drow_Touch_Point(lcddev.width-20,lcddev.height-20,WHITE);	//清除点4
   	 					TP_Drow_Touch_Point(20,20,RED);			//画点1
											
 						//TP_Adj_Info_Show(pos_temp[0][0],pos_temp[0][1],pos_temp[1][0],pos_temp[1][1],pos_temp[2][0],pos_temp[2][1],pos_temp[3][0],pos_temp[3][1],fac*100);//显示数据   
						My_LCD_string(3,4,24,"Failed",Virtual);
 						continue;
					}
					tem1=abs(pos_temp[0][0]-pos_temp[2][0]);//x1-x3
					tem2=abs(pos_temp[0][1]-pos_temp[2][1]);//y1-y3
					tem1*=tem1;
					tem2*=tem2;
					d1=sqrt(tem1+tem2);//得到1,3的距离
					
					tem1=abs(pos_temp[1][0]-pos_temp[3][0]);//x2-x4
					tem2=abs(pos_temp[1][1]-pos_temp[3][1]);//y2-y4
					tem1*=tem1;
					tem2*=tem2;
					d2=sqrt(tem1+tem2);//得到2,4的距离
					fac=(float)d1/d2;
					if(fac<0.95||fac>1.05)//不合格
					{
						cnt=0;
 				    	TP_Drow_Touch_Point(lcddev.width-20,lcddev.height-20,WHITE);	//清除点4
   	 					TP_Drow_Touch_Point(20,20,RED);								//画点1
 						//TP_Adj_Info_Show(pos_temp[0][0],pos_temp[0][1],pos_temp[1][0],pos_temp[1][1],pos_temp[2][0],pos_temp[2][1],pos_temp[3][0],pos_temp[3][1],fac*100);//显示数据   
						My_LCD_string(3,4,24,"Failed",Virtual);
						continue;
					}//正确了
								   
					//对角线相等
					tem1=abs(pos_temp[1][0]-pos_temp[2][0]);//x1-x3
					tem2=abs(pos_temp[1][1]-pos_temp[2][1]);//y1-y3
					tem1*=tem1;
					tem2*=tem2;
					d1=sqrt(tem1+tem2);//得到1,4的距离
	
					tem1=abs(pos_temp[0][0]-pos_temp[3][0]);//x2-x4
					tem2=abs(pos_temp[0][1]-pos_temp[3][1]);//y2-y4
					tem1*=tem1;
					tem2*=tem2;
					d2=sqrt(tem1+tem2);//得到2,3的距离
					fac=(float)d1/d2;
					if(fac<0.95||fac>1.05)//不合格
					{
						cnt=0;
 				    	TP_Drow_Touch_Point(lcddev.width-20,lcddev.height-20,WHITE);	//清除点4
   	 					TP_Drow_Touch_Point(20,20,RED);								//画点1
 						//TP_Adj_Info_Show(pos_temp[0][0],pos_temp[0][1],pos_temp[1][0],pos_temp[1][1],pos_temp[2][0],pos_temp[2][1],pos_temp[3][0],pos_temp[3][1],fac*100);//显示数据   
						My_LCD_string(3,4,24,"Failed",Virtual);
						continue;
					}//正确了
					//计算结果
					tp_dev.xfac=(float)(lcddev.width-40)/(pos_temp[1][0]-pos_temp[0][0]);//得到xfac		 
					tp_dev.xoff=(lcddev.width-tp_dev.xfac*(pos_temp[1][0]+pos_temp[0][0]))/2;//得到xoff
						  
					tp_dev.yfac=(float)(lcddev.height-40)/(pos_temp[2][1]-pos_temp[0][1]);//得到yfac
					tp_dev.yoff=(lcddev.height-tp_dev.yfac*(pos_temp[2][1]+pos_temp[0][1]))/2;//得到yoff  
					if(abs(tp_dev.xfac)>2||abs(tp_dev.yfac)>2)//触屏和预设的相反了.
					{
						cnt=0;
 				    	TP_Drow_Touch_Point(lcddev.width-20,lcddev.height-20,WHITE);	//清除点4
   	 					TP_Drow_Touch_Point(20,20,RED);								//画点1
						My_LCD_string(40,26,16,"TP Need readjust!",Fact);
						tp_dev.touchtype=!tp_dev.touchtype;//修改触屏类型.
						if(tp_dev.touchtype)//X,Y方向与屏幕相反
						{
							CMD_RDX=0X90;
							CMD_RDY=0XD0;	 
						}else				   //X,Y方向与屏幕相同
						{
							CMD_RDX=0XD0;
							CMD_RDY=0X90;	 
						}			    
						continue;
					}		
					POINT_COLOR=BLUE;
					LCD_Clear(WHITE);//清屏
					My_LCD_string(35,110,16,"Touch Screen Adjust OK!",Fact);//校正完成
					delay_ms(1000);
					TP_Save_Adjdata();  
 					LCD_Clear(WHITE);//清屏   
					return;//校正完成				 
			}
		}
		delay_ms(10);
		outtime++;
		if(outtime>1000)
		{
			TP_Get_Adjdata();
			break;
	 	} 
 	}
}	 
//触摸屏初始化  		    
//返回值:0,没有进行校准
//       1,进行过校准
u8 TP_Init(void)
{	
	  GPIO_InitTypeDef  GPIO_InitStructure;

		//注意,时钟使能之后,对GPIO的操作才有效
		//所以上拉之前,必须使能时钟.才能实现真正的上拉输出
   	 	
	 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB|RCC_APB2Periph_GPIOF, ENABLE);	 //使能PB,PF端口时钟
		
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;				 // PB1端口配置
	 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 //推挽输出
	 	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	 	GPIO_Init(GPIOB, &GPIO_InitStructure);//B1推挽输出
	 	GPIO_SetBits(GPIOB,GPIO_Pin_1);//上拉
		
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;				 // PB2端口配置
	 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 		 //上拉输入
	 	GPIO_Init(GPIOB, &GPIO_InitStructure);//B2上拉输入
	 	GPIO_SetBits(GPIOB,GPIO_Pin_2);//上拉		
		
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11|GPIO_Pin_9;				 // F9，PF11端口配置
	 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 //推挽输出
	 	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	 	GPIO_Init(GPIOF, &GPIO_InitStructure);//PF9,PF11推挽输出
	 	GPIO_SetBits(GPIOF, GPIO_Pin_11|GPIO_Pin_9);//上拉
		
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;				 // PF10端口配置
	 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 		 //上拉输入
	 	GPIO_Init(GPIOF, &GPIO_InitStructure);//PF10上拉输入
	 	GPIO_SetBits(GPIOF,GPIO_Pin_10);//上拉		
 
		TP_Read_XY(&tp_dev.x[0],&tp_dev.y[0]);//第一次读取初始化	 
		AT24CXX_Init();			//初始化24CXX
		if(TP_Get_Adjdata())return 0;//已经校准
		else			  		//未校准?
		{ 										    
			LCD_Clear(WHITE);	//清屏
			TP_Adjust();  		//屏幕校准  
		}			
		TP_Get_Adjdata();	
	return 1; 									 
}

//画键盘函数
u8* kbd_menu[15]={"Enter","0","Del","1","2","3","4","5","6","7","8","9",};//按键表

void LCD_keyboard_showMiddle(u8 i,u8 *show_string)
{
	u8 show_x=(i%3)*80+(80-(strlen(show_string)*12))/2;
	u16 show_y=(i/3)*30+3+200;
	My_LCD_string(show_y,show_x,24,show_string,Fact);
}

void LCD_keyboard(u8 x,u16 y, u8 **menu)
{

	POINT_COLOR=RED;
	LCD_DrawRectangle(x,y,x+240,y+120);
	LCD_DrawRectangle(x+80,y,x+160,y+120);
	LCD_DrawRectangle(x,y+30,x+240,y+60);
	LCD_DrawLine(0,290,240,290);
	LCD_DrawLine(0,170,240,170);
	My_LCD_CN(170+3,0,"还剩",Fact,BLACK);
	My_LCD_CN(170+3,60,"位",Fact,BLACK);
	POINT_COLOR=BLUE;
	for (u8 i = 0; i < 12; i++)
	{
		LCD_keyboard_showMiddle(i,menu[i]);
	}
}
void LCD_flush(u8 keyx,u8 sta)
{
	
	u8 start_x=(keyx%3)*80+1;
	u16 start_y=(keyx/3)*30+1+200;
	u8 end_x=start_x+80-2;
	u16 end_y=start_y+30-2;
	if (sta==1)
	{
		LCD_Fill(start_x,start_y,end_x,end_y,GREEN);
	}
	else
	{
		LCD_Fill(start_x,start_y,end_x,end_y,WHITE);
	}
	LCD_keyboard_showMiddle(keyx,kbd_menu[keyx]);
}
u8 LCD_get_keynum(void)
{
	u8 key=0;
	static u8 key_last=0;
	tp_dev.scan(0);
	u8 scale_x=0;u16 scale_y=0;

	if (tp_dev.sta&TP_PRES_DOWN)
	{
		for (u8 i = 0; i < 4; i++)
		{
			for (u8 j = 0; j < 3; j++)
			{
				scale_x=j*80;
				scale_y=i*30+200;
				if (tp_dev.x[0]>=scale_x&&tp_dev.x[0]<=scale_x+80&& \
				tp_dev.y[0]>=scale_y&&tp_dev.y[0]<=scale_y+30)
				{
					key=i*3+j+1;
					break;
				}
			}
			if (key)
			{
				if (key!=key_last)
				{
					LCD_flush(key-1,1);
					key_last=key;
					LCD_flush(key-1,0);
				}
				break;
			}
			
		}
		
	}
	else
	{
		key_last=0;
	}
	return key;
}

u8 PWSD[]={"20210909"};
u8 pwsd[9]={0};

void LCD_PWSD(void)
{
	u8 key_num=0;
	static u8 n=0;
	key_num=LCD_get_keynum();
	if (key_num)
	{
		if (key_num!=1&&key_num!=3)
		{
			if (n<8)
			{		
				if (key_num==2)
				{
					pwsd[n]='0';
					n++;
					pwsd[n]='\0';
				}
				else
				{
					pwsd[n]='0'+key_num-3;
					n++;
					pwsd[n]='\0';
				}
				My_LCD_string(170+3,6*24-1,24,pwsd,Fact);
			}
			else
			{
				My_LCD_string(140+3,0,24,"NO MORE SPACE",Fact);
			}
		}
		else if (key_num==3)
		{
			if (n!=0)
			{
				pwsd[n-1]='\0';
				n--;
			}
			My_LCD_string(170+3,6*24-1,24,"        ",Fact);
			My_LCD_string(170+3,6*24-1,24,pwsd,Fact);
		}
		else
		{
			My_LCD_string(170+3,6*24-1,24,"        ",Fact);

			if (strcmp(pwsd,PWSD)==0)
			{
				
				My_LCD_string(140+3,0,24,"SUCCESS",Fact);
			}
			else
			{
				My_LCD_string(140+3,0,24,"WRONG",Fact);
			}
			for (u8 k = 0; k < 9; k++)
			{
				pwsd[k]=0;
			}
			
			n=0;
			delay_ms(500);
			My_LCD_string(140+3,0,24,"        ",Fact);
		}	
	}
	//n的显示
	My_LCD_Num(170+3,48,8-n,24,1,Fact);
}