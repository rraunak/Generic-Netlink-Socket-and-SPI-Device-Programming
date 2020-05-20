#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netlink/msg.h>
#include <netlink/attr.h>
#include <time.h>
#include <pthread.h>

#include "socket.h"

#include "sock_header.h"

static char message[GENL_TEST_ATTR_MSG_MAX];

static unsigned int mcgroups;		/* Mask of groups */

struct nl_sock* nlsock = NULL;

char framej[sizeof(struct sock_tran)];
char framel[sizeof(struct sock_tran)];
char framer[sizeof(struct sock_tran)];
char framehcsr[sizeof(struct sockhcsr_tran)];
char framede[sizeof(struct sockde_tran)];

char *recvmsgkern;
char recvdist;
int distret=0;
int done=0;
//threads defined one for pattern and other for distance measurement
pthread_t human_pattern_thread, req_dist_thread;


static void add_group(char* group)
{
	unsigned int grp = strtoul(group, NULL, 10);

	if (grp > GENL_TEST_MCGRP_MAX-1) {
		fprintf(stderr, "Invalid group number %u. Values allowed 0:%u\n",
			grp, GENL_TEST_MCGRP_MAX-1);
		exit(EXIT_FAILURE);
	}

	mcgroups |= 1 << (grp);
}
//callback function for sending pin configuration
static int send_msg_hcsr_to_kernel(struct nl_sock *sock)
{
	struct nl_msg* msg;
	int family_id, err = 0;

	family_id = genl_ctrl_resolve(sock, GENL_TEST_FAMILY_NAME);
	

	msg = nlmsg_alloc();
	if (!msg) {
		fprintf(stderr, "failed to allocate netlink message\n");
		exit(EXIT_FAILURE);
	}

	if(!genlmsg_put(msg, NL_AUTO_PID, NL_AUTO_SEQ, family_id, 0, 
		NLM_F_REQUEST, GENL_TEST_C_HCSR_MSG, 0)) {
		fprintf(stderr, "failed to put nl hdr!\n");
		err = -ENOMEM;
		goto out;
	}

	err = nla_put_string(msg, GENL_TEST_ATTR_MSG, message);
	if (err) {
		fprintf(stderr, "failed to put nl string!\n");
		goto out;
	}

	err = nl_send_auto(sock, msg);
	if (err < 0) {
		fprintf(stderr, "failed to send nl message!\n");
	}

out:
	nlmsg_free(msg);
	return err;
}
//callback for distance enable request
static int send_msg_de_to_kernel(struct nl_sock *sock)
{
	struct nl_msg* msg;
	int family_id, err = 0;

	family_id = genl_ctrl_resolve(sock, GENL_TEST_FAMILY_NAME);
	
	msg = nlmsg_alloc();
	if (!msg) {
		fprintf(stderr, "failed to allocate netlink message\n");
		exit(EXIT_FAILURE);
	}

	if(!genlmsg_put(msg, NL_AUTO_PID, NL_AUTO_SEQ, family_id, 0, 
		NLM_F_REQUEST, GENL_TEST_C_DE_MSG, 0)) {
		fprintf(stderr, "failed to put nl hdr!\n");
		err = -ENOMEM;
		goto out;
	}

	err = nla_put_string(msg, GENL_TEST_ATTR_MSG, message);
	if (err) {
		fprintf(stderr, "failed to put nl string!\n");
		goto out;
	}

	err = nl_send_auto(sock, msg);
	if (err < 0) {
		fprintf(stderr, "failed to send nl message!\n");
	}

out:
	nlmsg_free(msg);
	return err;
}
//callback for pattern display
static int send_msg_to_kernel(struct nl_sock *sock)
{
	struct nl_msg* msg;
	int family_id, err = 0;

	family_id = genl_ctrl_resolve(sock, GENL_TEST_FAMILY_NAME);
	

	msg = nlmsg_alloc();
	if (!msg) {
		fprintf(stderr, "failed to allocate netlink message\n");
		exit(EXIT_FAILURE);
	}

	if(!genlmsg_put(msg, NL_AUTO_PID, NL_AUTO_SEQ, family_id, 0, 
		NLM_F_REQUEST, GENL_TEST_C_MSG, 0)) {
		fprintf(stderr, "failed to put nl hdr!\n");
		err = -ENOMEM;
		goto out;
	}

	err = nla_put_string(msg, GENL_TEST_ATTR_MSG, message);
	if (err) {
		fprintf(stderr, "failed to put nl string!\n");
		goto out;
	}

	err = nl_send_auto(sock, msg);
	if (err < 0) {
		fprintf(stderr, "failed to send nl message!\n");
	}

out:
	nlmsg_free(msg);
	return err;
}


