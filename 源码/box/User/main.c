#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "stdio.h"
/*本项目要用到的freertos知识*/
#include "task.h"
#include "event_groups.h"
/*自己的头文件*/
#include "sys.h"
#include "MyDelay.h"
#include "MyUART3.h"
#include "AS608.h"
#include "SG90.h"
#include "rc522.h"
#include "lcd.h"

/*优先级最高是5，待会改下优先级*/
#define SG90_TASK_STACK_SIZE 128
#define SG90_TASK_PRI 3/*SG90作为响应任务，优先级高一点*/
TaskHandle_t SG90_TASK_Handler;
void SG90_TASK(void *);

#define AS608_TASK_STACK_SIZE 128
#define AS608_TASK_PRIORI 2
TaskHandle_t AS608_TASK_Handler;
void AS608_TASK(void *);

#define RC522_TASK_STACK_SIZE 128
#define RC522_TASK_PRIORI 2
TaskHandle_t RC522_TASK_Handler;
void RC522_TASK(void *);

#define ESP8266_TASK_STACK_SIZE 128
#define ESP8266_TASK_PRIORI 2
TaskHandle_t ESP8266_TASK_Handler;
void ESP8266_TASK(void *);


#define LCD_TASK_STACK_SIZE 128
#define LCD_TASK_PRIORI 2
TaskHandle_t LCD_TASK_Handler;
void LCD_TASK(void *);

/*开始任务就不定义什么宏了，任务创建完毕就会自杀*/
void Task_Start(void);


#define LEDA PAout(1)
#define LEDB PAout(0)

/*事件组相关*/
EventGroupHandle_t MyEventGroupHandle;
#define EVENT_RC522_MASK (1ul<<0)
#define EVENT_AS608_MASK (1ul<<1)
#define EVENT_ESP8266_MASK (1ul<<2)
#define EVENT_LCD_MASK (1ul<<3)
#define EVENT_ALL_MASK 0x0F 

#define AS608_PIN_IN PAin(1)

int main(void)
{
    /*系统初始设置*/
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
    delay_init(72);
	
	
		/*外设初始化*/
		/*跟ESP8266通信*/
    MY_UART1_Init(115200);
		/*AS608用了UART2,占用引脚PA2和PA3，还占用了感应信号输入引脚PA7*/
		AS608_GPIO_Init();/*PA7*/ 
    MY_UART2_Init(9600);/*跟AS608通信，AS的波特率被设置成了9600*/
    /*舵机的初始化,选择了TIM3的通道3*/
    SG90_GPIO_Init();/*PB1*/
    SG90_PWM_Init();
    SG90_Turn_Angle(SG90_Angle_0);
    /*用于调试*/
    MY_UART3_Init(115200);
		u3_printf("欢迎使用智能门禁\r\n");
    /*读卡模块初始化，占用引脚PA4,5,6,7 PB0*/
    RC522_Init();
    USART1_WIFI_START_Trans();/*ESP开始通信*/

    MyEventGroupHandle = xEventGroupCreate();
    configASSERT(MyEventGroupHandle);
		
    xTaskCreate(
            (TaskFunction_t)         Task_Start ,
            (const char *)           "Task_Start", 
            (configSTACK_DEPTH_TYPE) configMINIMAL_STACK_SIZE,
            (void *)                 NULL,
            (UBaseType_t)            1,
            (TaskHandle_t *)         NULL 
    );
    vTaskStartScheduler();    

}

/*创建五个任务*/
void Task_Start(void)
{
    taskENTER_CRITICAL();
    
    xTaskCreate(
                (TaskFunction_t)         SG90_TASK ,
                (const char *)           "SG90_TASK", 
                (configSTACK_DEPTH_TYPE) SG90_TASK_STACK_SIZE,
                (void *)                 NULL,
                (UBaseType_t)            SG90_TASK_PRI,
                (TaskHandle_t *)         &SG90_TASK_Handler  
    );
    xTaskCreate(
                (TaskFunction_t)          RC522_TASK,
                (const char *)           "RC522_TASK", 
                (configSTACK_DEPTH_TYPE) RC522_TASK_STACK_SIZE,
                (void *)                 NULL,
                (UBaseType_t)            RC522_TASK_PRIORI,
                (TaskHandle_t *)         &RC522_TASK_Handler  
    );
    
    xTaskCreate(
                (TaskFunction_t)         AS608_TASK,
                (const char *)           "AS608_TASK", 
                (configSTACK_DEPTH_TYPE) AS608_TASK_STACK_SIZE,
                (void *)                 NULL,
                (UBaseType_t)            AS608_TASK_PRIORI,
                (TaskHandle_t *)         &AS608_TASK_Handler  
    );
    
     xTaskCreate(
                 (TaskFunction_t)         ESP8266_TASK,
                 (const char *)           "ESP8266_TASK", 
                 (configSTACK_DEPTH_TYPE) ESP8266_TASK_STACK_SIZE,
                 (void *)                 NULL,
                 (UBaseType_t)            ESP8266_TASK_PRIORI,
                 (TaskHandle_t *)         &ESP8266_TASK_Handler  
     );
    
     xTaskCreate(
                 (TaskFunction_t)         LCD_TASK,
                 (const char *)           "LCD_TASK", 
                 (configSTACK_DEPTH_TYPE) LCD_TASK_STACK_SIZE,
                 (void *)                 NULL,
                 (UBaseType_t)            LCD_TASK_PRIORI,
                 (TaskHandle_t *)         &LCD_TASK_Handler  
     );
    vTaskDelete(NULL); 
    taskEXIT_CRITICAL();
}


/*任务具体细节*/
void SG90_TASK(void *param)
{
    for (; ;)
    {
        xEventGroupWaitBits(MyEventGroupHandle,EVENT_ALL_MASK,pdTRUE,pdFALSE,portMAX_DELAY);
        SG90_Turn_Angle(SG90_Angle_90);
        
    }
    
    
}

void AS608_TASK(void *param)
{  
    
    for (; ;)
    {
       if (AS608_PIN_IN==1)
       {
            AS608_PIN_IN=0;
            if (AS608_Search(1,100)==1)
            {
               
                xEventGroupSetBits(MyEventGroupHandle,EVENT_AS608_MASK);
            }
            vTaskDelay(pdMS_TO_TICKS(500));
       }
       
    }
    
    
}

void RC522_TASK(void *param)
{   
    
    for (; ;)
    {
        if (MyRC522_Orignize()==ID_card_1)
        {
            xEventGroupSetBits(MyEventGroupHandle,EVENT_RC522_MASK);
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    
}

void  ESP8266_TASK(void *param)
{   
    for (; ;)
    {
       
    }
    
}

// void LCD_TASK(void *param)
// {   
//     for (; ;)
//     {
        
//     }
    
// }
