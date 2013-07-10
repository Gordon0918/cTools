#include <config_file.h>

FILE *config_file_open(char *filename)
{
    FILE *config_file;
    if ((config_file = fopen(filename,"a+")) == NULL ) {
        perror("open file failed\n");
    }

    return config_file;
}

int config_file_read(FILE *config_file, char *key, char *value)
{
    int  read_dat_size;
    char read_dat[FILE_ITEM_VALUE_SIZE + FILE_ITEM_KEY_SIZE];
    char *equal_sign_idx;

    memset(read_dat, 0, sizeof(read_dat));
    if (config_file == NULL) {

        printf("error file pointer \n");
        return -1;
    }

    fgets(read_dat,FILE_ITEM_MAXSIZE,config_file);

    read_dat_size = strlen(read_dat);
    if (read_dat_size > FILE_ITEM_MAXSIZE || read_dat_size < 3) {
        //printf("error size or read to the end\n");
        return -1;
    }

    if( (equal_sign_idx = strchr(read_dat,'=')) != NULL) {
        int key_size   = equal_sign_idx - read_dat;
        int value_size = read_dat_size - key_size -2;
        if (key_size < FILE_ITEM_KEY_SIZE && value_size < FILE_ITEM_VALUE_SIZE) {
            strncpy(key,read_dat,key_size);
            strncpy(value,equal_sign_idx + 1,value_size);
            key[key_size] = 0;
            value[value_size] = 0;
            return 0;
        }
    }

    return -1;
}


int config_file_append(FILE *config_file,char *key,char *value)
{
    int ret = -1;;
    if (config_file == NULL || key == NULL || value == NULL) {

        printf("confile_file is NULL\n");
        return -1;
    }

    ret = fprintf(config_file,"%s=%s\n",key,value);
    fflush(config_file);
    return ret;
}

int config_file_close(FILE *config_file)
{
    fclose(config_file);
    return 0;
}


int http_filter_config_load(char * filename,char *seg_key,int (* handle)(char *))
{
    FILE * fp;
    int ret = 0;
    //add bsfx
    char key[FILE_ITEM_KEY_SIZE];
    char value[FILE_ITEM_VALUE_SIZE];


    fp = config_file_open(filename);
    if (fp != NULL) {

        while(config_file_read(fp, key, value) == 0) {
            if(strcmp(key,seg_key) == 0) {
                ret |= handle(value);
            }
        }
    }
    config_file_close(fp);

    return ret;
}

