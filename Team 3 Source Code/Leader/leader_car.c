#include "kernel.h"
#include "kernel_id.h"
#include "ecrobot_interface.h"

/* OSEK declarations */
DeclareCounter(SysTimerCnt);
DeclareTask(EventDispatcher);
DeclareTask(EventHandler);
DeclareTask(Initialize);
DeclareTask(SpeedTask);
DeclareTask(BrakeTask);
DeclareTask(IdleTask);
DeclareTask(SonarSensing);
DeclareEvent(TouchSensorOnEvent);
DeclareEvent(TouchSensorOffEvent);
DeclareEvent(event1);
DeclareEvent(event2);
DeclareResource(R1);
DeclareResource(OtherResource);

/* below macro enables run-time Bluetooth connection */
#define RUNTIME_CONNECTION


  int Current_speed=55; 
  int LR_Way=0; //left 100 right -100 striaght 0
  int Slow_brake=0; // 현재 속도에 따라 결정되는 slow brake값 
  int Motor_Run=0; // 1일 경우 모터 작동
  int Pre_Order=0; // 천천히 정지 시 이후에 다시 출발하기 위한 값
  int Brake; //즉시 정지 천천히 정지를 저장
  int Steering; // 모터를 최대 출력으로 돌려보고 get count로 받아서 좌우 회전시
                // 안쪽 바퀴의 속도를 조절하기 위한 변수
  int Sonar_1 = 0;// 초음파 센서의 값이 연속인지 확인 후 그 값을 사용하는데 
                  // 이전 초음파 센서의 값을 저장
  int Sonar_2 = 0;
  int Sonar_Check_1=2; // 초음파 센서의 값이 몇번 연속으로 나와야 그 값을 사용할지
                       // 저장하는 변수 (예 1이면 한번 연속이면 사용 2면 두번 연속이면)
  int Sonar_check_2=2;
  int Actual_Sonar_Num_1 = 0;// 맞다고 생각되는 초음파 센서의 값을 저장하는 변수
  int Actual_Sonar_Num_2 = 0;
  int Active_Only_One_Sonar=0;
  void Motor_Run_Fun(U8 buf);


/*

front car


*/


/* nxtOSEK hooks */
void ecrobot_device_initialize()
{
  ecrobot_init_sonar_sensor(NXT_PORT_S2);
  ecrobot_init_sonar_sensor(NXT_PORT_S3);
  ecrobot_init_bt_slave("LEJOS-DONGHYUN");

}

/* LEJOS OSEK hook to be invoked from an ISR in category 2 */
void user_1ms_isr_type2(void)
{  
StatusType ercd;
  ercd = SignalCounter(SysTimerCnt); /* Increment OSEK Alarm Counter */  
if(ercd != E_OK) 
 {
    ShutdownOS(ercd);  
}
}


void ecrobot_device_terminate()
{
  ecrobot_term_bt_connection();
  ecrobot_term_sonar_sensor(NXT_PORT_S2);
  ecrobot_term_sonar_sensor(NXT_PORT_S3);
}




