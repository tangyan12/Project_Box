#include "touch.h" 
#include "lcd.h"
#include "MyDelay.h"
#include "stdlib.h"
#include "math.h"
#include "24cxx.h" 
//�������ݽṹ���ʼ��
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
//Ĭ��Ϊtouchtype=0������
//DAת�����������֣���������������ô���ĵ�ѹ��Ȼ��DAת���õ�����ֵ
//�����ǵõ���������ת��Ϊ��ѹ���ǵ�ѹת��Ϊ����
u8 CMD_RDX=0XD0;
u8 CMD_RDY=0X90;

//SPI���ͽ���
	 			    					   
//SPIд����
//������ICд��1byte����    
//num:Ҫд�������
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
		TCLK=1;		//��������Ч	        
	}		 			    
} 		 
//SPI������ 
//�Ӵ�����IC��ȡadcֵ
//CMD:ָ��
//����ֵ:����������	   
u16 TP_Read_AD(u8 CMD)	  
{ 	 
	u8 count=0; 	  
	u16 Num=0; 
	TCLK=0;		//������ʱ�� 	 
	TDIN=0; 	//����������
	TCS=0; 		//ѡ�д�����IC
	TP_Write_Byte(CMD);//����������
	delay_us(6);//ADS7846��ת��ʱ���Ϊ6us
	TCLK=0; 	     	    
	delay_us(1);    	   
	TCLK=1;		//��1��ʱ�ӣ����BUSY
	delay_us(1);    
	TCLK=0; 	     	    
	for(count=0;count<16;count++)//����16λ����,ֻ�и�12λ��Ч 
	{ 				  
		Num<<=1; 	 
		TCLK=0;	//�½�����Ч  	    	   
		delay_us(1);    
 		TCLK=1;
 		if(DOUT)Num++; 		 
	}  	
	Num>>=4;   	//ֻ�и�12λ��Ч.
	TCS=1;		//�ͷ�Ƭѡ	 
	return(Num);   
}
//��ȡһ������ֵ(x����y)
//������ȡREAD_TIMES������,����Щ������������,
//Ȼ��ȥ����ͺ����LOST_VAL����,ȡƽ��ֵ 
//xy:ָ�CMD_RDX/CMD_RDY��
//����ֵ:����������
#define READ_TIMES 5 	//��ȡ����
#define LOST_VAL 1	  	//����ֵ
u16 TP_Read_XOY(u8 xy)
{
	u16 i, j;
	u16 buf[READ_TIMES];
	u16 sum=0;
	u16 temp;
	for(i=0;i<READ_TIMES;i++)buf[i]=TP_Read_AD(xy);		 		    
	for(i=0;i<READ_TIMES-1; i++)//ð������
	{
		for(j=i+1;j<READ_TIMES;j++)
		{
			if(buf[i]>buf[j])//��������
			{
				temp=buf[i];
				buf[i]=buf[j];
				buf[j]=temp;
			}
		}
	}	  
	sum=0;
	//ȥ�����ֵ�����ֵ������ɾȥԪ�أ����Ǹı�����ͳ��ڵ㣬����ѧϰһ��
	for(i=LOST_VAL;i<READ_TIMES-LOST_VAL;i++)sum+=buf[i];
	temp=sum/(READ_TIMES-2*LOST_VAL);
	return temp;   
} 
//��ȡx,y����
//��Сֵ��������100.
//x,y:��ȡ��������ֵ
//����ֵ:0,ʧ��;1,�ɹ���
u8 TP_Read_XY(u16 *x,u16 *y)
{
	u16 xtemp,ytemp;			 	 		  
	xtemp=TP_Read_XOY(CMD_RDX);
	ytemp=TP_Read_XOY(CMD_RDY);	  												   
	//if(xtemp<100||ytemp<100)return 0;//����ʧ��
	*x=xtemp;
	*y=ytemp;
	return 1;//�����ɹ�
}
//����2�ζ�ȡ������IC,�������ε�ƫ��ܳ���
//ERR_RANGE,��������,����Ϊ������ȷ,�����������.	   
//�ú����ܴ�����׼ȷ��
//x,y:��ȡ��������ֵ
//����ֵ:0,ʧ��;1,�ɹ���
#define ERR_RANGE 50 //��Χ 
u8 TP_Read_XY2(u16 *x,u16 *y) 
{
	u16 x1,y1;
 	u16 x2,y2;
 	u8 flag;    
    flag=TP_Read_XY(&x1,&y1);   
    if(flag==0)return(0);
    flag=TP_Read_XY(&x2,&y2);	   
    if(flag==0)return(0);   
	//һ��һС����Ĳ�����С��+ERR_RANGE
    if(((x2<=x1&&x1<x2+ERR_RANGE)||(x1<=x2&&x2<x1+ERR_RANGE))//ǰ�����β�����+-50��
    &&((y2<=y1&&y1<y2+ERR_RANGE)||(y1<=y2&&y2<y1+ERR_RANGE)))
    {
        *x=(x1+x2)/2;
        *y=(y1+y2)/2;
        return 1;
    }else return 0;	  
}  
//////////////////////////////////////////////////////////////////////////////////		  
//oled�Ļ�ͼ���Ҷ�ûѧ�������ʱ����
//��LCD�����йصĺ���  
//��һ��������
//����У׼�õ�
//x,y:����
//color:��ɫ
void TP_Drow_Touch_Point(u16 x,u16 y,u16 color)
{
	POINT_COLOR=color;
	LCD_DrawLine(x-12,y,x+13,y);//����
	LCD_DrawLine(x,y-12,x,y+13);//����
	LCD_DrawPoint(x+1,y+1);
	LCD_DrawPoint(x-1,y+1);
	LCD_DrawPoint(x+1,y-1);
	LCD_DrawPoint(x-1,y-1);
	LCD_Draw_Circle(x,y,6);//������Ȧ
}	  
//��һ�����(2*2�ĵ�)		   
//x,y:����
//color:��ɫ
void TP_Draw_Big_Point(u16 x,u16 y,u16 color)
{	    
	POINT_COLOR=color;
	LCD_DrawPoint(x,y);//���ĵ� 
	LCD_DrawPoint(x+1,y);
	LCD_DrawPoint(x,y+1);
	LCD_DrawPoint(x+1,y+1);	 	  	
}						  
//////////////////////////////////////////////////////////////////////////////////		  
//��������ɨ��
//tp:0,��Ļ����;1,��������(У׼�����ⳡ����)
//����ֵ:��ǰ����״̬.
//0,�����޴���;1,�����д���

