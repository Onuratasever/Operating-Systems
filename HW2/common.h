#ifndef COMMON_H
#define COMMON_H
#include <unistd.h> // for open
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h>

#define MAX_FILE_SIZE 4 * 1024 * 1024 // 4 MB
#define FILE_NAME_LENGTH 30
#define SIZE_LENGTH 10
#define OWNER_PERMISSIONS_LENGTH 3
#define ATTRIBUTES_LENGTH 95
int BLOCK_SIZE;
struct attributes
{

	char *file_name;
	int size;
	int is_protected;
	char owner_permissions;
	char *last_modification_date;
	char *last_modification_time;
	char *creation_date;
	char *creation_time;
	char *password;

	// attributes()
	// {
	//     file_name = "";
	//     size = 0;
	//     is_protected = false;
	//     owner_permissions = '0';
	//     last_modification_date = "00";
	//     last_modification_time = "00";
	//     creation_date = "00";
	//     creation_time = "00";
	//     password = "0000";
	// }
};

struct directory_entry
{
	struct attributes attr;
	char *file_name;
	char *extension;
	char *attributes;
	char *reserved;
	char *time;
	char *date;
	char *first_block_number;
	char *size;
	// directory_entry()
	// {
	//     file_name = "";
	//     extension = "";
	//     attributes = "";
	//     reserved = "";
	//     time = "00";
	//     date = "00";
	//     first_block_number = "00";
	//     size = "0000";
	// }
};


char **splitPath(char *path, int *number_of_files)
{
	char **splittedPath = (char **)malloc(100 * sizeof(char *));

	// splittedPath[0] = strdup("NOPARENT");

	char *token = strtok(path, "/");

	// printf("token: %s\n", token);
	for (int i = 0; i < 100 && token != NULL; i++)
	{
		// printf("token: %s\n", token);
		splittedPath[i] = strdup(token);
		token = strtok(NULL, "/");
		(*number_of_files)++;
	}
	return splittedPath;
}

char *remove_spaces(const char *str)
{
	int len = strlen(str);
	char *result = (char *)malloc(len + 1); // Yeni string için bellek ayır

	if (result == NULL)
	{
		perror("Bellek ayırma hatası");
		exit(EXIT_FAILURE);
	}

	int i, j;
	for (i = 0, j = 0; str[i]; i++)
	{
		if (str[i] != ' ')
		{
			result[j++] = str[i];
		}
	}
	result[j] = '\0'; // Yeni stringi sonlandır

	return result;
}

int tokenize(char *words[3], char *buffer)
{
	char *token = strtok(buffer, ",");
	int word_count = 0;
	// printf("tokenize içine geldim: *%s*\n", buffer);
	while (token != NULL)
	{
		// Store the word in the words array
		// printf("token: *%s*\n", token);
		char *without_spaces = remove_spaces(token);
		words[word_count] = without_spaces; // BUNU SONRA FREELER
		if (words[word_count] == NULL)
		{
			perror("Error allocating memory");
			exit(EXIT_FAILURE);
		}
		(word_count)++;
		// Get the next token
		token = strtok(NULL, ",");
	}
	// printf("word_count: %d\n", word_count);
	return word_count;
}

off_t go_to_line(int file, int target_line)
{
	off_t offset;
	char read_char;
	int current_block = 0;

	// Dosya başına git
	offset = lseek(file, 0, SEEK_SET);
	if (offset == -1)
	{
		perror("lseek hatası go to line içinde");
		exit(EXIT_FAILURE);
	}

	// Hedef satıra kadar dosyayı oku
	while (current_block < target_line && read(file, &read_char, 1) > 0)
	{
		if (read_char == '\n')
		{
			current_block++;
		}
		offset++;
	}

	return offset;
}