/* EventDispatcher executed every 5ms */
TASK(EventDispatcher)
{
  static U8 bt_receive_buf[32];
  static U8 TouchSensorStatus_old = 0;
  U8 TouchSensorStatus;


  /* read packet data from the master device */
  ecrobot_read_bt_packet(bt_receive_buf, 32);


      int temp=Pre_Order;
      if(bt_receive_buf[3]==1||bt_receive_buf[3]==2) temp = bt_receive_buf[3];
  if(temp!=Pre_Order && bt_receive_buf[4]!=3&&bt_receive_buf[4]!=4&&LR_Way!=100&&LR_Way!=-100) Motor_Run=1;


//조향장치
//조향장치의 경우 err = (50*LR_Way)/100 - nxt_motor_get_count(NXT_PORT_A);에서 보이는 50이 최대각도이다. 
// 조향 장치가 움직이지 않을 때는 get_count값이 0이고 조향 장치를 움직이려 하면 LR_Way에 +-100이 들어온다.
// 초기에는 err에 50이 되고 이 값은 set_speed로 들어가서 조향장치를 움직인다.
// 이후에 조향장치가 50도까지 이동하게 되면 get_count는 50이 되고 LR_Way에 +-100이 들어오면 err는 0이 된다. 
// 따라서 최대각도인 50도 이후에는 err는 0이 되어서 조향 장치가 이동하지 않게 된다. 
// 이후에 조향장치의 명령을 중지하면 LR_Way는 0이 되고 err는 get_count에서만 값을 받아온다. 이때 get_count에서
// 받아온 값 앞에는 -가 붙어있고 -의 의미는 현재까지 이동한 방향의 반대 방향을 의미한다. 따라서 조향장치는
// 이전에 이동한 방향의 반대로 이동하게 되고 원래 위치로 이동하게 된다. 
  
  float err;
  if (bt_receive_buf[4] == 3) {LR_Way=-100;}
  else if (bt_receive_buf[4] == 4) {LR_Way=100;}
  else if(bt_receive_buf[7] == 2  && Motor_Run==1){ ;}
  else LR_Way=0;

  err = (40*LR_Way)/100 - nxt_motor_get_count(NXT_PORT_A);


  if (err > 0)
  {
    nxt_motor_set_speed(NXT_PORT_A, (err*2 + 40),1);
  }
  else if (err < 0)
  {
    nxt_motor_set_speed(NXT_PORT_A, (err*2 - 40),1);
  }
  else
  {
    nxt_motor_set_speed(NXT_PORT_A, 0,1);
  }
//고속일 경우
//고속으로 주행 할 경우 이전 속도와 현재 속도가 변화했는지 확인하고 변화가 있으면 
//Motor_Run을 1로 설정하고 event1을 발생시켜서 속도를 변화시키고 모터를 돌게한다.

  if (bt_receive_buf[5] == 1){
  if(Current_speed!=90 ){
  Motor_Run=1;
  SetEvent(SpeedTask, event1);
  }
  }
  else if (bt_receive_buf[5] == 2){
  if(Current_speed!=70 ){
  Motor_Run=1;
  SetEvent(SpeedTask, event1);
  }
  }

//천천히 브레이크
//천천히 브레이크의 경우 현재 속도에서 점점더 큰 값을 빼서 점점더 낮은 속도가 되도록 한다. 
   if(Slow_brake>0)
  {
      int temp;
    if(Current_speed==90){
    temp = (181-Slow_brake)/2;}
    if(Current_speed==70){
    temp = (141-Slow_brake)/2;}

  if (bt_receive_buf[3] == 1)
  {
    nxt_motor_set_speed(NXT_PORT_B, -Current_speed+temp,1);
    nxt_motor_set_speed(NXT_PORT_C, -Current_speed+temp,1);
  }
  if (bt_receive_buf[3] == 2)
  {
    nxt_motor_set_speed(NXT_PORT_B, +Current_speed-temp,1);
    nxt_motor_set_speed(NXT_PORT_C, +Current_speed-temp,1);

  }

  Slow_brake--;
  Motor_Run=0;

  }
//브레이크의 경우 Brake에 어떤 브레이크인지 저장하고 브레이크 이벤트 호출
  else if ((bt_receive_buf[6] == 1 || bt_receive_buf[6] == 2) && bt_receive_buf[7] == 1) 
  {
    Brake=bt_receive_buf[6];
    SetEvent(BrakeTask, event2);
  }

  //직진
  //직진일 경우 Motor_Run를 통해서 확인하고 모터작동를 작동시킨다.
  //또한 천천히 브레이크에서 완전히 정지시키기 때문에 다시 모터를 작동시키려면 버튼의 변화를 감지해야 하는데 
  // 현재 버퍼 3번에 들어있는 값을 Pre_Order에 저장한다.

  else if(Motor_Run==1)
  { 
  Motor_Run_Fun(bt_receive_buf[3]);

  if(bt_receive_buf[3]==1||bt_receive_buf[3]==2) Pre_Order=bt_receive_buf[3];
  }





  TerminateTask();
}

/* EventHandler executed by OSEK Events */
TASK(EventHandler)
{
  static U8 bt_send_buf[32];

  while(1)
  {
    WaitEvent(TouchSensorOnEvent); /* Task is in waiting status until the Event comes */
    ClearEvent(TouchSensorOnEvent);
    /* send packet data to the master device */
    bt_send_buf[0] = 1;
    ecrobot_send_bt_packet(bt_send_buf, 32);

    WaitEvent(TouchSensorOffEvent); /* Task is in waiting status until the Event comes */
    ClearEvent(TouchSensorOffEvent);
    /* send packet data to the master device */
    bt_send_buf[0] = 0;
    ecrobot_send_bt_packet(bt_send_buf, 32);
  }

  TerminateTask();
}

