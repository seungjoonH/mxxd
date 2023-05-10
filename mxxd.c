#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <ctype.h>

#define BUF_SIZ 1024
#define INDEX_LEN 9
#define HEX_LEN 41
#define ASCII_LEN 17

typedef enum {
	NO_ERROR, FILE_IO_ERR,
	WRG_IPT_ERR, ERR_LEN
} ErrorType;

typedef enum {
	BIN_TO_TXT, TXT_TO_BIN
} Mode;

int getFileSize(char *file);
int execute(Mode mode);
int binToTxt(FILE *infp, FILE *outfp);
int txtToBin(FILE *infp, FILE *outfp);
int binToBin(FILE *infp, FILE *outfp);

char *input;
char *output;
char *errMsg[ERR_LEN] = {
	0x0, "File open error",
	"Wrong input format\n./mxxd inputfile outputfile\n./mxxd -r inputfile outputfile",
};

int main(int argc, char **argv) {
	ErrorType err = NO_ERROR;
	bool reverse = false;
	if (argc > 1) reverse = !strcmp(argv[1], "-r");

	bool validInput = argc != 1
		&& (!reverse && argc == 3 || reverse && argc == 4);

	if (!validInput) err = WRG_IPT_ERR;

	if (!err) {
		Mode mode = reverse ? TXT_TO_BIN : BIN_TO_TXT;
		input = argv[mode + 1]; 
		output = argv[mode + 2];

		err = execute(mode);
	}

	if (err) fprintf(stderr, "[ERROR] %s\n", errMsg[err]);
	return err;
}

int getFileSize(char *file) {
	struct stat st;
	stat(file, &st);
	return st.st_size;
}

int execute(Mode mode) {
	int ret = 0;

	FILE *infp = fopen(input, "rb");
	FILE *outfp = fopen(output, "wb+");
	
	if (infp == NULL || outfp == NULL) return FILE_IO_ERR;
	
	switch (mode) {
		case BIN_TO_TXT: ret = binToTxt(infp, outfp); break;
		case TXT_TO_BIN: ret = txtToBin(infp, outfp); break;
	}

	fclose(infp);
	fclose(outfp);

	return ret;
}

int binToTxt(FILE *infp, FILE *outfp) {
	int ret = 0;
	
	int size = getFileSize(input);
	FILE *tmpfp = tmpfile();
	if (tmpfp == NULL) return FILE_IO_ERR;

	char buf[BUF_SIZ];
	char line[INDEX_LEN + HEX_LEN + ASCII_LEN + 2];
	char index[INDEX_LEN], hex[HEX_LEN], ascii[ASCII_LEN];

	while (size > 0) {
		int readSize = fread(buf, 1, BUF_SIZ, infp);

		for (int i = 0; i < readSize; i += 16) {
			*line = '\0'; *index = '\0'; 
			*hex = '\0';  *ascii = '\0';

			sprintf(index, "%08x", i);
			for (int j = 0; j < 16; j++) {
				unsigned char c = buf[i + j];
				sprintf(hex, "%s%02x", hex, c);
				if (j % 2) strcat(hex, " ");
				if (isprint(c)) sprintf(ascii, "%s%c", ascii, c);
				else sprintf(ascii, "%s.", ascii);
			}

			sprintf(line, "%s: %s %s\n", index, hex, ascii);
			fwrite(line, 1, strlen(line), tmpfp);
		}
		size -= readSize;
	}

	rewind(tmpfp);
	binToBin(tmpfp, outfp);

	return ret;
}

int txtToBin(FILE *infp, FILE *outfp) {
	char buf[BUF_SIZ];
	int size = 0;

	FILE *tmpfp = tmpfile();
	if (tmpfp == NULL) return FILE_IO_ERR;

	while (fgets(buf, BUF_SIZ, infp)) {
		bool validInput = true;

		char line[BUF_SIZ];
		strcpy(line, buf);

		validInput &= strlen(line) == 68;
		validInput &= strtok(line, ": ") != NULL;
		for (int i = 0; i < 9; i++) 
			validInput &= strtok(NULL, " ") != NULL;

		if (!validInput) return FILE_IO_ERR;

		buf[49] = '\0';
		char *hex = buf + 10;
		
		char *token = strtok(hex, " ");
		while (token) {
			unsigned int hexVal;
			sscanf(token, "%x", &hexVal);
			unsigned char a = hexVal / 256;
			unsigned char b = hexVal % 256;

			fwrite(&a, 1, 1, tmpfp);
			fwrite(&b, 1, 1, tmpfp);
			size += 2;

			token = strtok(NULL, " ");
		}

	}

	rewind(tmpfp);
	binToBin(tmpfp, outfp);

	return NO_ERROR;
}

int binToBin(FILE *infp, FILE *outfp) {
	char buf[BUF_SIZ];
	int r, w;

	while ((r = fread(buf, 1, BUF_SIZ, infp))) {
		w = fwrite(buf, 1, r, outfp);
		if (w < r) return 1;
	}

	return NO_ERROR;
}