#ifndef CAMMODULE_H_
#define CAMMODULE_H_

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_WIDTH	1280
#define MAX_HEIGHT 	960 

struct cammodule_arguments 
{
	int 	width;
	int 	height;
	char 	*device_name;
};

/* These functions return ERROR value as an integer */
int cammodule_init 	(struct cammodule_arguments *arg);
int cammodule_start	(void);
int cammodule_stop 	(void);
int cammodule_getframe	(char *data);

#ifdef __cplusplus
}
#endif

#endif /* CAMMODULE_H_ */