TASK(Initialize)
{
  GetResource(R1);


  int Count_temp=0;
  int Right_Count=0;
  int Left_Count=0;
  int temp=100;
//모터를 한쪽으로 끝까지 돌린다.
  nxt_motor_set_speed(NXT_PORT_A, 35 ,1);
//더이상 회전하지 못하면 끝이라고 인식하고 while문을 끝낸다.
  while(temp!=nxt_motor_get_count(NXT_PORT_A))
  {
  temp=nxt_motor_get_count(NXT_PORT_A);
  systick_wait_ms(50);
  } 
//더이상 돌지 못하면 속도를 0으로하고 Right_Count에 get_count를 통해서 
//얼마나 돌았는지 저장한다.
  nxt_motor_set_speed(NXT_PORT_A, 0,1);
  Right_Count=nxt_motor_get_count(NXT_PORT_A);
//반대로 회전후 그 때의 각도를 Left_Count에 저장한다.
  nxt_motor_set_speed(NXT_PORT_A, -35 ,1);
  temp=100;
  while(temp!=nxt_motor_get_count(NXT_PORT_A))
  {
    temp=nxt_motor_get_count(NXT_PORT_A);
    systick_wait_ms(50);
  }
  nxt_motor_set_speed(NXT_PORT_A, 0,1);
  Left_Count=nxt_motor_get_count(NXT_PORT_A);
//Count_temp에 목표 각도를 저장한다.
  Count_temp=(Right_Count-Left_Count)/2+Left_Count;
  nxt_motor_set_speed(NXT_PORT_A, 35 ,1);
  temp=nxt_motor_get_count(NXT_PORT_A);
//현재각도가 목표 각도가 될때까지 이동시킨다.
  while(temp!=Count_temp)
  {
    temp=nxt_motor_get_count(NXT_PORT_A);
  }
//조향 측정이 완료된 A포트를 정지한다.
  nxt_motor_set_speed(NXT_PORT_A, 0 ,1);
  nxt_motor_set_count(NXT_PORT_A, 2);

//B와 C포트에 최대출력을 1초동안 준다.
  nxt_motor_set_speed(NXT_PORT_B, 0, 1);
  nxt_motor_set_speed(NXT_PORT_C, 0, 1);
  nxt_motor_set_count(NXT_PORT_B, 0);
  nxt_motor_set_count(NXT_PORT_C, 0);
  systick_wait_ms(1000);  
  nxt_motor_set_speed(NXT_PORT_B, 100, 1);
  nxt_motor_set_speed(NXT_PORT_C, 100, 1);
  systick_wait_ms(500);

  //그 때의 각도를 저장한다.
  Right_Count=nxt_motor_get_count(NXT_PORT_B)/10;
  Left_Count=nxt_motor_get_count(NXT_PORT_C)/10;


  //Steering의 경우 여러가지 배터리를 확인하면서 38+((Right_Count+Left_Count)/2)*2이라는
  //함수를 찾았고 Steering에 함수값을 넣는다.
  Steering=38+((Right_Count+Left_Count)/2)*2;

  //차를 원해 위치로 이동시키기 위해 반대로 출력을 준다.
  nxt_motor_set_speed(NXT_PORT_B, -100, 1);
  nxt_motor_set_speed(NXT_PORT_C, -100, 1);
  systick_wait_ms(1000);
  nxt_motor_set_speed(NXT_PORT_B, 0, 1);
  nxt_motor_set_speed(NXT_PORT_C, 0, 1);



  ReleaseResource(R1);
  TerminateTask();
}


TASK(SpeedTask)
{
  while(1)
  {

  WaitEvent(event1);
  ClearEvent(event1);
  GetResource(R1);
  if(Current_speed==90) {Current_speed=70;}
  else if(Current_speed==70) {Current_speed=90;}
  Motor_Run_Fun(Pre_Order); 
  ReleaseResource(R1);
   }
  TerminateTask();
}

TASK(BrakeTask)
{


  while(1)
  {
    WaitEvent(event2);
    ClearEvent(event2);
    GetResource(R1);
    if(Brake==1)
    {
    nxt_motor_set_speed(NXT_PORT_B, 0,1);
    nxt_motor_set_speed(NXT_PORT_C, 0,1);
    } 
    else if(Brake==2)
    {
    if(Current_speed==90 && Motor_Run==1) {Slow_brake=180;}
    else if(Current_speed==70 && Motor_Run==1) {Slow_brake=140;}
    Motor_Run=0;
    }
  ReleaseResource(R1);
  }
  TerminateTask();
}