int isExist(const char *fileSystem, char **splitted_path, int number_of_path, int path_counter, int block_number)
{
	// printf("Path counter: %d\n", path_counter);

	if (number_of_path == 0) // direkt root directory demektir
	{
		return 1; // Root demek. Root block number döndürülür yani 0
	}

	int file = open(fileSystem, O_RDONLY);
	if (file == -1)
	{
		perror("open");
		exit(-1);
	}

	int i = 0;

	go_to_line(file, block_number);
	// Dosyadaki imleci 83 karakter ileri taşı
	off_t offset = lseek(file, ATTRIBUTES_LENGTH, SEEK_CUR);
	//	off_t offset = lseek(file, 0, Current);

	if (offset == -1)
	{
		perror("lseek hatasıgg");
		close(file);
		return 0;
	}
	char entry;
	char asd[50];
	// read(file, &asd, 25);
	// printf("asd: *%s*\n", asd);
	char *tokens[3];
	int file_counter = 0;
	char read_char;
	int readByte;
	int j = 0;
	int word_count;
	int current_line = 0;
	while ((readByte = read(file, &read_char, 1)) > 0)
	{
		if (read_char == '\n')
		{
			i = -1;
			current_line++;
			return -2;
		}
		if (read_char != ';' && read_char != '\n')
		{
			asd[i] = read_char;
		}
		else if (read_char == ';' && read_char != '\n')
		{
			asd[i] = '\0';
			//  printf("tokenize gitmeden önce: i:%d *%s*\n", i, asd);

			tokenize(tokens, asd);
			if (strcmp(tokens[0], splitted_path[path_counter]) == 0)
			{
				path_counter++;
				if (path_counter == number_of_path)
				{
					close(file);
					return atoi(tokens[1]);
				}
				int return_value = isExist(fileSystem, splitted_path, number_of_path, path_counter, atoi(tokens[1]));
				close(file);
				// printf("return_value: %d\n", return_value);
				return return_value;
			}
			i = -1;
		}
		i++;
		if (i >= 28)
			i = 0;
	}
	close(file);
	return -2;
}

int isSameName(const char *fileSystem, char *name, int block_number)
{
	int file = open(fileSystem, O_RDONLY);
	if (file == -1)
	{
		perror("open");
		exit(-1);
	}

	int i = 0;

	go_to_line(file, block_number);
	// Dosyadaki imleci 83 karakter ileri taşı
	off_t offset = lseek(file, ATTRIBUTES_LENGTH, SEEK_CUR);
	//	off_t offset = lseek(file, 0, Current);

	if (offset == -1)
	{
		perror("lseek hatasıgg");
		close(file);
		return 0;
	}
	char entry;
	char asd[50];
	char *tokens[3];
	char read_char;
	int readByte;
	while ((readByte = read(file, &read_char, 1)) > 0)
	{
		if (read_char == '\n')
		{
			i = -1;
			return -1; // şu anlık tek satır daha sonra implement edilecek
		}
		if (read_char != ';' && read_char != '\n')
			asd[i] = read_char;
		else if (read_char == ';' && read_char != '\n')
		{
			asd[i] = '\0';
			// printf("tokenize gitmeden önce: i:%d *%s*\n", i, asd);

			tokenize(tokens, asd);
			// for (int k = 0; k < 3; k++)
			// {
			// 	printf("atokens[%d]: *%s*\n", k, tokens[k]);
			// }

			if (strcmp(tokens[0], name) == 0)
			{
				printf("Name is exist\n");
				close(file);
				return atoi(tokens[1]);
			}
			i = -1;
		}
		i++;
		if (i >= 28)
			i = 0;
	}
	close(file);
	printf("Name is not exist\n");
	return -1;
}

int findFirstAvailableBlock(int file)
{
	int i = 0;
	char read_char;
	int readByte;
	int current_block_number = 0;
	int check_if_block_is_available = 0;
	go_to_line(file, 0);
	while ((readByte = read(file, &read_char, 1)) > 0)
	{
		if (check_if_block_is_available == 1)
		{
			/* code */
			// printf("read_char: *%c*\n", read_char);
			if (read_char == ' ')
				return current_block_number;
			check_if_block_is_available = 0;
		}

		if (read_char == '\n')
		{
			current_block_number++;
			check_if_block_is_available = 1;
		}
	}
	close(file);
	return -1;
}

