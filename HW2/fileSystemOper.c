#include <stdio.h>
#include "common.h"

int main(int argc, char **argv)
{
    if (argc < 4) // burayı düzelt argüman 3 de olabilri
    {
        // cout << "Usage: " << argv[0] << " <block_size> <file_name>" << endl;
        printf("Usage: %s <file_name> <operation> <parameters>\n", argv[0]);
        return 1;
    }

    const char *fileSystem = argv[1];
    const char *operation = argv[2];
    char **splitted_path;
    int number_of_path = 0;
    if (argc > 3)
    {
        char *parameters = argv[3]; // arguman sayısı 3 ise bu olmucak onu ayarlaman lazım
        splitted_path = splitPath(parameters, &number_of_path);
        // printf("number_of_path: %d\n", number_of_path);
        // for (int i = 0; i < number_of_path; i++)
        // {
        //     printf("*%s*\n", splitted_path[i]);
        // }
    }

    int return_block_number;
    int path_counter = 0;
    int block_number = 1;
    init(fileSystem);
    if (strcmp(operation, "mkdir") == 0)
    {
        int file_block_num;
        // önce verilen path var mı kontrol et
        if ((return_block_number = isExist(fileSystem, splitted_path, number_of_path - 1, path_counter, block_number)) == -2)
        {
            // printf("Path not found return block number: %d\n", return_block_number);
            printf("Path not found\n");
            return 1;
        }
        else
        {
            // printf("Path found return block number: %d\n", return_block_number);
            if ((file_block_num = isSameName(fileSystem, splitted_path[number_of_path - 1], return_block_number)) != -1 )
            {
                printf("Same name found\n");
            }
            else
            {
                // printf("Same name not found\n");
                // Burda ekleme yapılcak
                mkdir_operation(fileSystem, splitted_path[number_of_path - 1], return_block_number, 1);
            }
        }
    }
    else if (strcmp(operation, "dir") == 0)
    {
        if ((return_block_number = isExist(fileSystem, splitted_path, number_of_path, path_counter, block_number)) == -2)
        {
            // printf("Path not found return block number: %d\n", return_block_number);
            printf("Path not found\n");
            return 1;
        }
        else
        {
            // printf("Path found return block number: %d\n", return_block_number);
            dir_operation(fileSystem, return_block_number);
        }
    }
    else if (strcmp(operation, "write") == 0)
    {
        if ((return_block_number = isExist(fileSystem, splitted_path, number_of_path - 1, path_counter, block_number)) == -2)
        {
            // printf("Path not found return block number: %d\n", return_block_number);
            printf("Path not found\n");
            return 1;
        }
        else
        {
            int file_block_num;
            if ((file_block_num = isSameName(fileSystem, splitted_path[number_of_path - 1], return_block_number)) != -1 )
            {
                printf("Same name found\n");
                // printf("file_block_num: %d\n", file_block_num);
                write_operation(fileSystem, file_block_num, argv[4]);
            }
            else
            {
                file_block_num = mkdir_operation(fileSystem, splitted_path[number_of_path - 1], return_block_number, 0);
                write_operation(fileSystem, file_block_num, argv[4]);
            }
        }
    }
    else if (strcmp(operation, "read") == 0)
    {
        if ((return_block_number = isExist(fileSystem, splitted_path, number_of_path, path_counter, block_number)) == -2)
        {
            // printf("Path not found return block number: %d\n", return_block_number);
            printf("Path not found\n");
            return 1;
        }
        else
        {
            // printf("Path found return block number: %d\n", return_block_number);
            int file_block_num;
            read_operation(fileSystem, return_block_number, argv[4]);
        }
    }
    else if (strcmp(operation, "addpw") == 0)
    {
        if ((return_block_number = isExist(fileSystem, splitted_path, number_of_path, path_counter, block_number)) == -2)
        {
            // printf("Path not found return block number: %d\n", return_block_number);
            printf("Path not found\n");
            return 1;
        }
        else
        {
            // printf("Path found return block number: %d\n", return_block_number);
            password_operation(fileSystem, return_block_number, argv[4]);
           
        }
    }
    else if(strcmp(operation, "chmod") == 0)
    {
        if ((return_block_number = isExist(fileSystem, splitted_path, number_of_path, path_counter, block_number)) == -2)
        {
            // printf("Path not found return block number: %d\n", return_block_number);
            printf("Path not found\n");
            return 1;
        }
        else
        {
            // printf("Path found return block number: %d\n", return_block_number);
            chmod_operation(fileSystem, return_block_number, argv[4]);
           
        }
    }
    else
    {
        printf("Operation not found\n");
    }

    return 0;
}