//����ʱִ�У�����������ýӴ������Ļ���������������
//û����ʱִ�У����֮ǰ���¹��������־����������ֵ������
u8 TP_Scan(u8 tp)
{			   
	if(PEN==0)//�а�������
	{
		if(tp)TP_Read_XY2(&tp_dev.x[0],&tp_dev.y[0]);//��ȡ��������
		else if(TP_Read_XY2(&tp_dev.x[0],&tp_dev.y[0]))//��ȡ��Ļ����
		{
	 		tp_dev.x[0]=tp_dev.xfac*tp_dev.x[0]+tp_dev.xoff;//�����ת��Ϊ��Ļ����
			tp_dev.y[0]=tp_dev.yfac*tp_dev.y[0]+tp_dev.yoff;  
	 	} 
		//sta�����λ������¼������û�а��¹�����һλ�Ҹо�Ҳ��������ܣ�������ΪɶҪ������
		if((tp_dev.sta&TP_PRES_DOWN)==0)//֮ǰû�б�����
		{		 
			tp_dev.sta=TP_PRES_DOWN|TP_CATH_PRES;//��������  
			tp_dev.x[4]=tp_dev.x[0];//��¼��һ�ΰ���ʱ������
			tp_dev.y[4]=tp_dev.y[0];  	   			 
		}			   
	}else
	{
		if(tp_dev.sta&TP_PRES_DOWN)//֮ǰ�Ǳ����µ�
		{
			tp_dev.sta&=~(1<<7);//��ǰ����ɿ�//����֣�û�������6λ	
		}else//֮ǰ��û�б�����
		{
			tp_dev.x[4]=0;
			tp_dev.y[4]=0;
			tp_dev.x[0]=0xffff;
			tp_dev.y[0]=0xffff;
		}	    
	}
	return tp_dev.sta&TP_PRES_DOWN;//���ص�ǰ�Ĵ���״̬
}	  
//////////////////////////////////////////////////////////////////////////	 
//������EEPROM����ĵ�ַ�����ַ,ռ��14���ֽ�(RANGE:SAVE_ADDR_BASE~SAVE_ADDR_BASE+13)
#define SAVE_ADDR_BASE 40
//����У׼����,xfac,yfac,xoff,yoff������֮���һ����ַ��ֵ����ʾ�Ѿ�У׼��										    
void TP_Save_Adjdata(void)
{
	AT24CXX_Write(SAVE_ADDR_BASE,(u8*)&tp_dev.xfac,14);	//ǿ�Ʊ���&tp_dev.xfac��ַ��ʼ��14���ֽ����ݣ������浽tp_dev.touchtype
 	AT24CXX_WriteOneByte(SAVE_ADDR_BASE+14,0X0A);		//�����д0X0A���У׼����
}
//�õ�������EEPROM�����У׼ֵ
//����ֵ��1���ɹ���ȡ����
//        0����ȡʧ�ܣ�Ҫ����У׼
u8 TP_Get_Adjdata(void)
{					  
	u8 temp;
	temp=AT24CXX_ReadOneByte(SAVE_ADDR_BASE+14);//��ȡ�����,���Ƿ�У׼���� 		 
	if(temp==0X0A)//�������Ѿ�У׼����			   
 	{ 
		AT24CXX_Read(SAVE_ADDR_BASE,(u8*)&tp_dev.xfac,14);//��ȡ֮ǰ�����У׼���� 
		//��Ϊ����������ҪУ׼�����������жϵ��Ǻ�������������Ȼ�����xy��ȡָ��
		if(tp_dev.touchtype)//X,Y��������Ļ�෴
		{
			CMD_RDX=0X90;
			CMD_RDY=0XD0;	 
		}else				   //X,Y��������Ļ��ͬ
		{
			CMD_RDX=0XD0;
			CMD_RDY=0X90;	 
		}		 
		return 1;	 
	}
	return 0;
}	 
//��ʾ�ַ���
u8* const TP_REMIND_MSG_TBL="Please use the stylus click the cross on the screen.The cross will always move until the screen adjustment is completed.";
 					  