off_t find_last_semicolon(int file, int block_number, int *semicolonNumber)
{

	int offset = 0;
	int i = 0;
	char read_char;
	int readByte, count = 0;
	char entry = '9';
	while ((readByte = read(file, &read_char, 1)) > 0)
	{
		i++;
		// printf("read_char: *%c*\n", read_char);
		if (read_char == ';')
		{
			offset = i; // son ;
			count++;
		}
		else if (read_char == '\n')
			break;
	}
	// write(file, &entry, 1);
	*semicolonNumber = count;
	return offset;
}

off_t go_to_offset(int file, off_t offset)
{
	off_t offset2 = lseek(file, offset, SEEK_CUR);
	if (offset2 == -1)
	{
		perror("lseek hatası go to offset içinde");
		exit(EXIT_FAILURE);
	}
	return offset2;
}

int mkdir_operation(const char *fileSystem, char *name, int block_number, int isDirectory)
{
	int file = open(fileSystem, O_RDWR);
	if (file == -1)
	{
		perror("open");
		exit(-1);
	}

	int i = 0;

	int firstAvailableBlock = findFirstAvailableBlock(file);
	if (firstAvailableBlock == -1)
	{
		perror("First available block not found");
		exit(-1);
	}

	// Dosya başına git
	go_to_line(file, firstAvailableBlock);
	
	char buffer[96];
	char space[20];
	int len_file_name = strlen(name);
	for (i = 0; i < 20 - len_file_name; i++)
	{
		space[i] = ' ';
	}
	space[i] = '\0';
	struct stat file_stat;
	if (stat(fileSystem, &file_stat) == -1)
	{
		perror("stat");
		return 1;
	}
	time_t creation_time = file_stat.st_ctime;

	// Oluşturulma tarihini insan tarafından okunabilir biçime dönüştür
	struct tm *creation_tm = localtime(&creation_time);
	//if it is directory write 1 for isDirectory bit
	if (isDirectory == 1)
	{
		sprintf(buffer, "%s%s,size      ,100,0,          ,-1   ,%d-%02d-%02d %02d:%02d:%02d,%d-%02d-%02d %02d:%02d:%02d;", name, space,
				creation_tm->tm_year + 1900, creation_tm->tm_mon + 1, creation_tm->tm_mday,
				creation_tm->tm_hour, creation_tm->tm_min, creation_tm->tm_sec, creation_tm->tm_year + 1900, creation_tm->tm_mon + 1, creation_tm->tm_mday,
				creation_tm->tm_hour, creation_tm->tm_min, creation_tm->tm_sec);
	}
	else//if it is file write 0 for isDirectory bit
	{
		sprintf(buffer, "%s%s,size      ,000,0,          ,-1   ,%d-%02d-%02d %02d:%02d:%02d,%d-%02d-%02d %02d:%02d:%02d;", name, space,
				creation_tm->tm_year + 1900, creation_tm->tm_mon + 1, creation_tm->tm_mday,
				creation_tm->tm_hour, creation_tm->tm_min, creation_tm->tm_sec, creation_tm->tm_year + 1900, creation_tm->tm_mon + 1, creation_tm->tm_mday,
				creation_tm->tm_hour, creation_tm->tm_min, creation_tm->tm_sec);
	}
	char asd[50];
	char *tokens[3];
	char read_char;
	int readByte;
	
	//write attributes to the beginning of the block
	write(file, buffer, ATTRIBUTES_LENGTH);
	go_to_line(file, block_number); // parent directory'e git
	int semicolonNumber;
	off_t semicolon_offset = find_last_semicolon(file, block_number, &semicolonNumber);

	//go to beginning of the file and move to last semicolon
	go_to_line(file, block_number);
	go_to_offset(file, semicolon_offset);
	int count = 0;
	int number = firstAvailableBlock;

	while (number != 0)
	{
		number /= 10; // Sayıyı 10'a böl
		count++;
	}
	char space2[4];
	for (i = 0; i < 4 - count; i++)
	{
		space2[i] = ' ';
	}
	space2[i] = '\0';
	sprintf(buffer, "%s%s,%d%s,1;", name, space, firstAvailableBlock, space2);

	write(file, buffer, 28);
	close(file);
	return firstAvailableBlock;
}

