//
// Created by lhj on 17-5-31.
//

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
static int lun = 0;
static int block = 2;

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
    nvm_geo_pr(geo);

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
void nvm_buf_fill01(char *buf, size_t nbytes)
{
    for (size_t i = 0; i < nbytes/2; ++i)
        buf[i] = '1';
    for (size_t i = nbytes/2; i < nbytes; ++i)
        buf[i] = 'a';
}
int  test_order_w() {  //顺序写
    char *buf_w = NULL;
    char *buf_r = NULL;
    const int naddrs =  geo->nsectors;  //sectors的整数倍，此处为4
    struct nvm_addr addrs[naddrs];
    struct nvm_ret ret;               //返回状态
    ssize_t res;
    size_t buf_w_nbytes, buf_r_nbytes;
    int pmode = NVM_FLAG_PMODE_SNGL;
    int failed = 1;

    //nvm_addr_pr(blk_addr);

    buf_w_nbytes = geo->page_nbytes;     //开辟8M数据区
    buf_w = nvm_buf_alloc(geo, buf_w_nbytes);    // Setup buffers


    if (!buf_w) {
        printf("error!cant alloc buf_w!\n");
    }
    nvm_buf_fill01(buf_w, buf_w_nbytes);

    /* Erase */
    addrs[0].ppa=blk_addr.ppa;
    addrs[0].g.pl=0;

    res = nvm_addr_erase(dev, addrs, 1, pmode, &ret);
    if (res < 0) {
        printf("erase error!");
    }

    double start_time = 0.0, end_time = 0.0;
    start_time = get_time();

    /* Write */
    //for (size_t blk = 0; blk < 8; ++blk)
    // for (size_t pg = 0; pg < 4; ++pg) {
    for (int i = 0; i < naddrs; ++i) {
        addrs[i].ppa = blk_addr.ppa;

        //addr[i].g.blk=blk;
        addrs[i].g.pg = 0;
        addrs[i].g.sec = i % geo->nsectors;
        addrs[i].g.pl = 0;

        //   }
        //}
    }
        res = nvm_addr_write(dev, addrs, naddrs, buf_w, NULL, pmode, &ret);     //page单位写一次
        if (res < 0) {
            printf("Write failure");
            free(buf_w);
        }
    for (int i = 0; i < naddrs; ++i) {
        nvm_addr_pr(addrs[i]);
    }

    /*read*/
    buf_r_nbytes=geo->sector_nbytes;
    buf_r=nvm_buf_alloc(geo,buf_r_nbytes);
    if (!buf_r) {
        printf("error!cant alloc buf_r!\n");
    }
    memset(buf_r, '0', buf_r_nbytes);
    struct nvm_addr addr;
        for (int i = 0; i < naddrs; ++i) {
            addr.ppa = blk_addr.ppa;

            addr.g.pg = 0;
            addr.g.sec = i % geo->nsectors;
            addr.g.pl = 0;

            res = nvm_addr_read(dev, &addr, 1, buf_r, NULL, pmode, &ret);
            if (res < 0) {
                printf("read error!");
                nvm_ret_pr(&ret);
            }

            printf("%lu KB\n", buf_r_nbytes / 1024);
            printf("sec %d: %.4096s\n",addr.g.sec, buf_r);
        }
    //printf("pg:%lu: %s\n", pg,buf_r);
    return 0;

}
int main() {
    setup();
    test_order_w();
    teardown();
    return 0;
}
