#include <stdio.h>
#include "common.h"

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        // cout << "Usage: " << argv[0] << " <block_size> <file_name>" << endl;
        printf("Usage: %s <block_size> <file_name>\n", argv[0]);
        return 1;
    }

    float block_size = 1024 * atof(argv[1]); // 0.5 KB or 1 KB block sizes supported
    int number_of_blocks = MAX_FILE_SIZE / block_size;
    const char *file_name = argv[2];

    int file = open(file_name, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (file == -1)
    {
        perror("open");
        exit(-1);
    }

    struct stat file_stat;

    if (stat(file_name, &file_stat) == -1)
    {
        perror("stat");
        return 1;
    }

    // Dosyanın oluşturulma tarihini al
    time_t creation_time = file_stat.st_ctime;

    // Oluşturulma tarihini insan tarafından okunabilir biçime dönüştür
    struct tm *creation_tm = localtime(&creation_time);

    // Oluşturulma tarihini ekrana yaz
    printf("Dosyanın oluşturulma tarihi: %d-%02d-%02d %02d:%02d:%02d\n",
           creation_tm->tm_year + 1900, creation_tm->tm_mon + 1, creation_tm->tm_mday,
           creation_tm->tm_hour, creation_tm->tm_min, creation_tm->tm_sec);

    char buffer[96];
    //name, size, drw, isProtected,password, nextblock, creationTime, lastModificationTime
    sprintf(buffer, "/                   ,size      ,100,0,          ,-1   ,%d-%02d-%02d %02d:%02d:%02d,%d-%02d-%02d %02d:%02d:%02d;",
            creation_tm->tm_year + 1900, creation_tm->tm_mon + 1, creation_tm->tm_mday,
            creation_tm->tm_hour, creation_tm->tm_min, creation_tm->tm_sec, creation_tm->tm_year + 1900, creation_tm->tm_mon + 1, creation_tm->tm_mday,
            creation_tm->tm_hour, creation_tm->tm_min, creation_tm->tm_sec);
    // /                   ,size      ,000,0,          ,-1   ;2024-06-07 22:20:13,2024-06-07 22:20:13;
    // printf("buffer: *%s*\n", buffer);
    char space = ' ';
    char blocksize[5];
    char entry;
    if(block_size == 0,5)
    {
        entry = '0';
    }
    else
    {
        entry = '1';
    }
    sprintf(blocksize, "%c", entry);
    for (int i = 0; i < number_of_blocks; i++)
    {
        if(i == 0)
        {
            write(file, &entry, 1);
        }
        if (i == 1)
        {
            write(file, buffer, 95);
        }

        for (int j = 0; j < block_size - 1; j++)
        {

            if (j >= 95 && i == 0)
                write(file, &space, 1);
            else if(i != 0)
                write(file, &space, 1);
        }
        if (i != block_size - 1)
            write(file, "\n", 1);
        else
            write(file, &space, 1);
    }

    if (close(file) == -1)
    {
        perror("Dosya kapatılamadı");
        return 1;
    }
    return 0;
}