void getAttributes(int block_number, int file, char *tokens[20])
{
	go_to_line(file, block_number);
	// char* tokens[20];
	char asd[ATTRIBUTES_LENGTH + 1];
	read(file, &asd, ATTRIBUTES_LENGTH);
	tokenize(tokens, asd);

}

int read_operation(const char *fileSystem, int block_number, char *file_name)
{
	int file = open(fileSystem, O_RDWR);
	if (file == -1)
	{
		perror("open");
		exit(-1);
	}

	char isPasswordProtected;
	go_to_line(file, block_number);
	lseek(file, 36, SEEK_CUR);
	read(file, &isPasswordProtected, 1);
	char password[11];
	lseek(file, 1, SEEK_CUR);
	if (isPasswordProtected == '1')
	{
		read(file, password, 10);
		password[10] = '\0';
		char *new_pass = remove_spaces(password);
		char input[11];
		do
		{
			printf("Enter password: ");
			fgets(input, sizeof(input), stdin);
			size_t len = strlen(input);
			if (len > 0 && input[len - 1] == '\n')
			{
				input[len - 1] = '\0';
			}
			//printf("input read: *%s*\n", input);

		} while (strcmp(input, new_pass) != 0);
	}

	// Dosya oluştur ve aç (yoksa oluştur, varsa içeriğini sil)
	int writeFile = open(file_name, O_WRONLY | O_CREAT | O_TRUNC, 0644);

	int i = 0;

	int start_point = go_to_line(file, block_number); // line başına gider

	char entry = '?';
	char buffer[96];

	struct stat file_stat;
	if (stat(fileSystem, &file_stat) == -1)
	{
		perror("stat");
		return 1;
	}
	time_t creation_time = file_stat.st_ctime;

	// Oluşturulma tarihini okunabilir biçime dönüştür
	struct tm *creation_tm = localtime(&creation_time);
	char *attributesTokens[20];
	//It gets attriubtes of the file
	getAttributes(block_number, file, attributesTokens);
	int next_block = atoi(attributesTokens[5]);
	int semicolonNumber;

	off_t semicolon_offset = lseek(file, 0, SEEK_CUR);
	entry = '!';
	off_t difference = 0;
	if (semicolonNumber != 1)
	{
		// printf("semicolon ilk değil\n");
		// semicolon_offset = lseek(file, -1, SEEK_CUR);
	}
	difference = semicolon_offset - start_point;
	go_to_line(file, block_number);
	go_to_offset(file, difference);
	char read_char;
	int readByte;
	int current_block = block_number;
	while ((readByte = read(file, &read_char, 1)) > 0)
	{
		if (read_char == ';')
			break;
		if (read_char == '\n') // yeni block bul ve ordan devam et
		{
			//Fixed size olarak data section başına git
			go_to_line(file, current_block);
			go_to_offset(file, 49);
			char next_block[6];
			read(file, next_block, 5);
			int new = atoi(next_block);
			go_to_line(file, new);
			char file_attributes[ATTRIBUTES_LENGTH + 1];
			//attribute'ları oku
			read(file, &file_attributes, ATTRIBUTES_LENGTH);
			current_block = new; //current block u güncelle
			if (current_block == -1)
				break;
		}
		else
		{
			write(writeFile, &read_char, 1);
		}
	}
	close(file);
	close(writeFile);
	return 0;
}

