#define BAUD 9600
#define DEV_NAME "GSM board v1  "
#define DEFAULT_PHONE_1 "+79210522030"
#define DEFAULT_PHONE_2 "+79210522030"
#define COUNT_REQUEST_TRY 3


#include "board/board.h"
#include "refs-avr/bwl_uart.h"
#include "refs-avr/bwl_simplserial.h"
#include "refs-avr/sim900.h"
#include "refs-avr/bwl_strings.h"

#include "refs-avr/ds18b20_avr.h"

#include "modules/gsm.h"
#include <util/setbaud.h>
#include <avr/wdt.h>

void sserial_send_start(unsigned char portindex)
{
	if (portindex==UART_485){
		DDRC|=(1<<4);
		PORTC|=(1<<4);
	}
}

void sserial_send_end(unsigned char portindex)
{
	if (portindex==UART_485)
	{
	    var_delay_ms(1);
		DDRC|=(1<<4);
		PORTC&=(~(1<<4));		 	
	}
}

char gsm_state_power_on=0;
unsigned long gsm_state_counter=0;
void gsm_operations()
{
	gsm_poll();
	if ((gsm_state_counter++)>=20000)
	{
		gsm_state_counter=0;
		board_led_set(0);
		gsm_checkstate();
		board_led_set(gsm_working);
		if (gsm_working==0)
		{
			gsm_init();
		}else
		{
			if (gsm_state_power_on==0)
			{
				wdt_reset();
				gsm_send_sms(DEFAULT_PHONE_1,"Module powered on");_delay_ms(5000);
				wdt_reset();
				//gsm_send_sms(DEFAULT_PHONE_2,"Module powered on");
				gsm_state_power_on=1;
			}
		}
	}
}

unsigned long regular_operations_counter=0;
void regular_operatios()
{
	if ((regular_operations_counter++)>=20000)
	{
		regular_operations_counter=0;
	}
}
//义耱
void led_on()
{
	sserial_request.command=19;
	sserial_request.data[0]=1;
	sserial_request.data[1]=0;
	sserial_request.datalength=2;
	volatile char result=sserial_send_request_wait_response(UART_485, 300);
	if (result==0)
	{
		string_clear();
		string_add_string("Board not respond");
	}else
	{
		string_add_string("LED on");
		board_led_set(0);
	}
}
//义耱
void led_off()
{
	sserial_request.command=20;
	sserial_request.data[0]=0;
	sserial_request.data[1]=0;
	sserial_request.datalength=2;
	volatile char result=sserial_send_request_wait_response(UART_485, 300);
	if (result==0)
	{
		string_clear();
		string_add_string("Board not respond");
	}
	else
	{
		string_add_string("LED off");
		board_led_set(1);
	}
}

//义耱
void switch_led()
{
	static int current_state=0;
	static int index=0;
	index++;
	if (index>200)
	{
		if (board_button_get()==1)
		{
			if (current_state==0)
			{
				led_on();
				current_state=1;
			}
			else
			{
				led_off();
				current_state=0;
			}
		}
		index=0;
	}
}

void get_battery_voltage()
{
	for (int counter=1; counter<COUNT_REQUEST_TRY; counter++)
	{
		wdt_reset();
		sserial_request.command=4;
		sserial_request.datalength=0;
		volatile char result=sserial_send_request_wait_response(UART_485, 300);
		string_clear();
		if (result!=0)
		{
			string_clear();
			float val = sserial_response.data[0];
			val /= 10;
			string_add_string("Battery voltage: ");
			string_add_float(val,1);
			string_add_string("V");
			return;
		}
	}
	string_add_string("Board not respond");
}

//义耱
void recive_voltage()
{
	static int index=0;
	index++;
	if (index>200)
	{
		if (board_button_get()==1)
		{
			get_battery_voltage();
			gsm_send_sms(gsm_received_sms_phone,string_buffer);
		}
		index=0;
	}
}

//ENGINE_STOP=1,
//IGNITION_INIT=2,
//ENGINE_STARTING=3,
//ENGINE_RUN=4,
//ENGINE_STOPPING=5,
void engine_start(int count_minutes)
{
	for (int counter=1; counter<COUNT_REQUEST_TRY; counter++)
	{
		wdt_reset();
		sserial_request.command=10;
		sserial_request.data[0]=count_minutes;
		sserial_request.datalength=1;
		volatile char result=sserial_send_request_wait_response(UART_485, 300);
		string_clear();
		if (result != 0)
		{
			if (sserial_response.data[0]==4) string_add_string("Engine run");
			if (sserial_response.data[0]==3) string_add_string("Engine starting");
			return;
		}
	}
	string_add_string("Board not respond");
}

void engine_stop()
{
	//消蜞屐�� 铗镳噔栩� 礤耜铍� 疣�
	for (int counter=1; counter<COUNT_REQUEST_TRY; counter++)
	{
		wdt_reset();
		sserial_request.command=5;
		sserial_request.datalength=0;
		string_clear();
		volatile char result=sserial_send_request_wait_response(UART_485, 100);
		if (result!=0)
		{
			string_add_string("Engine stop");
			return;
		}
	}
	string_add_string("Board not respond");
}

