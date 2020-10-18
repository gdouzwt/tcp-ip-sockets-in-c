#include<stdio.h>
#include<json-c/json.h>
#include <string.h>
#include <errno.h>

long getSizeOfInput(FILE *input){
    long retvalue = 0;
    int c;

    if (input != stdin) {
        if (-1 == fseek(input, 0L, SEEK_END)) {
            fprintf(stderr, "Error seek end: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        if (-1 == (retvalue = ftell(input))) {
            fprintf(stderr, "ftell failed: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        if (-1 == fseek(input, 0L, SEEK_SET)) {
            fprintf(stderr, "Error seek start: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
    } else {
        /* for stdin, we need to read in the entire stream until EOF */
        while (EOF != (c = fgetc(input))) {
            retvalue++;
        }
    }

    return retvalue;
}

int main(int argc, char **argv) {
	FILE *fp;

    if (argc > 1) {
        if(!strcmp(argv[1],"-")) {
            fp = stdin;
        } else {
            fp = fopen(argv[1],"r");
            if (NULL == fp) {
                fprintf(stderr, "Unable to open '%s': %s\n",
                        argv[1], strerror(errno));
                exit(EXIT_FAILURE);
            }
        }
    } else {
        fp = stdin;
    }

	char buffer[1024];
	struct json_object *parsed_json;
	struct json_object *cmd;
	struct json_object *model;
    struct json_object *sid;
    struct json_object *short_id;
    struct json_object *token;
	struct json_object *data;
	//size_t n_friends;

	//size_t i;

	//fp = fopen("test.json","r");
	fread(buffer, 1024, 1, fp);
	fclose(fp);

	parsed_json = json_tokener_parse(buffer);

	json_object_object_get_ex(parsed_json, "cmd", &cmd);
	json_object_object_get_ex(parsed_json, "model", &model);
	json_object_object_get_ex(parsed_json, "sid", &sid);
	json_object_object_get_ex(parsed_json, "short_id", &short_id);
	json_object_object_get_ex(parsed_json, "token", &token);
	json_object_object_get_ex(parsed_json, "data", &data);

	printf("cmd: %s\n", json_object_get_string(cmd));
	printf("model: %s\n", json_object_get_string(model));
	printf("sid: %s\n", json_object_get_string(sid));
	printf("short_id: %s\n", json_object_get_string(short_id));
	printf("token: %s\n", json_object_get_string(token));
	printf("data: %s\n", json_object_get_string(data));

	//n_friends = json_object_array_length(sid);
	//printf("Found %lu sid\n",n_friends);

	/*for(i=0;i<n_friends;i++) {
		friend = json_object_array_get_idx(sid, i);
		printf("%lu. %s\n",i+1,json_object_get_string(friend));
	}*/
}