int write_operation(const char *fileSystem, int block_number, char *file_name)
{
	int file = open(fileSystem, O_RDWR);
	if (file == -1)
	{
		perror("open");
		exit(-1);
	}
	char isPasswordProtected;
	go_to_line(file, block_number);
	lseek(file, 36, SEEK_CUR);
	read(file, &isPasswordProtected, 1);
	char password[11];
	lseek(file, 1, SEEK_CUR);
	if (isPasswordProtected == '1')
	{
		// printf("Password protected\n");
		read(file, password, 10);
		password[10] = '\0';
		char *new_pass = remove_spaces(password);
		// printf("password read: *%s*\n", new_pass);
		char input[11];
		do
		{
			printf("Enter password: ");
			fgets(input, sizeof(input), stdin);
			size_t len = strlen(input);
			if (len > 0 && input[len - 1] == '\n')
			{
				input[len - 1] = '\0';
			}
			// printf("input read: *%s*\n", input);

		} while (strcmp(input, new_pass) != 0);
	}
	int readFile = open(file_name, O_RDONLY);

	int i = 0;

	int start_point = go_to_line(file, block_number); // line başına gider

	char entry = ';';
	char buffer[96];

	struct stat file_stat;
	if (stat(fileSystem, &file_stat) == -1)
	{
		perror("stat");
		return 1;
	}
	time_t creation_time = file_stat.st_ctime;

	// Oluşturulma tarihini insan tarafından okunabilir biçime dönüştür
	struct tm *creation_tm = localtime(&creation_time);
	char *attributesTokens[20];
	getAttributes(block_number, file, attributesTokens);
	int next_block = atoi(attributesTokens[5]);

	go_to_line(file, block_number);
	// write(file, &entry, 1);
	int semicolonNumber;
	off_t semicolon_offset = find_last_semicolon(file, block_number, &semicolonNumber); // burda line'ın en sonuna gidiyor
	go_to_line(file, block_number);

	off_t current_offset = go_to_offset(file, semicolon_offset);

	off_t difference = 0;
	if (semicolonNumber != 1)
	{
		printf("semicolon ilk değil\n");

		current_offset = lseek(file, -1, SEEK_CUR);
		// if (new_position == (off_t)-1)
		// {
		// 	perror("lseek (bir sola kaydır)");
		// 	close(file);
		// 	return EXIT_FAILURE;
		// }
		// difference = new_position - start_point;
	}
	// else
	// difference = semicolon_offset - start_point;

	difference = current_offset - start_point;
	// go_to_line(file, block_number);
	// go_to_offset(file, semicolon_offset);
	char read_char;
	int readByte;
	int current_block = block_number;
	int ai = 0;
	while ((readByte = read(readFile, &read_char, 1)) > 0)
	{
		if (difference == BLOCK_SIZE - 1) // yeni block bul ve ordan devam et
		{
			int firstAvailableBlock = findFirstAvailableBlock(file);
			// printf("firstAvailableBlock: %d\n", firstAvailableBlock);
			// printf("difference: %ld\n", difference);
			int number = firstAvailableBlock;
			if (firstAvailableBlock == -1)
			{
				perror("First available block not found");
				exit(-1);
			}
			go_to_line(file, current_block);
			go_to_offset(file, 49);
			char block_index[6]; // 20 byte genellikle int değeri için yeterli olacaktır
			int count = 0;
			while (number != 0)
			{
				number /= 10; // Sayıyı 10'a böl
				count++;
			}
			char space2[5];
			for (i = 0; i < 5 - count; i++)
			{
				space2[i] = ' ';
			}
			space2[i] = '\0';
			sprintf(block_index, "%d%s", firstAvailableBlock, space2);
			// printf("block_index: *%s*\n", block_index);
			write(file, block_index, 5);
			go_to_line(file, block_number);
			char file_attributes[ATTRIBUTES_LENGTH + 1];
			read(file, &file_attributes, ATTRIBUTES_LENGTH);
			file_attributes[49] = '-';
			file_attributes[50] = '1';
			file_attributes[51] = ' ';
			file_attributes[52] = ' ';
			file_attributes[53] = ' ';
			go_to_line(file, firstAvailableBlock);
			write(file, file_attributes, ATTRIBUTES_LENGTH);
			current_block = firstAvailableBlock;
			difference = 95;
		}
		else
		{
			write(file, &read_char, 1);
			difference++;
			ai++;
		}
	}
	write(file, &entry, 1);
	difference++;
	close(file);
	close(readFile);
	return 0;
}

