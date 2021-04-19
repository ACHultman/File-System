/*
Author: Adam Hultman
Date: 30/10/2019
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <libgen.h>
#include <limits.h>
#include <assert.h>
#include <time.h>
#include <sys/mman.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>

// Super block
struct __attribute__((__packed__)) superblock_t {
    uint8_t fs_id [8];
    uint16_t block_size;
    uint32_t file_system_block_count;
    uint32_t fat_start_block;
    uint32_t fat_block_count;
    uint32_t root_dir_start_block;
    uint32_t root_dir_block_count;
};

// FAT info
struct fat_info_t {
    int free_blocks;
    int res_blocks;
    int alloc_blocks;
};

// Time and date entry
struct __attribute__((__packed__)) dir_entry_timedate_t {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
};

// Directory entry
struct __attribute__((__packed__)) dir_entry_t {
    uint8_t status;
    uint32_t starting_block;
    uint32_t block_count;
    uint32_t size;
    struct dir_entry_timedate_t modify_time;
    struct dir_entry_timedate_t create_time;
    uint8_t filename[31];
    uint8_t unused[6];
};

struct superblock_t sb;
struct fat_info_t fi;
struct dir_entry_timedate_t det;
struct dir_entry_t de;

void diskinfo(char **argv){
    int fd = open(argv[1], O_RDWR);
    struct stat buffer;
    if((fstat(fd, &buffer)) < 0){
        exit(1);
    }

    //template:   pa=mmap(addr, len, prot, flags, files, off);
    //c will implicitly cast void* to char*, while c++ does NOT
    char* address = mmap(NULL, buffer.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    uint32_t address_read = 0;

    memcpy(&sb.fs_id, address, 8);
    printf("Super block information:\n");

    memcpy(&address_read, address + 8, 2);
    sb.block_size = htons(address_read);
    printf("Block size: %d\n", sb.block_size);

    memcpy(&address_read, address + 10, 4);
    sb.file_system_block_count = htonl(address_read);
    printf("Block count: %d\n", sb.file_system_block_count);

    memcpy(&address_read, address+14, 4);
    sb.fat_start_block = htonl(address_read);
    printf("FAT starts: %d\n", sb.fat_start_block);

    memcpy(&address_read, address+18, 4);
    sb.fat_block_count = htonl(address_read);
    printf("FAT blocks: %d\n", sb.fat_block_count);

    memcpy(&address_read, address+22, 4);
    sb.root_dir_start_block = htonl(address_read);
    printf("Root directory start: %d\n", sb.root_dir_start_block);

    memcpy(&address_read, address+26, 4);
    sb.root_dir_block_count = htonl(address_read);
    printf("Root directory blocks: %d\n", htonl(address_read));


    long temp = 0;
    for(long i = sb.block_size * sb.fat_start_block;
        i < sb.block_size * (sb.fat_block_count+sb.fat_start_block);
        i+=0x4){
        memcpy(&temp, address+(i), 8);
        switch(htonl(temp)){
            case 0x1:
                fi.res_blocks++;
                break;
            case 0x0:
                fi.free_blocks++;
                break;
            default:
                fi.alloc_blocks++;
        }
    }

    printf("\nFAT Information:\n");
    printf("Free Blocks: %d\n", fi.free_blocks);
    printf("Reserved Blocks: %d\n", fi.res_blocks);
    printf("Allocated Blocks: %d\n", fi.alloc_blocks);

    munmap(address,buffer.st_size);
    close(fd);
    return;
}


struct dir_entry_t disklist(char **argv, int mode, char* target_file) {

    int fd = open(argv[1], O_RDWR);
    struct stat buffer;
    if((fstat(fd, &buffer)) < 0){
        exit(EXIT_FAILURE);
    }

    //template:   pa=mmap(addr, len, prot, flags, files, off);
    //c will implicitly cast void* to char*, while c++ does NOT
    void* address = mmap(NULL, buffer.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    uint32_t address_read = 0;

    memcpy(&sb.fs_id, address, 8);
    memcpy(&address_read, address + 8, 2);
    sb.block_size = htons(address_read);
    memcpy(&address_read, address + 10, 4);
    sb.file_system_block_count = htonl(address_read);
    memcpy(&address_read, address+14, 4);
    sb.fat_start_block = htonl(address_read);
    memcpy(&address_read, address+18, 4);
    sb.fat_block_count = htonl(address_read);
    memcpy(&address_read, address+22, 4);
    sb.root_dir_start_block = htonl(address_read);
    memcpy(&address_read, address+26, 4);
    sb.root_dir_block_count = htonl(address_read);


        int temp;
        for(long i = (sb.block_size * sb.root_dir_start_block) - 64; //27136
            i < sb.root_dir_block_count * 8 * sb.block_size;  //32768
            i+=64){

            memcpy(&de.status, address+(i+64), 1);
            if(de.status != 3 && de.status != 5) continue;

            memcpy(&temp, address+(i+64)+1, 4);
            de.starting_block = htonl(temp);

            memcpy(&temp, address+(i+64)+ 5, 4);
            de.block_count = htonl(temp);

            memcpy(&temp, address+(i+64)+9, 4);
            de.size = htonl(temp);

            memcpy(&temp, address+(i+64)+13, 2);
            det.year = htons(temp);
            memcpy(&det.month, address+(i+64)+15, 1);
            memcpy(&det.day, address+(i+64)+16, 1);
            memcpy(&det.hour, address+(i+64)+17, 1);
            memcpy(&det.minute, address+(i+64)+18, 1);
            memcpy(&det.second, address+(i+64)+19, 1);
            de.modify_time = det;

            memcpy(&temp, address+(i+64)+20, 2);
            det.year = htons(temp);
            memcpy(&det.month, address+(i+64)+22, 1);
            memcpy(&det.day, address+(i+64)+23, 1);
            memcpy(&det.hour, address+(i+64)+24, 1);
            memcpy(&det.minute, address+(i+64)+25, 1);
            memcpy(&det.second, address+(i+64)+26, 1);
            de.create_time = det;

            memcpy(&de.filename, address+(i+64)+27, 31);
            memcpy(&de.unused, address+(i+64)+58, 6);

            if(mode == 1 && strncmp(target_file, (char *) de.filename, strlen((char*)(de.filename))) == 0){
                return de;
            }

            if(de.size != 0){

                if(de.status == 3){
                    de.status = 'F';
                }
                else{
                    de.status = 'D';
                }
                if(mode == 0) {
                    printf("%c %10d %30s %u/%02u/%02u %u:%02u:%02u\n", de.status, de.size, de.filename,
                           de.modify_time.year, de.modify_time.month, de.modify_time.day, de.modify_time.hour,
                           de.modify_time.minute, de.modify_time.second);
                }
            }
        }
    munmap(address, buffer.st_size);
    close(fd);
    return de;
}

void diskget(int argc, char* argv[]) {

    if (argc != 4) {
        return;
    }

    int fd = open(argv[1], O_RDWR);
    struct stat buffer;
    if ((fstat(fd, &buffer)) < 0) {
        exit(EXIT_FAILURE);
    }

    //template:   pa=mmap(addr, len, prot, flags, files, off);
    //c will implicitly cast void* to char*, while c++ does NOT
    void *address = mmap(NULL, buffer.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    argv[2] = basename(argv[2]);
    //printf("New basename argv[2]: %s\n", argv[2]);
    struct dir_entry_t target_file = disklist(argv, 1, argv[2]);

    if(strncmp( (char *)target_file.filename, argv[2], strlen(argv[2])) != 0){
        printf("File not found.\n");
        return;
    }

    printf("File found: %s \n", target_file.filename);
    printf("File starting block: %u \n", target_file.starting_block);
    printf("File blocks: %u \n", target_file.block_count);
    printf("File size: %u \n", target_file.size);

    FILE* fp = fopen(argv[3], "wb+");
    if(!fp){
        exit(EXIT_FAILURE);
    }

    uint8_t file_data[target_file.size];
    int fat_entry = 0;
    int read_block_count = 1;
    if(target_file.size >= sb.block_size){
        memcpy(&file_data, address+(target_file.starting_block * sb.block_size), sb.block_size);
        fwrite(file_data, sizeof(char), sb.block_size, fp);
    }
    else{
        memcpy(&file_data, address+(target_file.starting_block * sb.block_size), target_file.size);
        fwrite(file_data, sizeof(char), target_file.size, fp);
    }

    for(long i = (sb.fat_start_block * sb.block_size) + (target_file.starting_block*4);
        read_block_count < target_file.block_count;
        i+=4){
        int remaining_size = target_file.size - (read_block_count * sb.block_size);
        memcpy(&fat_entry,address+i,4);
        fat_entry = htonl(fat_entry);

        if(remaining_size < sb.block_size){
            memcpy(&file_data, address+(fat_entry * sb.block_size), remaining_size);
            fwrite(file_data, sizeof(char), remaining_size, fp);
        }
        else {
            memcpy(&file_data, address+(fat_entry * sb.block_size), sb.block_size);
            fwrite(file_data, sizeof(char), sb.block_size, fp);
        }
        read_block_count++;
        printf("Fat entry: %d\n", fat_entry);
    }
    fclose(fp);
    munmap(address,buffer.st_size);
    close(fd);
    return;
}

void diskput(int argc, char* argv[]){

    if (argc != 4) {
        return;
    }

    int fd = open(argv[1], O_RDWR);
    struct stat buffer;
    if ((fstat(fd, &buffer)) < 0) {
        exit(EXIT_FAILURE);
    }
    printf("Opening: %s\n", argv[2]);
    FILE* fp = fopen(argv[2], "rb+");
    if(!fp){
        printf("File not found.\n");
        exit(EXIT_FAILURE);
    }

    //template:   pa=mmap(addr, len, prot, flags, files, off);
    //c will implicitly cast void* to char*, while c++ does NOT
    void *address = mmap(NULL, buffer.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    uint32_t address_read = 0;

    memcpy(&sb.fs_id, address, 8);

    memcpy(&address_read, address + 8, 2);
    sb.block_size = htons(address_read);

    memcpy(&address_read, address + 10, 4);
    sb.file_system_block_count = htonl(address_read);

    memcpy(&address_read, address+14, 4);
    sb.fat_start_block = htonl(address_read);

    memcpy(&address_read, address+18, 4);
    sb.fat_block_count = htonl(address_read);

    memcpy(&address_read, address+22, 4);
    sb.root_dir_start_block = htonl(address_read);

    memcpy(&address_read, address+26, 4);
    sb.root_dir_block_count = htonl(address_read);

    struct stat file_stats;
    printf("Fd: %s\n", argv[2]);
    stat(argv[2], &file_stats);
    printf("Found file, size: %ld\n", file_stats.st_size);

    struct dir_entry_t new_file;

    long j = sb.root_dir_start_block * sb.block_size;
    while(j < (sb.root_dir_start_block * sb.block_size)+(sb.root_dir_block_count * sb.block_size)
          && new_file.status != 0){
        memcpy(&new_file.status, address+j, 1);
        printf("new_file.status: %u\n", new_file.status);
        printf("new_file.status: %u\n", new_file.status);
        j+=64;
    }
    if(j == sb.root_dir_block_count*sb.block_size){
        printf("Directory full.\n");
        return;
    }
    new_file.status = 3;
    j-=64;

    printf("j: %ld\n", j);

    uint8_t file_data[sb.block_size];
    int fat_entry_value = 0;
    int fat_entry_count = 0;
    int fat_start_block = 0;
    long file_size = file_stats.st_size;
    long i;
    for(i = sb.fat_start_block * sb.block_size;
        i < sb.fat_block_count * sb.block_size && file_size != 0;
        i+=4){
        memcpy(&fat_entry_value, address+i, 4);
        printf("htonl(fat_entry_value): %u\n", htonl(fat_entry_value));
        fat_entry_count++;

        if(htonl(fat_entry_value) != 0) continue;

        fat_entry_value = htons(fat_entry_count-1);
        if(file_size == file_stats.st_size){
            fat_start_block = fat_entry_value;
        }
        printf("new fat_entry_value: %d\n", htons(fat_entry_value));
        memcpy(address+i, &fat_entry_value, 4);
        printf("Copied %u to %ld \n", htonl(fat_entry_value), i);

        if(file_size >= sb.block_size) {
            fread(file_data, sizeof(char), sb.block_size, fp);
            memcpy(address+(fat_entry_count*sb.block_size), file_data, sb.block_size);
            file_size-=sb.block_size;
            printf("Wrote 512 bytes to %u\n", fat_entry_count*sb.block_size);
        }
        else{
            fread(file_data, sizeof(char), file_size, fp);
            memcpy(address+(fat_entry_count*sb.block_size), file_data, file_size);
            printf("Wrote %ld bytes to %u\n", file_size, fat_entry_count);
            file_size = 0;
        }

    }

    char buffy_the_vampire_slayer[31] = "";
    int chomsky = 0;
    strcpy(buffy_the_vampire_slayer, argv[2]);
    for(int incremental_integer_variable = 0; incremental_integer_variable < 2; incremental_integer_variable++){
        chomsky = (int) buffy_the_vampire_slayer[incremental_integer_variable];
        memcpy(address+(j+27+incremental_integer_variable), &chomsky, 1);
    }

    uint64_t end = 0xFFFFFFFF;
    uint32_t end_real = 0xFFFF;
    memcpy(address+j, &new_file.status, 1); // status
    fat_start_block = ntohl(fat_start_block);
    memcpy(address+j+1, &fat_start_block, 4);
    int file_blocks = htonl(file_stats.st_blocks);
    memcpy(address+j+5, &file_blocks, 4);
    file_stats.st_size = htonl(file_stats.st_size);
    memcpy(address+j+9, &file_stats.st_size, 4);
    strncpy(address+j+27, buffy_the_vampire_slayer, 31);
    memcpy(address+i+58, &end, 4);
    memcpy(address+i+62, &end_real, 2);


    // create time & modify time

    time_t toime;
    time(&toime);
    struct tm *loc = localtime(&toime);

    uint8_t sec = loc->tm_sec;
    uint8_t min = loc->tm_min;
    uint8_t hour = loc->tm_hour;
    uint8_t day = loc->tm_mday;
    uint8_t month = loc->tm_mon;
    uint16_t year = htons((loc->tm_year)+1900);

    memcpy(address+(j+13), &year, 2);
    memcpy(address+(j+15), &month, 1);
    memcpy(address+(j+16), &day, 1);
    memcpy(address+(j+17), &hour, 1);
    memcpy(address+(j+18), &min, 1);
    memcpy(address+(j+19), &sec, 1);
    memcpy(address+(j+20), &year, 2);
    memcpy(address+(j+22), &month, 1);
    memcpy(address+(j+23), &day, 1);
    memcpy(address+(j+24), &hour, 1);
    memcpy(address+(j+25), &min, 1);
    memcpy(address+(j+26), &sec, 1);

    fclose(fp);
    munmap(address,buffer.st_size);
    close(fd);
    return;
}

void diskfix(){
    printf("Part 5\n");
}

int main(int argc, char* argv[]) {
#if defined(PART1)
    diskinfo(argv);
#elif defined(PART2)
    disklist(argv, 0, "");
#elif defined(PART3)
    diskget(argc, argv);
#elif defined(PART4)
    diskput(argc, argv);
#elif defined(PART5)
    diskfix(argc,argv);
#else
#   error "PART[12345] must be defined"
#endif
    return 0;
}

