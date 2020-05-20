#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <linux/kthread.h>
#include <linux/miscdevice.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>
#include <linux/spi/spidev.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/skbuff.h>
#include <linux/netlink.h>
#include <linux/timer.h>
#include <linux/export.h>
#include <net/genetlink.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/string.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/semaphore.h>
#include<linux/init.h>
#include<linux/moduleparam.h>
#include <linux/miscdevice.h>
#include <linux/kernel.h>    						
#include <linux/gpio.h>      						
#include <linux/interrupt.h> 						
#include <linux/time.h>
#include <linux/unistd.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/jiffies.h>
#include <asm/div64.h>
#include <asm/delay.h>
#include <linux/stat.h>
#include <linux/math64.h>
#include <linux/unistd.h>
#include <linux/sched.h>
#include <linux/list.h>
#include <linux/ktime.h>
#include <linux/hrtimer.h>


#include "sock_header.h"
#include "socket.h"


#define DEVICE_NAME	"spi"

#define MAJOR_NUMBER	152

#define NETLINK_USER 31



static struct timer_list timer;
static struct genl_family genl_test_family;



int r=0,k=0,n=0;
int cs_pin,trig_pin,echo_pin,de;

int counter=0;
int send_to_user=0;
int echo,trigger;
int retp;
//struct for spi
struct spi_dev {
	struct spi_device *spi;
	int cs;
	int cs_pin;
	};
//struct for pattern reception
struct pattern {
	
	struct task_struct *thread;
	unsigned char led[16];
	
	};

struct pattern *pat;
static struct spi_dev *spi_led;
static struct spi_message m;
static struct spi_device *spimax_device;
static struct class *spimax_class;



static unsigned char xfer_tx[2]={0};

static unsigned int disp=0;
static unsigned int busy=0;



unsigned int distret=0;

int exitcount=0;

int distinitcount=0;


int speed = 340;
long long unsigned distance = 0;
long period;
unsigned int irql;
unsigned long f;

uint64_t rising, falling;

//structure for echo, trigger, number of samples and sample period defined
struct hcsr04_config
{
	int echo;
	int echopin;
	int trigger;
	int triggerpin;
	int sample;
	int sample_period;
} hcsr04_configs;


//Structure for the hcsr defined
struct hcsr04_dev{

	struct hcsr04_config hcsr04_configs;
	int c;
	int head;
	int counter;
	int m;
	struct task_struct *thread;	
	
};

static struct hcsr04_dev *hcsr04_devp;
static struct sock_tran *c;
static struct sockhcsr_tran *chcsr;
static struct sockde_tran *cde;


//Time stamp counter defined
static __inline__ unsigned long long RDTSC(void)
{
  unsigned hi, lo;
  __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
  return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}





//Configuration setting for the pins
void config_set( int gpioA, int gpioB, int direction, int f, int pinmuxA, int first, int pinmuxB, int last)
{
	if(!f)
	{
		if(pinmuxA != -1)
		{
			gpio_request(pinmuxA, "muxSelect1");
			gpio_set_value_cansleep(pinmuxA, first);
		}
		if(gpioB != -1)
		{
			gpio_request(gpioB, "shiftReg");
			if(direction)
				gpio_direction_input(gpioB);
			else
				gpio_direction_output(gpioB, direction);
		}
		if(pinmuxB != -1)
		{
			gpio_request(pinmuxB, "muxSelect2");
			gpio_set_value_cansleep(pinmuxB, last);
		}

		gpio_request(gpioA, "IOpin");
		if(direction)
		{
			gpio_direction_input(gpioA);
		}
		else
		{
			gpio_set_value_cansleep(gpioA, 0);
			gpio_direction_output(gpioA, direction);
		}
	}
	else
	{
		if(pinmuxA != -1)
			gpio_free(pinmuxA);
		if(gpioB != -1)
			gpio_free(gpioB);
		if(pinmuxB != -1)
			gpio_free(pinmuxB);

		gpio_free(gpioA);
	}
} 