int password_operation(const char *fileSystem, int return_block_number, char *password)
{
	int file = open(fileSystem, O_RDWR);
	if (file == -1)
	{
		perror("open");
		exit(-1);
	}

	int i = 0;

	go_to_line(file, return_block_number);
	// Dosyadaki imleci 83 karakter ileri taşı
	off_t offset = lseek(file, 36, SEEK_CUR);
	char entry = '1';
	write(file, &entry, 1);
	//	off_t offset = lseek(file, 0, Current);
	offset = lseek(file, 1, SEEK_CUR);
	char password_buffer[11];
	for (int i = 0; i < 10; i++)
	{
		if (i < strlen(password))
		{
			password_buffer[i] = password[i];
		}
		else
			password_buffer[i] = ' ';
	}
	// password_buffer[i] = '\0';
	// printf("password_buffer: *%s*\n", password_buffer);
	// length of pass
	// printf("length of pass: %ld\n", strlen(password_buffer));
	write(file, password_buffer, 10);
	if (offset == -1)
	{
		perror("lseek hatasıgg");
		close(file);
		return 0;
	}

	close(file);
	return -1;
}

int chmod_operation(const char *fileSystem, int return_block_number, char *permission)
{
	int file = open(fileSystem, O_RDWR);
	if (file == -1)
	{
		perror("open");
		exit(-1);
	}

	int i = 0;
	go_to_line(file, return_block_number);
	off_t offset = lseek(file, 33, SEEK_CUR);
	if(permission[0] == '+')
	{
		char entry = '1';
		write(file, &entry, 1);
		write(file, &entry, 1);
	}
	if(permission[0] == '-')
	{
		char entry = '0';
		write(file, &entry, 1);
		write(file, &entry, 1);
	}

}

int dir_operation(const char *fileSystem, int block_number)
{
	int file = open(fileSystem, O_RDONLY);
	if (file == -1)
	{
		perror("open");
		exit(-1);
	}

	int i = 0;

	go_to_line(file, block_number);
	// Dosyadaki imleci attributes karakter ileri taşı
	off_t offset = lseek(file, ATTRIBUTES_LENGTH, SEEK_CUR);
	if (offset == -1)
	{
		perror("lseek hatasıgg");
		close(file);
		return 0;
	}

	char entry;
	char asd[50];
	char *tokens[3];
	int file_counter = 0;
	char read_char;
	int readByte;
	int j = 0;
	int word_count;
	int current_line = 0;
	while ((readByte = read(file, &read_char, 1)) > 0)
	{
		if (read_char == '\n') //Satır sonuna geldi demek ki file ve directoryler bitti
		{
			i = -1;
			current_line++;
			return -2; 
		}
		if (read_char != ';' && read_char != '\n')
		{
			asd[i] = read_char;
		}
		else if (read_char == ';' && read_char != '\n')
		{
			asd[i] = '\0';

			tokenize(tokens, asd);
			off_t position = lseek(file, 0, SEEK_CUR);
			if (position == (off_t)-1)
			{
				perror("lseek");
				close(file);
				return EXIT_FAILURE;
			}
			char *attributesTokens[20];
			getAttributes(atoi(tokens[1]), file, attributesTokens);
			printf("name: %s, ", attributesTokens[0]);
			printf("drw: %s, ", attributesTokens[2]);
			printf("protected: %s, ", attributesTokens[3]);
			printf("creation date time: %s, ", attributesTokens[6]);
			printf("modification date time: %s\n", attributesTokens[7]);
			lseek(file, position, SEEK_SET);
			i = -1;
		}
		i++;
		if (i >= 28)
			i = 0;
	}
	close(file);
	return -2;
}

void init(const char *fileSystem)
{
	int file = open(fileSystem, O_RDONLY);
	if (file == -1)
	{
		perror("open");
		exit(-1);
	}
	go_to_line(file, 0);
	char entry;
	read(file, &entry, 1);
	if(entry == '0')
		BLOCK_SIZE = 512;
	else
		BLOCK_SIZE = 1024;
}
#endif