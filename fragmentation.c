#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <stdint.h>
//MOUNTPOINT, PREWRITE-GB, AFTERWRITE-GB, READTEST

static uint64_t get_micro_time(timeval res){
	return res.tv_usec+res.tv_sec*1000000;
}
int main(int argc, char *argv[]){
	if(argc<6){
		printf("usage: %s [MOUNTPOINT] [PREWRITE] [PROWRITE] [INTERVAL] [READTEST (1,0)]\n", argv[0]);
		return -1;
	}

	char block[4096];
	char *mount_ptr=argv[1];
	uint32_t first_file_byte_GB=atoi(argv[2]);
	uint64_t first_file_LB=(uint64_t)first_file_byte_GB*1024*1024/4;

	uint32_t second_file_byte_GB=atoi(argv[3]);
	uint64_t second_file_LB=(uint64_t)second_file_byte_GB*1024*1024/4;

	uint32_t interval=atoi(argv[4]);
	if(interval==0){
		printf("input is 0, setting interval to 2\n");
		interval=2;
	}
	bool readtest=atoi(argv[5]);

	char first_file_name[128];
	sprintf(first_file_name, "%s1", mount_ptr);
	printf("first file name: %s\n",first_file_name);

	char second_file_name[128];
	sprintf(second_file_name, "%s2", mount_ptr);
	printf("second file name: %s\n",second_file_name);


	printf("------file <1> size: %u GB\n", first_file_byte_GB);
	printf("------file <1> size: %u GB\n", second_file_byte_GB);
	printf("------punch hole interval %u\n", interval);

	int ffd=open(first_file_name, O_CREAT|O_RDWR|O_TRUNC, 0666);
	int sfd=open(second_file_name, O_CREAT|O_RDWR|O_TRUNC, 0666);
	
	if(ffd==-1 || sfd==-1){
		perror("file open error!!\n");
		return -1;
	}

	printf("first file write!!!\n");
	struct timeval st, et, res;
	gettimeofday(&st, NULL);
	int cnt=0;
	for(uint64_t i=0; i<first_file_LB; i++){
		pwrite(ffd, block, 4096, i*4096);
		if(i==first_file_LB/100*cnt){
			printf("\r%lu/%lu done ....", first_file_LB/100*cnt, first_file_LB);
			cnt++;
		}
	}
	fsync(ffd);
	gettimeofday(&et, NULL);
	timersub(&et, &st, &res);
	uint64_t micro_time=get_micro_time(res);
	printf("\nfirst file write done:%.6lf sec\n", (double)micro_time/1000000);

	printf("make hole\n");
	gettimeofday(&st, NULL);
	cnt=0;
	for(uint64_t i=0; i<first_file_LB/interval; i++){
		fallocate(ffd, FALLOC_FL_PUNCH_HOLE|FALLOC_FL_KEEP_SIZE, 4096*(i*interval), 4096);
		//pwrite(ffd, block, 4096, i*4096);
		if(i==first_file_LB/2/1000*cnt){
			printf("\r%lu/%lu done ....", first_file_LB/2/1000*cnt, first_file_LB/2);
			cnt++;
		}
		fsync(ffd);
	}
	gettimeofday(&et, NULL);
	timersub(&et, &st, &res);
	micro_time=get_micro_time(res);
	printf("\nmake hole done:%.6lf sec\n", (double)micro_time/1000000);

	printf("second file write\n");
	gettimeofday(&st, NULL);
	cnt=0;
	for(uint64_t i=0; i<second_file_LB; i++){
		pwrite(sfd, block, 4096, i*4096);
		//pwrite(ffd, block, 4096, i*4096);
		if(i==second_file_LB/100*cnt){
			printf("\r%lu/%lu done ....", second_file_LB/100*cnt, second_file_LB);
			cnt++;
		}
	}
	fsync(sfd);
	gettimeofday(&et, NULL);
	timersub(&et, &st, &res);
	micro_time=get_micro_time(res);
	printf("\nsecond file write done:%.6lf sec\n", (double)micro_time/1000000);

	if(!readtest){
		return 0;
	}

	system("echo 3 > /proc/sys/vm/drop_caches");
	uint32_t read_unit=32;//128KB
	char *read_buffer=(char*)malloc(32*4096);
	printf("first file read\n");
	gettimeofday(&st, NULL);
	cnt=0;
	for(uint64_t i=0; i<first_file_LB/read_unit; i++){
		read(ffd, read_buffer, read_unit*4096);
		//pwrite(ffd, block, 4096, i*4096);
		if(i==first_file_LB/read_unit/100*cnt){
			printf("\r%lu/%lu done ....", first_file_LB/read_unit/100*cnt, first_file_LB/read_unit);
			cnt++;
		}
	}
	gettimeofday(&et, NULL);
	timersub(&et, &st, &res);
	micro_time=get_micro_time(res);
	printf("\nfirst file read done:%.6lf sec\n", (double)micro_time/1000000);


	system("echo 3 > /proc/sys/vm/drop_caches");
	printf("second file read\n");
	gettimeofday(&st, NULL);
	cnt=0;
	for(uint64_t i=0; i<second_file_LB/read_unit; i++){
		read(ffd, read_buffer, read_unit*4096);
		//pwrite(ffd, block, 4096, i*4096);
		if(i==second_file_LB/read_unit/100*cnt){
			printf("\r%lu/%lu done ....", second_file_LB/read_unit/100*cnt, second_file_LB/read_unit);
			cnt++;
		}
	}
	gettimeofday(&et, NULL);
	timersub(&et, &st, &res);
	micro_time=get_micro_time(res);
	printf("\nsecond file read done:%.6lf sec\n", (double)micro_time/1000000);
	free(read_buffer);
	return 0;
}
