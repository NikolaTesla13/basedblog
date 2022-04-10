#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/limits.h>
#include <dirent.h>
#include <md4c.h>
#include <md4c-html.h>

char source_folder[PATH_MAX];

enum parsing_state {
	CONFIG, CONTENT, UNDEFINED
} pstate;

struct membuffer {
	char* data;
	size_t asize;
	size_t size;
};

static void membuf_init(struct membuffer* buf, size_t new_asize) {
	buf->size = 0;
	buf->asize = new_asize;
	buf->data = malloc(buf->asize);
	if(buf->data == NULL) {
		printf("membuf_init: malloc() failed.\n");
		exit(-1);
	}
}

static void membuf_clean(struct membuffer* buf) {
	if(buf->data) {
		free(buf->data);
		buf->size = 0;
		buf->asize = 0;
	}
}

static void membuf_resize(struct membuffer* buf, size_t new_asize) {
	buf->data = realloc(buf->data, new_asize);
	if(buf->data == NULL) {
		printf("membuf_resize: realloc() failed.\n");
		exit(-1);
	}
	buf->asize = new_asize;
}

static void membuf_append(struct membuffer* buf, const char* data, size_t size) {
	if(buf->asize < buf->size + size)
		membuf_resize(buf, 3*buf->size/2 + size);
	memcpy(buf->data + buf->size, data, size);
	buf->size += size;
}

static void md_callback(const char* text, size_t size, void* userdata) {
	membuf_append((struct membuffer*)userdata, text, size);
}

int main(int argc, char* argv[]) {
	for(unsigned int i=0; i<argc; i++) {
		if(strncmp(argv[i], "-h", strlen(argv[i])) == 0) {
			printf("this is a help message\n");
			return EXIT_SUCCESS;	
		} else if(strncmp(argv[i], "-i", strlen(argv[i])) == 0) {
			if(i == argc-1) {
				printf("not enough arguments\n");
				return EXIT_FAILURE;
			}
			strncpy(source_folder, argv[i+1], strlen(argv[i+1]));
		}
	}

	if(source_folder[0] == '\0') {
		strcpy(source_folder, "content");
	}

	FILE* out = fopen("build/index.html", "w+");
	if(out == NULL) {
		printf("error while opening the build file\n");
		return EXIT_SUCCESS;
	}

	DIR* directory;
	struct dirent* dir;
	directory = opendir(source_folder);
	if(directory) {
		while((dir = readdir(directory)) != NULL) {
			if(dir->d_name[0] == '.') 
				continue;
		
			char path[PATH_MAX] = "";
			strcat(path, source_folder);
			strcat(path, "/");
			strcat(path, dir->d_name);
		
			FILE* current_file = fopen(path, "rw");
			char* line = NULL;
			size_t len = 0;
			ssize_t read;
				
			pstate = UNDEFINED;
			char config[1024][1024] = { "" }, content_md[4096000] = "";
			unsigned int config_length = 0;

			while((read = getline(&line, &len, current_file)) != -1) {
				if(pstate == CONFIG && (line[0] != '-' && line[0] != '\n')) {
					char key[1024]="", value[2014]="";
					int ik=0, iv=0;
					for(int i=0; i<strlen(line); i++) {
						if(line[i] == '\n') continue;
						if(line[i] == ':') {
							ik = -1;
							i += 1;
							continue;
						}
						if(ik != -1) {
							key[ik] = line[i];
							ik++;
						} else {
							value[iv] = line[i];
							iv++;
						}
					}
					strcpy(config[config_length], key);
					strcpy(config[config_length+1], value);
					config_length += 2;
				} else if(pstate == CONTENT && (line[0] != '-' && line[0] != '\n')) {
					strcat(content_md, line);
				}
				if(strcmp(line+3, "Config --\n") == 0) {
					pstate = CONFIG;
				} else if(strcmp(line+3, "Content --\n") == 0) {
					pstate = CONTENT;
				}
			}
			
			printf("Processing the %s file.\n", dir->d_name);
			/*for(int i=0; i <= config_length-2; i+=2) {
				printf("key = %s; value = %s\n", config[i], config[i+1]);
			}
			printf("%s", content_md);*/
			
			struct membuffer buf_in = {0}, buf_out = {0};
			membuf_init(&buf_in, 32 * 1024);
			unsigned int content_len = strlen(content_md);
			if(content_len > buf_in.asize) {
				membuf_resize(&buf_in, buf_in.asize/2 + content_len);
			}
			membuf_append(&buf_in, content_md, content_len);
		
			membuf_init(&buf_out, (size_t)(buf_in.size + buf_in.size/8 + 64));
			int ret = md_html(buf_in.data, (size_t)buf_in.size, md_callback, (void*)&buf_out, NULL, MD_HTML_FLAG_DEBUG);
			if(ret != 0) {
				printf("parsing the file failed.\n");
				exit(ret);
			}	

			printf("%s", buf_out.data);

			membuf_clean(&buf_in);
			membuf_clean(&buf_out);
			fclose(current_file);
			free(line);
		}
		closedir(directory);
	}

	fclose(out);
	return EXIT_SUCCESS;
}
