#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <liblightnvm.h>
#include <sys/time.h>

#include <CUnit/Basic.h>

static char nvm_dev_path[NVM_DEV_PATH_LEN] = "/dev/nvme0n1";

static int channel = 0;
static int lun = 1;
static int block = 8;

static struct nvm_dev *dev;
static const struct nvm_geo *geo;
static struct nvm_addr blk_addr;

int setup(void)   //打开设备
{
    dev = nvm_dev_open(nvm_dev_path);
    if (!dev) {
        perror("nvm_dev_open");
    }
    geo = nvm_dev_get_geo(dev);

    blk_addr.ppa = 0;
    blk_addr.g.ch = channel;
    blk_addr.g.lun = lun;
    blk_addr.g.blk = block;

    return 0;
}

int teardown(void)  //关闭设备
{
    nvm_dev_close(dev);

    return 0;
}

double get_time(void){
    struct timeval mytime;
    gettimeofday(&mytime,NULL);
    return (mytime.tv_sec*1.0+mytime.tv_usec/1000000.0);
}
int  test_order_r(){  //顺序写
    char *buf_w=NULL;
    char *buf_r=NULL;
   // const int naddrs = geo->nplanes * geo->nsectors;
    struct nvm_addr addrs[1];
    struct nvm_ret ret;               //返回状态
    ssize_t res;
    size_t buf_w_nbytes, buf_r_nbytes;
    int pmode = NVM_FLAG_PMODE_SNGL;
    int failed = 1;

    nvm_addr_pr(blk_addr);

    buf_w_nbytes = 256 * geo->page_nbytes;
    buf_w = nvm_buf_alloc(geo, buf_w_nbytes);	// Setup buffers
    if (!buf_w) {
        printf("error!cant alloc buf_w!\n");
    }

    nvm_buf_fill(buf_w, buf_w_nbytes);
    double start_time=0.0,end_time=0.0;
    start_time= get_time();
    res = nvm_addr_write(dev, addrs, 1, buf_w, NULL, pmode, &ret);
    end_time= get_time();
    printf("cost time :%.6lf\n",end_time-start_time);
    return 0;

}
int main() {
    setup();
    test_order_r();
    teardown();
    return 0;
}
