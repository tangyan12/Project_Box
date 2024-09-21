#ifndef __SG90_H__
#define __SG90_H__

typedef enum {
    SG90_Angle_F_90=25,
    SG90_Angle_F_45=50,
    SG90_Angle_0=75,
    SG90_Angle_45=100,
    SG90_Angle_90=125,
}SG90_PWM_Angle ;

void SG90_GPIO_Init(void);
void SG90_PWM_Init(void);
void SG90_Turn_Angle( SG90_PWM_Angle Angle);
#endif
