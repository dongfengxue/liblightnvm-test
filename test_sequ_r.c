//
// Created by lhj on 17-5-26.
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
void nvm_buf_fills0(char *buf,  int size)
{
    for (int i = 0; i < size; ++i)
        buf[i] = '0';
}
void nvm_buf_fills1(char *buf,  int size)
{
    for (int  i = 0; i < size; ++i)
        buf[i] = '1';
}
void nvm_buf_fills01(char *buf,  int size)
{
    for (int  i = 0; i < size/2; ++i)
        buf[i] = '1';
    for (int  i = size/2; i < size; ++i)
        buf[i] = '0';
}
int test_sequ_r(){
    char *buf_r=NULL,*buf_w=NULL;
    const int naddrs = geo->nplanes * geo->nsectors;
    struct nvm_addr addrs[naddrs];
    struct nvm_ret ret;               //返回状态
    ssize_t res;
    size_t  buf_r_nbytes,buf_w_nbytes;
    int pmode = NVM_FLAG_PMODE_SNGL;
    int failed = 1;
    nvm_addr_pr(blk_addr);

    //erase
    if (pmode) {
        addrs[0].ppa = blk_addr.ppa;
    } else {
        for (size_t pl = 0; pl < geo->nplanes; ++pl) {
            addrs[pl].ppa = blk_addr.ppa;

            addrs[pl].g.pl = pl;
        }
    }
    res = nvm_addr_erase(dev, addrs, pmode ? 1 : geo->nplanes, pmode, &ret);
    if (res < 0) {
        printf("erase error");
    }
    //write
    buf_w_nbytes = 2*geo->page_nbytes;        //开辟一个页大小的缓存
    buf_w= nvm_buf_alloc(geo, buf_w_nbytes);	// Setup buffers
    if (!buf_w) {
        printf("error!cant alloc buf_w!\n");
        free(buf_w);
    }
    nvm_buf_fills01(buf_w, buf_w_nbytes);
    //第一个页
   for(int pg=0;pg<2;pg++) {
       for (int i = 0; i < naddrs; i++)   //两个plane的第一个page
       {
           addrs[i].ppa = blk_addr.ppa;
           addrs[i].g.pg = pg;
           addrs[i].g.sec = i % geo->nsectors;
           addrs[i].g.pl = (i / geo->nsectors) % geo->nplanes;

       }
       res = nvm_addr_write(dev, addrs, naddrs, buf_w, NULL, pmode, &ret);
   }


    //read
    buf_r_nbytes = 2*geo->page_nbytes;
    buf_r = nvm_buf_alloc(geo, buf_r_nbytes);// Setup buffers
    if (!buf_r) {
        printf("buf_r alloc error!\n");
        return 1;
    }
  
    //读第一个页
    for(int pg=0;pg<2;pg++) {
        for (int i = 0; i < naddrs; ++i) {
            addrs[i].ppa = blk_addr.ppa;

            addrs[i].g.pg = 1;
            addrs[i].g.pl = (i / geo->nsectors) % geo->nplanes;
            addrs[i].g.sec = i % geo->nsectors;
        }
        memset(buf_r, '0', buf_r_nbytes);
        res = nvm_addr_read(dev, addrs, naddrs, buf_r, NULL, pmode, &ret);
        if (res < 0) {
            printf("read error!\n");
            return 1;
        }
    }


    printf("buf_w:%s\n",buf_w);
    
    printf("buf_r:%s\n",buf_r);
   
    return 0;
}
int main(){
    setup();
    test_sequ_r();
    teardown();
    return 0;
}