//pin multiplexing
int config_pin(int io, int gpio_pin, int f)
{
	int pin;
	switch(gpio_pin)
	{
		case 0:
			config_set(11, 32, io, f, -1, -1, -1, -1);
			pin = 11;
			break;
		case 1:
			config_set(12, 28, io, f, 45, 0, -1, -1);
			pin = 12;
			break;
		case 2:
			config_set(13, 34, io, f, 77, 0, -1, -1);
			pin = 13;
			break;
		case 3:
			config_set(14, 16, io, f, 76, 0, 64, 0);
			pin = 14;
			break;
		case 4:
			config_set(6, 36, io, f, -1, -1, -1, -1);
			pin = 6;
			break;
		case 5:
			config_set(0, 18, io, f, 66, 0 , -1, -1);
			pin = 0;
			break;
		case 6:
			config_set(1, 20, io, f, 68, 0, -1, -1);
			pin = 1;
			break;
		case 7:
			config_set(38, -1, io, f, -1, -1, -1, -1);
			pin = 38;
			break;
		case 8:
			config_set(40, -1, io, f, -1, -1, -1, -1);
			pin = 40;
			break;
		case 9:
			config_set(4, 22, io, f, 70, 0, -1, -1);
			pin = 4;
			break;
		case 10:
			config_set(10, 26, io, f, 74, 0, -1, -1);
			pin = 10;
			break;
		case 11:
			config_set(5, 24, io, f, 44, 0, 72, 0);
			pin = 5;
			break;
		case 12:
			config_set(15, 42, io, f, -1, -1, -1, -1);
			pin = 15;
			break;
		case 13:
			config_set(7, 30, io, f, 46, 0, -1, -1);
			pin = 7;
			break;
		case 14:
			config_set(48, -1, io, f, -1, -1, -1, -1);
			pin = 48;
			break;
		case 15:
			config_set(50, -1, io, f, -1, -1, -1, -1);
			pin = 50;
			break;
		case 16:
			config_set(52, -1, io, f, -1, -1, -1, -1);
			pin = 52;
			break;
		case 17:
			config_set(54, -1, io, f, -1, -1, -1, -1);
			pin = 54;
			break;
		case 18:
			config_set(56, -1, io, f, 60, 1, 78, 1);
			pin = 56;
			break;
		case 19:
			config_set(58, -1, io, f, 60, 1, 79, 1);
			pin = 58;
			break;
		default:
			printk("HCSR04: Wrong pin number entered");
			pin = -1;
			break;
	}
	return pin;
} 

static irq_handler_t hcsr04_irq_handler(unsigned int irq, void *dev_id)
{
	long long unsigned dist;
	long long unsigned hcsr04_period;
	//Access the current device structure
	struct hcsr04_dev* hcsr04_devp = (struct hcsr04_dev *)dev_id;
	//get the value of the echo pin
	int value=gpio_get_value(hcsr04_devp->hcsr04_configs.echo);
	//Rising edge
	if(value)
	{
	
		rising = RDTSC();
		
	}
	//Falling edge
	else
	{
		falling = RDTSC();
		
		hcsr04_period = falling - rising;

		
		dist = hcsr04_period*speed;
		distance = div_u64(dist, 8000000);  //as board frequency is 400 MHz and we need to divide by 2 to measure the distance as echo travels back and forth
		
	}
	
	return (irq_handler_t) IRQ_HANDLED;
}

