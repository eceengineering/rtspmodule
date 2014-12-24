#ifndef RTSPMODULE_H_
#define RTSPMODULE_H_

#ifdef __cplusplus
extern "C" {
#endif

struct rtspmodule_arguments 
{
	int 	width;
	int 	height;
	int 	gfps;
	int 	gbitrate;
	int 	gmtu;
	char 	*vsrc;
	char 	*vencoder;
	char 	*rtpencoder;
};

/* These functions return ERROR value as an integer */
int rtspmodule_init 	(struct rtspmodule_arguments *arg);
int rtspmodule_start	(void);
int rtspmodule_close 	(void);
int rtspmodule_setdata	(char *data);

/* Data Interface Handling */
sem_t gdata_ready;
sem_t gdata_wait;

#ifdef __cplusplus
}
#endif

#endif /* RTSPMODULE_H_ */
