#ifndef SCULL_H
#define SCULL_H


static int scull_major = 0;
static int scull_minor = 1;
static unsigned int scull_num = 1;

static int scull_quantum = 1000;
static int scull_qset = 4;

struct scull_qset
{
	void **data;
	struct scull_qset *next;
};

struct scull_dev
{
	struct scull_qset *data;
	int quantum;
	int qset;
	unsigned long size;
	unsigned int access_key;
	struct semaphore sem;
	struct cdev cdev;
};

#endif