//��ʾУ׼���(��������)
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
	LCD_ShowNum(40+24,160,x0,4,16);		//��ʾ��ֵ
	LCD_ShowNum(40+24+80,160,y0,4,16);	//��ʾ��ֵ
	LCD_ShowNum(40+24,180,x1,4,16);		//��ʾ��ֵ
	LCD_ShowNum(40+24+80,180,y1,4,16);	//��ʾ��ֵ
	LCD_ShowNum(40+24,200,x2,4,16);		//��ʾ��ֵ
	LCD_ShowNum(40+24+80,200,y2,4,16);	//��ʾ��ֵ
	LCD_ShowNum(40+24,220,x3,4,16);		//��ʾ��ֵ
	LCD_ShowNum(40+24+80,220,y3,4,16);	//��ʾ��ֵ
 	LCD_ShowNum(40+56,240,fac,3,16); 	//��ʾ��ֵ,����ֵ������95~105��Χ֮��.

}
		 
//������У׼����
//�õ��ĸ�У׼����
void TP_Adjust(void)
{								 
	u16 pos_temp[4][2];//���껺��ֵ���洢�ĸ����xy����ֵ
	u8  cnt=0;	//��¼�ĸ����Ѳ�����Ŀ
	u16 d1,d2;	//������xy����֮��
	u32 tem1,tem2; 
	double fac; 	
	u16 outtime=0;
 	cnt=0;				
	POINT_COLOR=BLUE;
	BACK_COLOR =WHITE;
	LCD_Clear(WHITE);//����   
	POINT_COLOR=RED;//��ɫ 
	LCD_Clear(WHITE);//���� 	   
	POINT_COLOR=BLACK;

	// LCD_ShowString(40,40,160,100,16,(u8*)TP_REMIND_MSG_TBL);//��ʾ��ʾ��Ϣ

	TP_Drow_Touch_Point(20,20,RED);//����1 
	tp_dev.sta=0;//���������ź� 
	tp_dev.xfac=0;//xfac��������Ƿ�У׼��,����У׼֮ǰ�������!�������	
	
	while(1)//�������10����û�а���,���Զ��˳�
	{
		tp_dev.scan(1);//ɨ����������
		//��������б����¹��������������־λ����ˢ�³�ʱʱ�䣬��¼����
		if((tp_dev.sta&0xc0)==TP_CATH_PRES)//����������һ��(��ʱ�����ɿ���.)
		{	
			outtime=0;		
			tp_dev.sta&=~(1<<6);//��ǰ����Ѿ����������.
						   			   
			pos_temp[cnt][0]=tp_dev.x[0];
			pos_temp[cnt][1]=tp_dev.y[0];
			cnt++;//�Ѿ���һ���㱻���Թ���	  
			switch(cnt)
			{			   
				case 1:						 
					TP_Drow_Touch_Point(20,20,WHITE);				//�����1 
					TP_Drow_Touch_Point(lcddev.width-20,20,RED);	//����2
					break;
				case 2:
 					TP_Drow_Touch_Point(lcddev.width-20,20,WHITE);	//�����2
					TP_Drow_Touch_Point(20,lcddev.height-20,RED);	//����3
					break;
				case 3:
 					TP_Drow_Touch_Point(20,lcddev.height-20,WHITE);			//�����3
 					TP_Drow_Touch_Point(lcddev.width-20,lcddev.height-20,RED);	//����4
					break;
				case 4:	 //ȫ���ĸ����Ѿ��õ�
	    		    //�Ա����
					tem1=abs(pos_temp[0][0]-pos_temp[1][0]);//x1-x2
					tem2=abs(pos_temp[0][1]-pos_temp[1][1]);//y1-y2
					tem1*=tem1;
					tem2*=tem2;
					d1=sqrt(tem1+tem2);//�õ�1,2�ľ���
					
					tem1=abs(pos_temp[2][0]-pos_temp[3][0]);//x3-x4
					tem2=abs(pos_temp[2][1]-pos_temp[3][1]);//y3-y4
					tem1*=tem1;
					tem2*=tem2;
					d2=sqrt(tem1+tem2);//�õ�3,4�ľ���
					fac=(float)d1/d2;
					//��ȡ�Ķ����������꣬������fac=1���������ж�
					if(fac<0.95||fac>1.05||d1==0||d2==0)//���ϸ�
					{
						cnt=0;
 				    	TP_Drow_Touch_Point(lcddev.width-20,lcddev.height-20,WHITE);	//�����4
   	 					TP_Drow_Touch_Point(20,20,RED);			//����1
											
 						//TP_Adj_Info_Show(pos_temp[0][0],pos_temp[0][1],pos_temp[1][0],pos_temp[1][1],pos_temp[2][0],pos_temp[2][1],pos_temp[3][0],pos_temp[3][1],fac*100);//��ʾ����   
						My_LCD_string(3,4,24,"Failed",Virtual);
 						continue;
					}
					tem1=abs(pos_temp[0][0]-pos_temp[2][0]);//x1-x3
					tem2=abs(pos_temp[0][1]-pos_temp[2][1]);//y1-y3
					tem1*=tem1;
					tem2*=tem2;
					d1=sqrt(tem1+tem2);//�õ�1,3�ľ���
					
					tem1=abs(pos_temp[1][0]-pos_temp[3][0]);//x2-x4
					tem2=abs(pos_temp[1][1]-pos_temp[3][1]);//y2-y4
					tem1*=tem1;
					tem2*=tem2;
					d2=sqrt(tem1+tem2);//�õ�2,4�ľ���
					fac=(float)d1/d2;
					if(fac<0.95||fac>1.05)//���ϸ�
					{
						cnt=0;
 				    	TP_Drow_Touch_Point(lcddev.width-20,lcddev.height-20,WHITE);	//�����4
   	 					TP_Drow_Touch_Point(20,20,RED);								//����1
 						//TP_Adj_Info_Show(pos_temp[0][0],pos_temp[0][1],pos_temp[1][0],pos_temp[1][1],pos_temp[2][0],pos_temp[2][1],pos_temp[3][0],pos_temp[3][1],fac*100);//��ʾ����   
						My_LCD_string(3,4,24,"Failed",Virtual);
						continue;
					}//��ȷ��
								   
					//�Խ������
					tem1=abs(pos_temp[1][0]-pos_temp[2][0]);//x1-x3
					tem2=abs(pos_temp[1][1]-pos_temp[2][1]);//y1-y3
					tem1*=tem1;
					tem2*=tem2;
					d1=sqrt(tem1+tem2);//�õ�1,4�ľ���
	
					tem1=abs(pos_temp[0][0]-pos_temp[3][0]);//x2-x4
					tem2=abs(pos_temp[0][1]-pos_temp[3][1]);//y2-y4
					tem1*=tem1;
					tem2*=tem2;
					d2=sqrt(tem1+tem2);//�õ�2,3�ľ���
					fac=(float)d1/d2;
					if(fac<0.95||fac>1.05)//���ϸ�
					{
						cnt=0;
 				    	TP_Drow_Touch_Point(lcddev.width-20,lcddev.height-20,WHITE);	//�����4
   	 					TP_Drow_Touch_Point(20,20,RED);								//����1
 						//TP_Adj_Info_Show(pos_temp[0][0],pos_temp[0][1],pos_temp[1][0],pos_temp[1][1],pos_temp[2][0],pos_temp[2][1],pos_temp[3][0],pos_temp[3][1],fac*100);//��ʾ����   
						My_LCD_string(3,4,24,"Failed",Virtual);
						continue;
					}//��ȷ��
					//������
					tp_dev.xfac=(float)(lcddev.width-40)/(pos_temp[1][0]-pos_temp[0][0]);//�õ�xfac		 
					tp_dev.xoff=(lcddev.width-tp_dev.xfac*(pos_temp[1][0]+pos_temp[0][0]))/2;//�õ�xoff
						  
					tp_dev.yfac=(float)(lcddev.height-40)/(pos_temp[2][1]-pos_temp[0][1]);//�õ�yfac
					tp_dev.yoff=(lcddev.height-tp_dev.yfac*(pos_temp[2][1]+pos_temp[0][1]))/2;//�õ�yoff  
					if(abs(tp_dev.xfac)>2||abs(tp_dev.yfac)>2)//������Ԥ����෴��.
					{
						cnt=0;
 				    	TP_Drow_Touch_Point(lcddev.width-20,lcddev.height-20,WHITE);	//�����4
   	 					TP_Drow_Touch_Point(20,20,RED);								//����1
						My_LCD_string(40,26,16,"TP Need readjust!",Fact);
						tp_dev.touchtype=!tp_dev.touchtype;//�޸Ĵ�������.
						if(tp_dev.touchtype)//X,Y��������Ļ�෴
						{
							CMD_RDX=0X90;
							CMD_RDY=0XD0;	 
						}else				   //X,Y��������Ļ��ͬ
						{
							CMD_RDX=0XD0;
							CMD_RDY=0X90;	 
						}			    
						continue;
					}		
					POINT_COLOR=BLUE;
					LCD_Clear(WHITE);//����
					My_LCD_string(35,110,16,"Touch Screen Adjust OK!",Fact);//У�����
					delay_ms(1000);
					TP_Save_Adjdata();  
 					LCD_Clear(WHITE);//����   
					return;//У�����				 
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
//��������ʼ��  		    
//����ֵ:0,û�н���У׼
//       1,���й�У׼
u8 TP_Init(void)
{	
	  GPIO_InitTypeDef  GPIO_InitStructure;

		//ע��,ʱ��ʹ��֮��,��GPIO�Ĳ�������Ч
		//��������֮ǰ,����ʹ��ʱ��.����ʵ���������������
   	 	
	 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB|RCC_APB2Periph_GPIOF, ENABLE);	 //ʹ��PB,PF�˿�ʱ��
		
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;				 // PB1�˿�����
	 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 //�������
	 	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	 	GPIO_Init(GPIOB, &GPIO_InitStructure);//B1�������
	 	GPIO_SetBits(GPIOB,GPIO_Pin_1);//����
		
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;				 // PB2�˿�����
	 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 		 //��������
	 	GPIO_Init(GPIOB, &GPIO_InitStructure);//B2��������
	 	GPIO_SetBits(GPIOB,GPIO_Pin_2);//����		
		
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11|GPIO_Pin_9;				 // F9��PF11�˿�����
	 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 //�������
	 	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	 	GPIO_Init(GPIOF, &GPIO_InitStructure);//PF9,PF11�������
	 	GPIO_SetBits(GPIOF, GPIO_Pin_11|GPIO_Pin_9);//����
		
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;				 // PF10�˿�����
	 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 		 //��������
	 	GPIO_Init(GPIOF, &GPIO_InitStructure);//PF10��������
	 	GPIO_SetBits(GPIOF,GPIO_Pin_10);//����		
 
		TP_Read_XY(&tp_dev.x[0],&tp_dev.y[0]);//��һ�ζ�ȡ��ʼ��	 
		AT24CXX_Init();			//��ʼ��24CXX
		if(TP_Get_Adjdata())return 0;//�Ѿ�У׼
		else			  		//δУ׼?
		{ 										    
			LCD_Clear(WHITE);	//����
			TP_Adjust();  		//��ĻУ׼  
		}			
		TP_Get_Adjdata();	
	return 1; 									 
}

//�����̺���
u8* kbd_menu[15]={"Enter","0","Del","1","2","3","4","5","6","7","8","9",};//������

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
	My_LCD_CN(170+3,0,"��ʣ",Fact,BLACK);
	My_LCD_CN(170+3,60,"λ",Fact,BLACK);
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
	//n����ʾ
	My_LCD_Num(170+3,48,8-n,24,1,Fact);
}