void gsm_received_sms()
{
	if (strstr(gsm_received_sms_text,"00")>0)	{gsm_send_sms(gsm_received_sms_phone, "01 Ping, 02 Devname, 03 Temp, 04 Bat voltage, 05 Engine stop, 10,15,20,25,30 Engine run minutes ");	}
	
	//橡钼屦赅 疋�玷
	if (strstr(gsm_received_sms_text,"01")>0)	{gsm_send_sms(gsm_received_sms_phone, "Pong!");	}
	//褥� 篑蝠铋耱忄
	if (strstr(gsm_received_sms_text,"02")>0)
	{
		string_clear();
		string_add_string("Board Devname: ");
		for (byte i=0; i<32; i++)
		{
			if (sserial_devname[i]>0)		string_add_char(sserial_devname[i]);
		}
		string_add_char(0);
		gsm_send_sms(gsm_received_sms_phone, string_buffer);
	}
	//义祜屦囹箴� � 溧蝼桕� 磬 镫囹�
	if (strstr(gsm_received_sms_text,"03")>0)
	{
		float sensor_temperature_0=ds18b20_get_temperature_float();
		string_clear();
		string_add_string("Temperature: ");
		
		string_add_float(sensor_temperature_0,1);
		gsm_send_sms(gsm_received_sms_phone,string_buffer);
	}
	//袜镳�驽龛� 徉蜞疱� � 爨礤
	if (strstr(gsm_received_sms_text,"04")>0)
	{
		wdt_reset();
		string_clear();
		get_battery_voltage();
		gsm_send_sms(gsm_received_sms_phone,string_buffer);
	}
	//务蜞眍忤螯 溻桡囹咫�
	if (strstr(gsm_received_sms_text,"05")>0)
	{
		wdt_reset();
		string_clear();
		engine_stop();
		//gsm_send_sms(gsm_received_sms_phone,string_buffer);	
	}
	//义耱
	if (strstr(gsm_received_sms_text,"08")>0)
	{
		wdt_reset();
		string_clear();
		led_on();
		gsm_send_sms(gsm_received_sms_phone,string_buffer);
	}
	//义耱
	if (strstr(gsm_received_sms_text,"09")>0)
	{
		wdt_reset();
		string_clear();
		led_off();
		gsm_send_sms(gsm_received_sms_phone,string_buffer);
	}
	//青矬耱栩� 溻桡囹咫� 磬 10
	if (strstr(gsm_received_sms_text,"10")>0)
	{
		wdt_reset();
		engine_start(10);
	}
	if (strstr(gsm_received_sms_text,"15")>0)
	{
		wdt_reset();
		engine_start(15);
	}
	if (strstr(gsm_received_sms_text,"20")>0)
	{
		wdt_reset();
		engine_start(20);
	}
	if (strstr(gsm_received_sms_text,"25")>0)
	{
		wdt_reset();
		engine_start(25);
	}	
	if (strstr(gsm_received_sms_text,"30")>0)
	{
		wdt_reset();
		engine_start(30);
	}	
}

void sserial_process_request(unsigned char portindex)
{
	//杨钺龛� 钺 篑镥铎 玎矬耜� 溻桡囹咫�
	if (sserial_request.command==30)
	{
		sserial_response.result=128+sserial_request.command;
		string_clear();
		string_add_string("Engine run");
		gsm_send_sms(gsm_received_sms_phone,string_buffer);
		sserial_response.datalength=0;
		sserial_send_response();
	}
	//拟桡囹咫� 礤 玎矬耱桦��
	if (sserial_request.command==31)
	{
		sserial_response.result=128+sserial_request.command;
		string_clear();
		string_add_string("Error. Engine does not start.");
		gsm_send_sms(gsm_received_sms_phone,string_buffer);
		sserial_response.datalength=0;
		sserial_send_response();
	}
	//拟桡囹咫� 铖蜞眍怆屙
	if (sserial_request.command==32)
	{
		sserial_response.result=128+sserial_request.command;
		float sensor_temperature_0=ds18b20_get_temperature_float();		
		string_clear();
		string_add_string("Engine stop. ");
		string_add_string("Temperature: ");	
		string_add_float(sensor_temperature_0,1);
		gsm_send_sms(gsm_received_sms_phone,string_buffer);
		sserial_response.datalength=0;
		sserial_send_response();
	}	
	if (sserial_request.command==8)
	{
		sserial_response.result=128+sserial_request.command;
		if (sserial_request.data[0]<255)	board_led_set(sserial_request.data[0]);
		sserial_response.data[0]=1;
		sserial_response.datalength=1;
		sserial_send_response();
	}
	if (sserial_request.command==9)
	{
		sserial_response.result=128+sserial_request.command;
		if (sserial_request.data[0]<255)	board_led_set(sserial_request.data[0]);
		sserial_response.data[0]=1;
		sserial_response.datalength=1;
		sserial_send_response();
	}
}

void device_init()
{
	set_unused_pin();
	board_button_enable();
}

int main(void)
{
	wdt_enable(WDTO_8S);
	uart_init_withdivider(UART_485,UBRR_VALUE);
	uart_init_withdivider(UART_GSM,UBRR_VALUE);

	device_init();
	//board_led_set(1);
	while(1)
	{
		gsm_operations();
		
		regular_operatios();
		//switch_led();
		//recive_voltage();
		sserial_poll_uart(UART_485);
		wdt_reset();
		_delay_ms(1);
	}
	
}