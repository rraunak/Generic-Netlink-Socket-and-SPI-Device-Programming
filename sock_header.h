


typedef struct sock_tran
{
	unsigned char *led;
	//int cs_pin;
	//int trig_pin;
	//int echo_pin;
	//char dist_enable;
}socktU,socktK;

typedef struct sockde_tran
{
	//unsigned char *led;
	//int cs_pin;
	//int trig_pin;
	//int echo_pin;
	char dist_enable;
}socktdeU,socktdeK;

typedef struct sockhcsr_tran
{
	//unsigned char *led;
	char cs_pin;
	char trig_pin;
	char echo_pin;
	//char dist_enable;
}sockthcsrU,sockthcsrK;