static int skip_seq_check(struct nl_msg *msg, void *arg)
{
	return NL_OK;
}
//callback for printing recieve function
static int print_rx_msg(struct nl_msg *msg, void* arg)
{
	
	struct nlattr *attr[GENL_TEST_ATTR_MAX+1];

	genlmsg_parse(nlmsg_hdr(msg), 0, attr, 
			GENL_TEST_ATTR_MAX, genl_test_policy);

	if (!attr[GENL_TEST_ATTR_MSG]) {
		fprintf(stdout, "Kernel sent empty message!!\n");
		return NL_OK;
	}

	recvmsgkern = nla_get_string(attr[GENL_TEST_ATTR_MSG]);
	
	//printf("Kernel Says: %s\n",recvmsgkern);
	distret = atoi(recvmsgkern);
	
	return NL_OK;
}
//setup the socket communication
static void prepare_sock(struct nl_sock** nlsock)
{
	int family_id, grp_id;
	unsigned int bit = 0;
	
	*nlsock = nl_socket_alloc();
	if(!*nlsock) {
		fprintf(stderr, "Unable to alloc nl socket!\n");
		exit(EXIT_FAILURE);
	}

	/* disable seq checks on multicast sockets */
	nl_socket_disable_seq_check(*nlsock);
	nl_socket_disable_auto_ack(*nlsock);

	/* connect to genl */
	if (genl_connect(*nlsock)) {
		fprintf(stderr, "Unable to connect to genl!\n");
		goto exit_err;
	}

	/* resolve the generic nl family id*/
	family_id = genl_ctrl_resolve(*nlsock, GENL_TEST_FAMILY_NAME);
	if(family_id < 0){
		fprintf(stderr, "Unable to resolve family name!\n");
		goto exit_err;
	}

	if (!mcgroups)
		return;

	while (bit < sizeof(unsigned int)) {
		if (!(mcgroups & (1 << bit)))
			goto next;

		grp_id = genl_ctrl_resolve_grp(*nlsock, GENL_TEST_FAMILY_NAME,GENL_TEST_MCGRP0_NAME);

		if (grp_id < 0)	{
			fprintf(stderr, "Unable to resolve group name for %u!\n",(1 << bit));
            goto exit_err;
		}
		if (nl_socket_add_membership(*nlsock, grp_id)) {
			fprintf(stderr, "Unable to join group %u!\n", (1 << bit));
            goto exit_err;
		}
next:
		bit++;
	}

    return;

exit_err:
    nl_socket_free(*nlsock); // this call closes the socket as well
    exit(EXIT_FAILURE);
}

static int left_running_man(void)
{
	//int ret;
	//unsigned char led[16]={0x1c,0x1c,0x08,0x3e,0x4a,0x08,0x7e,0x83,0x1c,0x1c,0x08,0x1c,0x3c,0x08,0x3c,0x26};
	unsigned char led[16]={0x1c,0x1c,0x08,0x3e,0x4a,0x08,0x7e,0x83,0x1c,0x1c,0x08,0x3E,0x49,0x08,0x3E,0x22};
	socktU sockl = {led};

	//printf("Userlwm: dist_enable=%d\n",sockl.dist_enable);

	memcpy(framel, &sockl, sizeof(framel));
	//printf("User: jsize=%zu\n",sizeof(framel));
	//printf("User: address=%s\n", &framel);

	strncpy(message, framel, GENL_TEST_ATTR_MSG_MAX);
	message[GENL_TEST_ATTR_MSG_MAX - 1] = '\0';

	send_msg_to_kernel(nlsock);
	return 0;
}

static int right_running_man(void)
{
	//int ret;
	//unsigned char led[16]={0x1c,0x1c,0x08,0x3e,0x29,0x08,0x7e,0xc1,0x1c,0x1c,0x08,0x1c,0x1e,0x08,0x3c,0x64};
	unsigned char led[16]={0x1c,0x1c,0x08,0x3e,0x29,0x08,0x7e,0xc1,0x1c,0x1c,0x08,0x3E,0x49,0x08,0x3E,0x22};
	socktU sockr = {led};

	//printf("Userrwm: dist_enable=%d\n",sockr.dist_enable);

	memcpy(framer, &sockr, sizeof(framer));
	//printf("User: jsize=%zu\n",sizeof(framer));
	//printf("User: address=%s\n", &framer);

	strncpy(message, framej, GENL_TEST_ATTR_MSG_MAX);
	message[GENL_TEST_ATTR_MSG_MAX - 1] = '\0';

	send_msg_to_kernel(nlsock);
	return 0;
}