//Calculate the distance
int distance_calc(struct hcsr04_dev *hcsr04_devp)
{
	unsigned long long sum=0;
	unsigned long long first=0;
	unsigned long long last=80000000;
	int i;
	hcsr04_devp->m=1;
	//printk("HCSR04: Outside FOR");
	for (i=0; i<hcsr04_devp->hcsr04_configs.sample + 2; i++)
	{
		//printk("HCSR04: Inside FOR");
		//trigger the pin
		gpio_set_value_cansleep(hcsr04_devp->hcsr04_configs.trigger, 0);
		udelay(2);
		gpio_set_value_cansleep(hcsr04_devp->hcsr04_configs.trigger, 1);
		//Setting 1 to greater than 10 to triggger
		udelay(15);
		gpio_set_value_cansleep(hcsr04_devp->hcsr04_configs.trigger, 0);
		mdelay(1);
		sum = sum + distance; 
		//Find the first and the last values
		if(distance > first)
			first = distance;
		if(distance < last)
			last = distance;

		msleep(hcsr04_devp->hcsr04_configs.sample_period);

	}

	sum = sum - first -last;  //exclude the first and the last values
	do_div(sum, hcsr04_devp->hcsr04_configs.sample);  //Find the average distance
	
	hcsr04_devp->m=0;
	return sum;
	//printk("HCSR04: RDTSC:%llu\n",RDTSC());
	//printk("HCSR04: Distance Found\n");
}

void set_pins(struct hcsr04_dev *hcsr04_devp, int trig, int echo)
{
	hcsr04_devp->hcsr04_configs.echo = config_pin(1, echo, 0);
	hcsr04_devp->hcsr04_configs.echopin = echo;
	hcsr04_devp->hcsr04_configs.trigger = config_pin(0, trig, 0);
	hcsr04_devp->hcsr04_configs.triggerpin = trig;
	printk("Pins set in structure echo=%d,echo_pin=%d,trigger=%d,trig_pin=%d\n",echo,echo_pin,trigger,trig_pin);
}

void set_spi_pin(struct spi_dev *spi_led, int cselect)
{
	config_pin(-1, cselect, 0);
	spi_led->cs = config_pin(0, cselect, 0);
	spi_led->cs_pin = cselect;
	printk("chip select pins set in the structure, cspin=%d, cs=%d\n", spi_led->cs_pin,spi_led->cs);
}
//measurement thread
int measure_thread(void *t)
{
	struct hcsr04_dev *hcsr04_devp;
	hcsr04_devp=t;
	
		distret = distance_calc(hcsr04_devp);
	
	return 0;
}


//initialise distance
void distance_init(struct hcsr04_dev *hcsr04_devp)
{
	int output;

	hcsr04_devp->hcsr04_configs.sample = 7;
	hcsr04_devp->hcsr04_configs.sample_period = 65;		

	if((irql = gpio_to_irq(hcsr04_devp->hcsr04_configs.echo)) < 0)  //get the irq from the echo pin
	{
		printk(KERN_INFO "HCSR04:IRQ Failed");
	}
	output = request_irq(irql, (irq_handler_t) hcsr04_irq_handler, IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING , "irq", (void *)hcsr04_devp);  //request irq
	if(output < 0)
	{
		printk(KERN_INFO "HCSR04:IRQ Request Failed\n");
		
		
	}

	
}


//start distance thread
void distance_start(struct hcsr04_dev *hcsr04_devp)
{
	hcsr04_devp->thread = kthread_run(measure_thread, (void *)hcsr04_devp, "measurethread");

}

//send the distance to user
static void distance_send(unsigned int group)
{   
	void *hdr;
	int res, flags = GFP_ATOMIC;
	char msg[GENL_TEST_ATTR_MSG_MAX];
	struct sk_buff* skb = genlmsg_new(NLMSG_DEFAULT_SIZE, flags);
	

	if (!skb) {
	printk(KERN_ERR "%d: OOM!!", __LINE__);
	return;
	}

	hdr = genlmsg_put(skb, 0, 0, &genl_test_family, flags, GENL_TEST_C_MSG);
	if (!hdr) {
	printk(KERN_ERR "%d: Unknown err !", __LINE__);
	
	}
	//printk("Distance returned:%d\n",distret);
	snprintf(msg, GENL_TEST_ATTR_MSG_MAX, "%d", distret);
	

	res = nla_put_string(skb, GENL_TEST_ATTR_MSG, msg);
	if (res) {
	printk(KERN_ERR "%d: err %d ", __LINE__, res);
	
	}
	//printk("Before skb,hdr\n");
	genlmsg_end(skb, hdr);
	//printk("After skb,hdr\n");
	genlmsg_multicast(&genl_test_family, skb, 0, group , flags);
	//printk("After multicast\n");
	return;
	

}

