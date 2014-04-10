#include <iostream>
#include <linux/sched.h>
#include <sys/ioctl.h>
#include <fcntl.h>

using namespace std;

#define SCULL_IOC_MAGIK 'k'

#define SCULL_IOCRESET _IO(SCULL_IOC_MAGIK, 0)
#define SCULL_IOCSQUANTUM _IOW(SCULL_IOC_MAGIK, 1, int)
#define SCULL_IOCSQSET _IOW(SCULL_IOC_MAGIK, 2, int)
#define SCULL_IOCTQUANTUM _IO(SCULL_IOC_MAGIK, 3)
#define SCULL_IOCTQSET _IO(SCULL_IOC_MAGIK, 4)
#define SCULL_IOCGQUANTUM _IOR(SCULL_IOC_MAGIK, 5, int)
#define SCULL_IOCGQSET _IOR(SCULL_IOC_MAGIK, 6, int)
#define SCULL_IOCQQUANTUM _IO(SCULL_IOC_MAGIK, 7)
#define SCULL_IOCQQSET _IO(SCULL_IOC_MAGIK, 8)
#define SCULL_IOCXQUANTUM _IOWR(SCULL_IOC_MAGIK, 9, int)
#define SCULL_IOCXQSET _IOWR(SCULL_IOC_MAGIK, 10, int)
#define SCULL_IOCHQUANTUM _IO(SCULL_IOC_MAGIK, 11)
#define SCULL_IOCHQSET _IO(SCULL_IOC_MAGIK, 12)

#define SCULL_IOC_MAXNR 14

int main(int argc, char** argv)
{
    int quantum = 2000;
    
    int fd;

    fd = open(argv[1], O_RDWR);

    if(fd == -1)
    {
        cout<<"open error!\n"<<endl;
        return -1;
    }

    int ret;

    ret = ioctl(fd, SCULL_IOCSQUANTUM, &quantum);
    ret = ioctl(fd, SCULL_IOCGQUANTUM, &quantum);
    cout<<"the new quantum is : "<<quantum<<endl;
    quantum = 0;
	
    quantum = ioctl(fd, SCULL_IOCQQUANTUM);
    cout<<"the new quantum is : "<<quantum<<endl;

    quantum = 3000;
    ret = ioctl(fd, SCULL_IOCTQUANTUM, quantum);
    quantum = 0;
    ret = ioctl(fd, SCULL_IOCGQUANTUM, &quantum);

    cout<<"the new quantum is : "<<quantum<<endl;

    quantum = 8000;
    ret = ioctl(fd, SCULL_IOCXQUANTUM, &quantum);
    cout<<"the new quantum is : "<<quantum<<endl;
	
    quantum = ioctl(fd, SCULL_IOCQQUANTUM);
    cout<<"the new quantum is : "<<quantum<<endl;

    quantum = 6000;
    quantum = ioctl(fd, SCULL_IOCHQUANTUM, quantum);
    cout<<"the new quantum is : "<<quantum<<endl;

    quantum = ioctl(fd, SCULL_IOCQQUANTUM);
    cout<<"the new quantum is : "<<quantum<<endl;

    close(fd);

    return 0;
}