TASK(SonarSensing)
{


//Active_Only_One_Sonar는 어떤 초음파 센서를 작동시킬지 선택한다.
if(Active_Only_One_Sonar==0)
{
    int temp=0;
    temp = ecrobot_get_sonar_sensor(NXT_PORT_S2);
//temp에 초음파 센서의 값을 넣고 아래 if를 통해 범위에 맞는지 확인한다.
if(temp<50 && temp >1 )
{
    //해당 초음파 센서의 값이 이전 초음파 센서의 값과 같고(연속적이고)
    // 초기 값인 0이 아니면 유효한 값으로 판단한다.
    if(Sonar_1==temp && temp!=0)
    {
    //Sonar_Check_1는 연속적으로 초음파 센서의 값이 같은 경우를 확인한다.
    --Sonar_Check_1;
    if(Sonar_Check_1==0)
      {
        Actual_Sonar_Num_1=temp;
        Sonar_Check_1=1;
      }
    }
    //만약 다를경우 지금 초음파 센서의 값을 이후의 비교를 위해 저장한다.
    else 
  {
    Sonar_1 = temp;
    Sonar_Check_1=2;
  }

}
//초음파 센서에 의미있는 값이 아니면 100으로 한다.
else 
{
Actual_Sonar_Num_1=100;
}
}
else if(Active_Only_One_Sonar==1)
{
    int temp1=0;
    temp1 = ecrobot_get_sonar_sensor(NXT_PORT_S3);
if( temp1<50 && temp1 >1 )
{
    if(Sonar_2==temp1 && temp1!=0)
  {
    --Sonar_check_2;
    if(Sonar_check_2==0)
      {
        Actual_Sonar_Num_2=temp1;
        Sonar_check_2=1;
      }
  }
    else 
  {
    Sonar_2 = temp1;
    Sonar_check_2=2;
  }
}
else 
{
Actual_Sonar_Num_2=100;
}
}
if(Active_Only_One_Sonar==0){Active_Only_One_Sonar=1;}
else if(Active_Only_One_Sonar==1){Active_Only_One_Sonar=0;}

   TerminateTask();
}

/* IdleTask */
TASK(IdleTask)
{
  static SINT bt_status = BT_NO_INIT;

  while(1)
  {
    ecrobot_init_bt_slave("LEJOS-DONGHYUN");

    if (ecrobot_get_bt_status() == BT_STREAM && bt_status != BT_STREAM)
    {
      display_clear(0);
      display_goto_xy(0, 0);
      display_string("[BT]");
      display_update();
    }
    bt_status = ecrobot_get_bt_status();
  }
}

/*
모터 작동
현재 속도를 확인하고 이전에 구한 Steering을 사용해서 조향장치를 사용시 좌우 바퀴의 회전수를 조절한다.
vel은 보정된 바퀴의 회전수를 의미하고 조향장치를 사용하지 않으면 현재 NXT를 일자로 구동하기 위한 10% 보정만을 사용한다.
이 수치는 NXT를 여러번 구동하면서 오른쪽 바퀴에 10%정도 추가로 속도를 주었을 때 
가장 일자로 구동할 수 있어서 이러한 수치를 사용했다.
조향 장치를 사용하면 vel을 안쪽 바퀴에 속도로 사용하고 회전시에는 속도가 느려지는 현상이 있어서 
회전시에는 좌우바퀴에 속도를 10%씩 동일하게 증가시킨다. 
*/
void Motor_Run_Fun(U8 buf)
  { 
  int vel,left,right;
  vel=Current_speed*Steering/100;


  if(LR_Way==100) {left=vel+Current_speed/10; right=Current_speed+Current_speed/10;}
  else if(LR_Way==-100) {left=Current_speed+Current_speed/10; right=vel+Current_speed/10;}
  else {right=Current_speed; left=Current_speed+Current_speed/10;}

  if (buf == 1)
  {
    nxt_motor_set_speed(NXT_PORT_B, -right,1);
    nxt_motor_set_speed(NXT_PORT_C, -left,1);
  }
  else if (buf == 2)
  {
    nxt_motor_set_speed(NXT_PORT_B, right,1);
    nxt_motor_set_speed(NXT_PORT_C, left,1);

  }

  }