static struct spi_transfer t = {

		.tx_buf = &xfer_tx[0],
		.rx_buf = 0,
		.len = 2,
		.cs_change = 1,
		.bits_per_word = 8,
		.speed_hz = 500000,
		}; 
//transfer values to spi
static void spimax_transfer(unsigned char row, unsigned char column)
{
	int ret=0;
	xfer_tx[0] = row;
	
	xfer_tx[1] = column;	

	gpio_set_value_cansleep(spi_led->cs, 0);	
	spi_message_init(&m);
	//printk("After init\n");
	spi_message_add_tail(&t, &m);
	//printk("After add tail\n");
	
	ret = spi_sync(spi_led->spi, &m);
	
	gpio_set_value_cansleep(spi_led->cs, 1);	
	
	//printk("After spi sync\n");
	return;
}


//make the pattern
int spimax_write(void *t)
{	
	

	unsigned char row=0,column=0;
	
	unsigned char s = 0x01;
	
	msleep(1000);
	for(r=1;r<17;r++)
	{
		unsigned char c = 0x00;
		if(r==9)
		{
			s = 0x01;
			msleep(1000);
		}
		
		row = s;
		k++;
		for(n=0;n<256;n++)
		{
			if(pat->led[r-1] == c)
			{
				
				column = c;
			

			k++;
			printk("Inside INNER FOR, value of c=%x\n",c);
			spimax_transfer(row, column);
			}
			
			c++;
			
		}
		
		s++;
		
		//printk("Inside For\n");
	}
	
	busy=0;
	
	return 0;
}
//recieve hcsr pin configuration
static int genl_test_rx_hcsr_msg(struct sk_buff* skb, struct genl_info* info)
{
	int pid,i;
	char frameshcsr[sizeof(struct sockhcsr_tran)];
	
	
	
	strcpy(frameshcsr, (char*)nla_data(info->attrs[GENL_TEST_ATTR_MSG]));
	
	pid = info->snd_portid;

	//printk("Kernel: hcsrsize=%zu\n",sizeof(frameshcsr));
	memcpy(chcsr, frameshcsr, sizeof(frameshcsr));


	//printk("C: cs_pin=%d\n",chcsr->cs_pin);
	//printk("C: trig_pin=%d\n",chcsr->trig_pin);
	//printk("C: echo_pin=%d\n",chcsr->echo_pin);
	

	echo_pin = (int)(chcsr->echo_pin);
	trig_pin = (int)(chcsr->trig_pin);
	cs_pin = (int)(chcsr->cs_pin);
	//printk("C: TYPEtrig_pin=%d\n",trig_pin);
	//printk("C: TYPEecho_pin=%d\n",echo_pin);
	//printk("C: TYPEcs_pin=%d\n",cs_pin);

	set_pins(hcsr04_devp, trig_pin , echo_pin );
	set_spi_pin(spi_led, cs_pin);

	printk("Pins set\n");	
	
	spimax_transfer(0x0F, 0x00);
	spimax_transfer(0x0C, 0x01);
	spimax_transfer(0x0B, 0x07);
	spimax_transfer(0x09, 0x00);
	spimax_transfer(0x0A, 0x0A);

	for(i=1; i<9; i++)
	{
		spimax_transfer(i, 0x00);
	}

	printk("led initialized\n");

	return 0;
	
}

