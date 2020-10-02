#include "kernel.h"
#include "kernel_id.h"
#include "ecrobot_interface.h"

/* Definitions */


/* Global Variables */
int sti;

/* Function Prototypes */
void run(U8 buf);

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

#define RUNTIME_CONNECTION

void ecrobot_device_initialize() {
    ecrobot_init_bt_slave("LEJOS-DONGHYUN");
    ecrobot_init_sonar_sensor(NXT_PORT_S2);
    ecrobot_init_sonar_sensor(NXT_PORT_S3);
}

void user_1ms_isr_type2(void) {
    Statustype ercd;
    ercd = SignalCounter(SysTimerCnt);
    if (ercd != E_OK) {
        ShutdownOS(ercd);
    }
}

void ecrobot_device_terminate() {
    ecrobot_term_sonar_sensor(NXT_PORT_S3);
    ecrobot_term_sonar_sensor(NXT_PORT_S2);
    ecrobot_term_bt_connection();
}

TASK(EventDispatcher) {
    static U8 bt_receive_buf[32];
    static U8 TouchSensorStatus_old = 0;
    U8 TouchSensorStatus;

    ecrobot_read_bt_packet(bt_receive_buf, 32);

    TerminateTask();
}

TASK(EventHandler) {
    static U8 bt_send_buf[32];

    while (1) {
        WaitEvent(TouchSensorOnEvent);
        ClearEvent(TouchSensorOnEvent);

        bt_send_buf[0] = 1;
        ecrobot_send_bt_packet(bt_send_buf, 32);

        WaitEvent(TouchSensorOffEvent);
        ClearEvent(TouchSensorOffEvent);

        bt_send_buf[0] = 0;
        ecrobot_send_bt_packet(bt_send_buf, 32);
    }

    TerminateTask();
}

TASK(Initialize) {
    float err = 0;
    int count_temp = 0;
    int t1 = 0;
    int t2 = 0;
    int temp = 100;
    int temp3 = 0;

    nxt_motor_set_speed(NXT_PORT_A, 35, 1);

    while (temp != nxt_motor_get_count(NXT_PORT_A)) {
        temp = nxt_motor_get_count(NXT_PORT_A);
        systick_wait_ms(50);
    }

    nxt_motor_set_speed(NXT_PORT_A, 0, 1);
    count_temp = nxt_motor_get_count(NXT_PORT_A);
    t1 = nxt_motor_get_count(NXT_PORT_A);
    nxt_motor_set_speed(NXT_PORT_A, -35, 1);
    temp = 100;
    while (temp != nxt_motor_get_count(NXT_PORT_A)) {
        temp = nxt_motor_get_count(NXT_PORT_A);
        systick_wait_ms(50);
    }

    nxt_motor_set_speed(NXT_PORT_A, 0, 1);
    t2 = nxt_motor_get_count(NXT_PORT_A);

    count_temp = (t1 - t2) / 2 + t2;
    nxt_motor_set_speed(NXT_PORT_A, 20, 1);
    temp3 = nxt_motor_get_count(NXT_PORT_A);
    while (temp3 != count_temp) {
        temp3 = nxt_motor_get_count(NXT_PORT_A);
    }

    nxt_motor_set_speed(NXT_PORT_A, 0, 1);
    nxt_motor_set_count(NXT_PORT_A, 0);

    int count1, count2;
    nxt_motor_set_speed(NXT_PORT_A, 0, 1);
    nxt_motor_set_speed(NXT_PORT_B, 0, 1);
    nxt_motor_set_speed(NXT_PORT_C, 0, 1);
    nxt_motor_set_count(NXT_PORT_A, 0);
    nxt_motor_set_count(NXT_PORT_B, 0);
    nxt_motor_set_count(NXT_PORT_C, 0);
    systick_wait_ms(1000);
    nxt_motor_set_speed(NXT_PORT_B, 100, 1);
    nxt_motor_set_speed(NXT_PORT_C, 100, 1);
    systick_wait_ms(500);

    count1 = nxt_motor_get_count(NXT_PORT_B) / 10;
    count2 = nxt_motor_get_count(NXT_PORT_C) / 10;

    count1 = (count1 + count2) / 2;
    sti = 30 + count1 * 2;

    nxt_motor_set_speed(NXT_PORT_B, -100, 1);
    nxt_motor_set_speed(NXT_PORT_C, -100, 1);
    systick_wait_ms(800);
    nxt_motor_set_speed(NXT_PORT_B, 0, 1);
    nxt_motor_set_speed(NXT_PORT_C, 0, 1);

    TerminateTask();
}

TASK(IdleTask) {
    static SINT bt_status = BT_NO_INIT;

    while (1) {
        ecrobot_init_bt_slave("LEJOS-DONGHYUN");

        if (ecrobot_get_bt_status() == BT_STREAM && bt_status != BT_STREAM) {
            display_clear(0);
            display_goto_xy(0, 0);
            display_string("[BT]");
            display_update();
        }
        bt_status = ecrobot_get_bt_status();
    }
}

void run(U8 buf) {
    int vel, left, right;
    vel = a;
    if (a == speed_h)
        vel = a * sti / 100;
    else if (a == speed_l)
        vel = a * (sti - ((speed_h - speed_l) / 2)) / 100;

    if (c == 100) {
        left = vel;
        right = a;
    }
    else if (c == -100) {
        left = a;
        right = vel;
    }
    
    else {
        right = a;
        left = a;
    }

    if (buf == 1) {
        nxt_motor_set_speed(NXT_PORT_B, -right, 1);
        nxt_motor_set_speed(NXT_PORT_C, -left, 1);
    }

    else if (buf == 2) {
        nxt_motor_set_speed(NXT_PORT_B, right, 1);
        nxt_motor_set_speed(NXT_PORT_C, left, 1);
    }
}