static int right_walking_man(void)
{
	//int ret;
	//unsigned char led[16]={0x1c,0x1c,0x08,0x3e,0x29,0x08,0x7e,0xc1,0x1c,0x1c,0x08,0x1c,0x1e,0x08,0x3c,0x64};
	unsigned char led[16]={0x1c,0x1c,0x08,0x1c,0x1e,0x08,0x3c,0x64,0x1c,0x1c,0x08,0x3E,0x49,0x08,0x3E,0x22};
	socktU sockr = {led};

	//printf("Userrwm: dist_enable=%d\n",sockr.dist_enable);

	memcpy(framer, &sockr, sizeof(framer));
	//printf("User: jsize=%zu\n",sizeof(framer));
	//printf("User: address=%s\n", &framer);

	strncpy(message, framer, GENL_TEST_ATTR_MSG_MAX);
	message[GENL_TEST_ATTR_MSG_MAX - 1] = '\0';

	send_msg_to_kernel(nlsock);
	return 0;
}

static int left_walking_man(void)
{
	//int ret;
	//unsigned char led[16]={0x1c,0x1c,0x08,0x3e,0x4a,0x08,0x7e,0x83,0x1c,0x1c,0x08,0x1c,0x3c,0x08,0x3c,0x26};
	unsigned char led[16]={0x1c,0x1c,0x08,0x1c,0x3c,0x08,0x3c,0x26,0x1c,0x1c,0x08,0x3E,0x49,0x08,0x3E,0x22};
	socktU sockl = {led};

	//printf("Userlwm: dist_enable=%d\n",sockl.dist_enable);

	memcpy(framel, &sockl, sizeof(framel));
	printf("User: jsize=%zu\n",sizeof(framel));
	//printf("User: address=%s\n", &framel);

	strncpy(message, framel, GENL_TEST_ATTR_MSG_MAX);
	message[GENL_TEST_ATTR_MSG_MAX - 1] = '\0';

	send_msg_to_kernel(nlsock);
	return 0;
}

static int jumping_man(void)
{
	//int ret;
	unsigned char led[16]={0x1c,0x1c,0x49,0x3E,0x08,0x49,0x3E,0x00,0x1c,0x1c,0x08,0x3E,0x49,0x08,0x3E,0x22};
	socktU sockj = {led};

	//printf("Userjm: dist_enable=%d\n",sockj.dist_enable);

	memcpy(framej, &sockj, sizeof(framej));
	//printf("User: jsize=%zu\n",sizeof(framej));
	//printf("User: address=%s\n", &framej);

	strncpy(message, framej, GENL_TEST_ATTR_MSG_MAX);
	message[GENL_TEST_ATTR_MSG_MAX - 1] = '\0';

	send_msg_to_kernel(nlsock);
	return 0;
}

static int configure_pin(int chip, int trig, int echo)
{
	//sockthcsrU sockthcsr = {12,10,5,0};
	//int ret;
	sockthcsrU sockthcsr = {chip, trig, echo};

	//printf("User: cs_pin=%d\n",sockthcsr.cs_pin);
	//printf("User: trig_pin=%d\n",sockthcsr.trig_pin);
	//printf("User: echo_pin=%d\n",sockthcsr.echo_pin);
	//printf("User: dist_enable=%d\n",sockthcsr.dist_enable);
	memcpy(framehcsr, &sockthcsr, sizeof(framehcsr));
	//printf("Userhcsr: size=%zu\n",sizeof(framehcsr));
	
	strncpy(message, framehcsr, GENL_TEST_ATTR_MSG_MAX);
	message[GENL_TEST_ATTR_MSG_MAX - 1] = '\0';

	send_msg_hcsr_to_kernel(nlsock);
	return 0;
}

static int distance_send(int dist)
{
	socktdeU socktde = {dist};
	//printf("User: dist_enable=%d\n",socktde.dist_enable);

	memcpy(framede, &socktde, sizeof(framede));
	//printf("Userde: size=%zu\n",sizeof(framede));

	strncpy(message, framede, GENL_TEST_ATTR_MSG_MAX);
	message[GENL_TEST_ATTR_MSG_MAX - 1] = '\0';

	send_msg_de_to_kernel(nlsock);
	return 0;
}