//recieve the pattern
static int genl_test_rx_msg(struct sk_buff* skb, struct genl_info* info)
{
	int pid,i;
	char frames[sizeof(struct sock_tran)];
	
	
	if(busy == 1 || disp == 1)
	{
		printk("Failure: data cannot be copied, another process currently in use\n");		
		return -1;
	}
	
	
	if(disp == 0)
	{
	disp=1;
	strcpy(frames, (char*)nla_data(info->attrs[GENL_TEST_ATTR_MSG]));
	
	pid = info->snd_portid;
	
	//printk("led values inside callback before copy before sleep:%s", frames);
	//printk("Kernel: size=%zu\n",sizeof(frames));
	memcpy(c, frames, sizeof(frames));
	
	//printk("led values inside callback before copy:%s", c->led);
	
	
	for(i=0;i<16;i++)
	{
		pat->led[i] = c->led[i];
		msleep(6);
	}
	
	disp=0;
	}
	

	busy=1;
	pat->thread = kthread_run(&spimax_write, (void*)c->led, "pattern_thread");
	 
	
	
	return 0;
	
}
//recieve distance measurement request
static int genl_test_rx_de_msg(struct sk_buff* skb, struct genl_info* info)
{
	int pid;
	char framesde[sizeof(struct sockde_tran)];
	

	
	strcpy(framesde, (char*)nla_data(info->attrs[GENL_TEST_ATTR_MSG]));
	
	pid = info->snd_portid;
	
	//printk("Kernel: DE size=%zu\n",sizeof(framesde));
	memcpy(cde, framesde, sizeof(framesde));


	de = (int)(cde->dist_enable);
	
	if(de == 1 && distinitcount == 0)
	{
		
		distance_init(hcsr04_devp);

		//printk("Distance initiated\n");

		distinitcount++;
	}
	if(de == 1)
	{
	
		//printk("before distance_start\n");
		distance_start(hcsr04_devp);
		//printk("before distance_send\n");
		distance_send(GENL_TEST_MCGRP0);
		//printk("after distance_send\n");
		
	}	
	
	
	return 0;
}



static int spimax_probe(struct spi_device *spi)
{
	int ret = 0;
	struct device *dev;

	spi_led = kzalloc(sizeof(*spi_led), GFP_KERNEL);

	spi_led->spi = spi;

	dev = device_create(spimax_class, &spi->dev, MKDEV(MAJOR_NUMBER, 1), spi_led, DEVICE_NAME);
	return ret;
}



static int spimax_remove(struct spi_device *spi)
{
	int ret=0;
	config_pin(-1, spi_led->cs_pin , 1);
	spi_led->spi = NULL;


	device_destroy(spimax_class, MKDEV(MAJOR_NUMBER, 1));
	kfree(spi_led);
	return ret;
}

//spi driver defined
static struct spi_driver spimax_driver = {
	.driver = {
		.name = "spi",
		.owner= THIS_MODULE,
	},
	.probe	      = spimax_probe,
	.remove	      = spimax_remove,
};

struct spi_board_info spi_info = {
	.modalias = "spi",
	.max_speed_hz = 500000,
	.bus_num = 1,
	.chip_select = 1,
	.mode = 0,
};

//functions for three commands from user space defined
static const struct genl_ops genl_test_ops[] = {
    {
	.cmd = GENL_TEST_C_MSG,
	.policy = genl_test_policy,
	.doit = genl_test_rx_msg,
	.dumpit = NULL,
    },

    {
	.cmd = GENL_TEST_C_DE_MSG,
	.policy = genl_test_policy,
	.doit = genl_test_rx_de_msg,
	.dumpit = NULL,
    },

    {
	.cmd = GENL_TEST_C_HCSR_MSG,
	.policy = genl_test_policy,
	.doit = genl_test_rx_hcsr_msg,
	.dumpit = NULL,
    },
};

static const struct genl_multicast_group genl_test_mcgrps[] = {
	[GENL_TEST_MCGRP0] = { .name = GENL_TEST_MCGRP0_NAME, },
	
};

