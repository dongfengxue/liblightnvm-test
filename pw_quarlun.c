//
// Created by lhj on 17-6-1.
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
//static int lun = 1;
static int block = 7;

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
    // blk_addr.g.lun = lun;
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
    const int naddrs =  16*geo->nsectors;  //page大小为16K,8M
    struct nvm_addr addrs[naddrs];
    struct nvm_ret ret;               //返回状态
    ssize_t res;
    size_t buf_w_nbytes, buf_r_nbytes;
    int pmode = NVM_FLAG_PMODE_SNGL;
    int failed = 1;

    // nvm_addr_pr(blk_addr);
    printf("naddrs:%d\n",naddrs);
    buf_w_nbytes = 16*geo->page_nbytes;     //开辟8M数据区
    buf_w = nvm_buf_alloc(geo, buf_w_nbytes);    // Setup buffers


    if (!buf_w) {
        printf("error!cant alloc buf_w!\n");
    }
    nvm_buf_fill01(buf_w, buf_w_nbytes);

    /* Erase  lun 0*/
        addrs[0].ppa=0;

        addrs[0].g.lun=0;
        addrs[0].g.pl=0;
    /*lun 1*/
        addrs[1].ppa=0;

        addrs[1].g.lun=1;
        addrs[1].g.pl=0;

/*lun 2*/

        addrs[2].ppa=0;

        addrs[2].g.lun=2;
        addrs[2].g.pl=0;
 /*lun 3*/

        addrs[3].ppa=0;

        addrs[3].g.lun=3;
        addrs[3].g.pl=0;
    
   res = nvm_addr_erase(dev, addrs, 4, pmode, &ret);
    if (res < 0) {
        printf("erase error!");
    }

   

    /* Write */

    /*first lun*/
    for (int i = 0; i < naddrs/4; ++i) {    //第一个lun
        addrs[i].ppa = blk_addr.ppa;

        addrs[i].g.lun = 0;
        // addrs[i].g.blk=((i/geo->nsectors)/geo->npages) % geo->nblocks;
        addrs[i].g.pg = (i/geo->nsectors) % geo->npages;
        addrs[i].g.sec = i % geo->nsectors;
        addrs[i].g.pl = 0;
    }
    /*second lun*/
    for (int i = naddrs/4; i < naddrs/2; ++i) {    //第二个lun
        addrs[i].ppa = blk_addr.ppa;

        addrs[i].g.lun = 1;
        // addrs[i].g.blk=((i/geo->nsectors)/geo->npages) % geo->nblocks;
        addrs[i].g.pg = ((i-naddrs/4)/geo->nsectors) % geo->npages;       //pg在新的lun重新计数
        addrs[i].g.sec = i % geo->nsectors;
        addrs[i].g.pl = 0;

    }
    /*third lun*/
    for (int i = naddrs/2; i < (naddrs/4)*3; ++i) {    //第三个lun
        addrs[i].ppa = blk_addr.ppa;

        addrs[i].g.lun = 2;
        // addrs[i].g.blk=((i/geo->nsectors)/geo->npages) % geo->nblocks;
        addrs[i].g.pg = ((i-naddrs/2)/geo->nsectors) % geo->npages;   //pg在新的lun重新计数
        addrs[i].g.sec = i % geo->nsectors;
        addrs[i].g.pl = 0;

    }
    /*forth lun*/
    for (int i = (naddrs/4)*3; i < naddrs; ++i) {    //第四个lun
        addrs[i].ppa = blk_addr.ppa;

        addrs[i].g.lun = 3;
        //  addrs[i].g.blk=((i/geo->nsectors)/geo->npages) % geo->nblocks;
        addrs[i].g.pg = ((i-(naddrs/4)*3)/geo->nsectors) % geo->npages; //pg在新的lun重新计数
        addrs[i].g.sec = i % geo->nsectors;
        addrs[i].g.pl = 0;

    }
   /* for (int i = 0; i < naddrs; ++i) {
        nvm_addr_pr(addrs[i]);
    }*/
     double start_time = 0.0, end_time = 0.0;
    start_time = get_time();
    
    res = nvm_addr_write(dev, addrs, naddrs, buf_w, NULL, pmode, &ret);
    if (res < 0) {
        printf("Write failure");
        free(buf_w);
    }


    //运行时间
    //   double start_time=0.0,end_time=0.0;
    //   start_time= get_time();
    //   res = nvm_addr_write(dev, addrs, naddrs, buf_w, NULL, pmode, &ret);
    end_time= get_time();
    printf("cost time :%.6lf\n",end_time-start_time);

    buf_r_nbytes=geo->page_nbytes;
    buf_r=nvm_buf_alloc(geo,buf_r_nbytes);
    if (!buf_r) {
        printf("error!cant alloc buf_r!\n");
    }
    memset(buf_r, '0', buf_r_nbytes);
    /*read*/
    //for(int blk=0;)
    for(size_t luns = 0;luns< 4;luns++){
        for (size_t pg = 0; pg < 4; ++pg) {
            struct nvm_addr addr[4];
            for (size_t sec = 0; sec < geo->nsectors; ++sec) {
                addr[sec].ppa = blk_addr.ppa;

                addr[sec].g.lun=luns;
                addr[sec].g.blk=block;
                addr[sec].g.pl=0;
                addr[sec].g.pg = pg;
                addr[sec].g.sec = sec;
                // nvm_addr_pr(addr[sec]);
            }
            res = nvm_addr_read(dev, addr, 4, buf_r, NULL, pmode, &ret);

            //  res = nvm_addr_read(dev, addrs, 16, buf_r, NULL, pmode, &ret);
            if (res < 0) {
                printf("read error!");
                nvm_ret_pr(&ret);
            }
            //   nvm_addr_pr(addr);
            // printf("%lu KB\n", buf_r_nbytes / 1024);
            //printf("pg: %.8192s\n", buf_r);
            //printf("pg:%lu: %s\n", pg,buf_r);
        }
    }

    return 0;

}
int main() {
    setup();
    test_order_w();
    teardown();
    return 0;
}