/*

void delay(int seconds)
{
	int millisec = 1000*seconds;

	clock_t starttime = clock();

	while (clock() < starttime + millisec);
}
*/
void* req_dist(void *t)
{
	while(1)
	{
		sleep(3);
		distance_send(recvdist);  //send the distance periodically
		
	}
	return NULL;
}

void* human_pattern(void *t)
{	
	int i,j,retj;
	int temp[1600000];
	temp[0] = distret;
	//temp[1] = distret;
	i=0;
	//send the patterns
	for(j=0;j<1;j++)
	{	
		retj=1;
		
		do {
			retj = jumping_man();
			printf("After: inside jumping man\n");
		}while(retj!=0);
	}
	
	while(1)
	{
		i++;
		//temp[i] = ret;
		temp[i] = distret;
		//take the distance in the userspace
		//printf("sequence 1\n");
		struct nl_cb *cb = NULL;	
		cb = nl_cb_alloc(NL_CB_DEFAULT);
		//printf("sequence 2\n");
		nl_cb_set(cb, NL_CB_SEQ_CHECK, NL_CB_CUSTOM, skip_seq_check, NULL);
		//printf("sequence 3\n");
		nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, print_rx_msg, NULL);
		//printf("sequence 4\n");
		//ret = 0;
		//do {
		nl_recvmsgs(nlsock, cb);
		//} while (!ret);
		//printf("sequence 5\n");
		nl_cb_put(cb);
		//printf("sequence 6\n");

		//printf("prev:%d curr:%d\n",temp[i-1],temp[i]);
		//sleep(1);
		if(((temp[i-1]-1) < temp[i]) && (temp[i] < (temp[i-1]+1)))
		{
			for(j=0;j<1;j++)
			{	
				retj=1;
				
				do {
					retj = jumping_man();
					printf("After: inside jumping man\n");
				}while(retj!=0);
			}
			
		}
		
		if(((temp[i-1]-50) < temp[i]) && (temp[i] < (temp[i-1]-1)))
		{
			for(j=0;j<1;j++)
			{	
				retj=1;
				
				do {
					retj = left_walking_man();
					printf("After: inside left walking man\n");
				}while(retj!=0);

			}
			
		}
		
		if(((temp[i-1] + 1) < temp[i]) && (temp[i] < (temp[i-1]+50)))
		{
			for(j=0;j<1;j++)
			{	
				retj=1;
				
				do {
					retj = right_walking_man();
					printf("After: inside right walking man\n");
				}while(retj!=0);
			
			}
			
		}

		if((temp[i-1]-50) > temp[i])
		{
			for(j=0;j<1;j++)
			{	
				retj=1;
				
				do {
					retj = left_running_man();
					printf("After: inside left running man\n");
				}while(retj!=0);

			}
			
		}
		
		if(temp[i] > (temp[i-1]+50))
		{
			for(j=0;j<1;j++)
			{	
				retj=1;
				
				do {
					retj = right_running_man();
					printf("After: inside right running man\n");
				}while(retj!=0);
			
			}
			
		}		
		sleep(3);
		
	}	
	return NULL;
}

int main()
{
	char a[1] = "0";
	int trig,echo,cs,de;
	int ret1, ret2;
	char t,e,c,d;
	printf("Welcome to distance measurement and pattern display\n");
	printf("Enter the chip select pin\n");
	scanf("%d",&cs);
	printf("Enter the echo pin\n");
	scanf("%d",&echo);
	printf("Enter the trigger pin\n");
	scanf("%d",&trig);
	printf("Setting the pins\n");	

	
	t = (char)(trig);
	e = (char)(echo);
	c = (char)(cs);
	
	
	add_group(a);

	
	//setup
	prepare_sock(&nlsock);

	//configure
	configure_pin(c,t,e);
	sleep(1);
	printf("Pins set\n");



	printf("Enter 1 to start the distance measurement and pattern display\n");
	scanf("%d",&de);
	d = (char)(de);	
	recvdist = d;
	//start threads
	ret1 = pthread_create(&req_dist_thread, NULL, &req_dist, NULL);
	if(ret1 != 0)
		printf("\ncan't create distance measurement thread");
	usleep(3000);

	ret2 = pthread_create(&human_pattern_thread, NULL, &human_pattern, NULL); 
	if(ret2 != 0)
		printf("\ncan't create pattern");

	//terminate threads
	pthread_join(req_dist_thread, NULL);
	pthread_join(human_pattern_thread, NULL);



	//close the socket	
	nl_socket_free(nlsock);
	
	return 0;
}