static struct genl_family genl_test_family = {
	.name = GENL_TEST_FAMILY_NAME,
	.version = 1,
	.maxattr = GENL_TEST_ATTR_MAX,
	.netnsok = false,
	.module = THIS_MODULE,
	.ops = genl_test_ops,
	.n_ops = ARRAY_SIZE(genl_test_ops),
	.mcgrps = genl_test_mcgrps,
	.n_mcgrps = ARRAY_SIZE(genl_test_mcgrps),
};

//initialise the functions
static int __init spimax_init(void)
{
	int rc;
	int ret=0;
	//int i;
	struct spi_master *master;

	master = spi_busnum_to_master(spi_info.bus_num);
	spimax_device = spi_new_device(master, &spi_info);
	spimax_device->bits_per_word = 8;
	ret = spi_setup(spimax_device);
	
	ret = spi_register_driver(&spimax_driver);

	pat = kmalloc(sizeof(*pat), GFP_KERNEL);

	hcsr04_devp = kmalloc(sizeof(*hcsr04_devp), GFP_KERNEL);

	c = (struct sock_tran *)kmalloc(sizeof(struct sock_tran), GFP_KERNEL);

	chcsr = (struct sockhcsr_tran *)kmalloc(sizeof(struct sockhcsr_tran), GFP_KERNEL);

	cde = (struct sockde_tran *)kmalloc(sizeof(struct sockde_tran), GFP_KERNEL);

	//initialize the device structure variables
	hcsr04_devp->head = 0;
	hcsr04_devp->c = 0;
	hcsr04_devp->counter = 0;
	hcsr04_devp->hcsr04_configs.triggerpin = 0;
	hcsr04_devp->hcsr04_configs.echopin = 0;


	gpio_free(44);
	gpio_free(72);
	gpio_free(24);
	gpio_free(46);
	gpio_free(30);

	gpio_request_one(44, GPIOF_DIR_OUT, "MOSI_MUX1");
	gpio_request_one(72, GPIOF_OUT_INIT_LOW, "MOSI_MUX2");
	gpio_request_one(24, GPIOF_DIR_OUT, "MOSI_SHIFT");
	gpio_request_one(46, GPIOF_DIR_OUT, "SPI_SCK");
	gpio_request_one(30, GPIOF_DIR_OUT, "SCK_SHIFT");
	

	gpio_set_value_cansleep(44, 1);
	gpio_set_value_cansleep(72, 0);
	gpio_set_value_cansleep(24, 0);
	gpio_set_value_cansleep(46, 1);
	gpio_set_value_cansleep(30, 0);
	
	printk("spi device initialized\n");

	
	//printk("Outside For\n");

	printk(KERN_INFO "genl_test: initializing netlink\n");

    	rc = genl_register_family(&genl_test_family);
    	

	return 0;

}
//free the pin configurations and exit
static void __exit spimax_exit(void)
{
	unsigned char i=0;
	for(i=1; i < 9; i++)
	{
		spimax_transfer(i, 0x00);
	}

	
	config_pin(-1, hcsr04_devp->hcsr04_configs.echopin, 1);
	config_pin(-1, hcsr04_devp->hcsr04_configs.triggerpin, 1);
	free_irq(gpio_to_irq(hcsr04_devp->hcsr04_configs.echo), (void *)hcsr04_devp);
	kfree(hcsr04_devp);
	
	gpio_free(44);
	gpio_free(72);
	gpio_free(46);
	gpio_free(24);
	gpio_free(30);	
	del_timer(&timer);
	genl_unregister_family(&genl_test_family);
	spi_unregister_driver(&spimax_driver);
	spi_unregister_device(spimax_device);
	kfree(pat);
	kfree(c);
	kfree(chcsr);
	kfree(cde);
	printk("spi device removed\n");
}


module_init(spimax_init);
module_exit(spimax_exit);

MODULE_LICENSE("GPL");

	
