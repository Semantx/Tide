#if _POSIX_C_SOURCE < 200112L
#define _POSIX_C_SOURCE 200112L
#endif
#ifdef __linux__
#define SEMANTX_TIDE_DEV_NAV_OS "/proc/self/exe"
#elif defined(__FreeBSD__)
#define SEMANTX_TIDE_DEV_NAV_OS "/proc/curproc/file"
#elif ((defined(sun) || defined(__sun)) && (defined(__SVR4) || defined(__svr4__)))
#define SEMANTX_TIDE_DEV_NAV_OS "/proc/self/path/a.out"
#else
#error Build error: Invalid target (expected Linux, FreeBSD, or Solaris)
#endif
#define SEMANTX_TIDE_DEV_NAV_VALUE(arg) #arg
#define SEMANTX_TIDE_DEV_NAV_STRING(arg) SEMANTX_TIDE_DEV_NAV_VALUE(arg)

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct nav {
	char *name;
	unsigned char size;
	unsigned char modifiers;
	unsigned short shortcuts;
	struct nav *children;
};

char output(struct nav *nav, char size, FILE *oFile);
void success(struct nav *nav, char size, unsigned short depth);
void failure(struct nav *nav, char size);

int main(int argc, char **argv) {
	char *iName = "input.cfg";
	char *oName = "output.dat";
	for (int i = 1; i < argc; ++i) {
		if (argv[i][0] == '-') {
			switch(argv[i][1]) {
			case 'I':
			case 'O':
				argv[i][1] |= 32;
			case 'i':
			case 'o':
				if (argv[i][2]) {
					if (argv[i][1] == 'i') {
						iName = argv[i] + 2;
					} else {
						oName = argv[i] + 2;
					}
				} else if (++i < argc) {
					if (argv[i - 1][1] == 'i') {
						iName = argv[i];
					} else {
						oName = argv[i];
					}
				} else {
					fprintf(stderr, "Expected '-%c FILE', got void\n", argv[i - 1][1]);
					break;
				}
				continue;
			default:
				fprintf(stderr, "Expected '-i' or '-o', got '%s'\n", argv[i]);
			}
		}
		fputs("Optional arguments:\n-i FILE (default: input.cfg)\n-o FILE (default: output.dat)\n", stderr);
		return EXIT_SUCCESS;
	}
	char buf[PATH_MAX];
	int size = readlink(SEMANTX_TIDE_DEV_NAV_OS, buf, PATH_MAX);
	if (size < 2) {
		SEMANTX_TIDE_DEV_NAV_READLINK: {
			perror("Error: readlink(\"" SEMANTX_TIDE_DEV_NAV_OS "\", buf, " SEMANTX_TIDE_DEV_NAV_STRING(PATH_MAX) ") failed");
			return EXIT_FAILURE;
		}
	}
	while (--size >= 0 && buf[size] != '/' && buf[size] != '\\');
	if (buf[size] != '/' && buf[size] != '\\') {
		goto SEMANTX_TIDE_DEV_NAV_READLINK;
	}
	if (PATH_MAX < size + strlen(iName) + 2) {
		SEMANTX_TIDE_DEV_NAV_BUFFER: {
			buf[size + 1] = '\0';
			fputs("Error: File path is too long\n", stderr);
			fprintf(stderr, "Error: Buffer overflow: strlen(\"%s%c%s\" {iName}) exceeds " SEMANTX_TIDE_DEV_NAV_STRING(PATH_MAX) " {PATH_MAX}\n", buf, buf[size], iName);
			return EXIT_FAILURE;
		}
	}
	if (PATH_MAX < size + strlen(oName) + 2) {
		iName = oName;
		goto SEMANTX_TIDE_DEV_NAV_BUFFER;
	}
	strcpy(buf + ++size, iName);
	FILE *iFile = fopen(buf, "r");
	FILE *oFile;
	if (!iFile) {
		SEMANTX_TIDE_DEV_NAV_FOPEN: {
			fprintf(stderr, "Error: fopen(\"%s\" {buf}, \"%c\") failed: %s\n", buf, iFile ? 'w' : 'r', strerror(errno));
			return EXIT_FAILURE;
		}
	}
	strcpy(buf + size, oName);
	oFile = fopen(buf, "w");
	int errnum;
	if (!oFile) {
		errnum = errno;
		if (fclose(iFile)) {
			perror("Error: fclose(iFile) failed");
		}
		errno = errnum;
		goto SEMANTX_TIDE_DEV_NAV_FOPEN;
	}
	errnum = EXIT_SUCCESS;
	size = 0;
	struct nav *global = NULL;
	struct nav *local = NULL;
	void *pointer;
	long row = 1;
	long column = 0;
	long offset;
	char context = '\0';
	int c;
	SEMANTX_TIDE_DEV_NAV_INPUT: {
		c = fgetc(iFile);
		if (c == 13) {
			goto SEMANTX_TIDE_DEV_NAV_INPUT;
		}
		++column;
		if (c && c != EOF) {
			switch (context) {
			case '\0':
				switch (c) {
				case 45:
				case 91:
					if (column == 1) {
						if (c == 45) {
							fprintf(stderr, "Error: '[' expected, got '-' (row %li, column 1)\n", row);
							goto SEMANTX_TIDE_DEV_NAV_EXIT;
						}
						pointer = (struct nav *)realloc(global, sizeof(struct nav) * ++size);
						if (!pointer) {
							fprintf(stderr, "Error: realloc(global, %lu {sizeof(struct nav) * size}) failed: %s\n", sizeof(struct nav) * size, strerror(errno));
							goto SEMANTX_TIDE_DEV_NAV_EXIT;
						}
						global = pointer;
						local = global + size - 1;
						SEMANTX_TIDE_DEV_NAV_NEW: {
							local->size = '\0';
							local->modifiers = '\0';
							local->shortcuts = 0;
							local->children = NULL;
							if (c == 45) {
								local->name = NULL;
								context = '\x09';
							} else {
								offset = column;
								context = '\x01';
							}
							goto SEMANTX_TIDE_DEV_NAV_INPUT;
						}
					} else if (global) {
						local = NULL;
						for (unsigned i = 1; i <= column; ++i) {
							if (i == column) {
								pointer = (struct nav *)realloc(local->children, sizeof(struct nav) * (++local->size));
								if (!pointer) {
									fprintf(stderr, "Error: realloc(local->children, %lu {sizeof(struct nav) * local->size}) failed: %s\n", sizeof(struct nav) * local->size, strerror(errno));
									goto SEMANTX_TIDE_DEV_NAV_EXIT;
								}
								local->children = pointer;
								pointer = local;
								local = local->children + local->size - 1;
								goto SEMANTX_TIDE_DEV_NAV_NEW;
							} else if (!local) {
								local = global + size - 1;
							} else if (local->children) {
								local = local->children + local->size - 1;
							} else {
								fprintf(stderr, "Error: '[' expected, got '\\t' (row %li, column %u)\n", row, i);
								goto SEMANTX_TIDE_DEV_NAV_EXIT;
							}
						}
					} else {
						fprintf(stderr, "Error: '[' expected, got '\\t' (row %li, column 1)\n", row);
						goto SEMANTX_TIDE_DEV_NAV_EXIT;
					}
				case 9:
					goto SEMANTX_TIDE_DEV_NAV_INPUT;
				}
				break;
			case '\x01':
				if (c == 93) {
					pointer = (char *)malloc(sizeof(char) * (column - offset));
					if (!pointer) {
						fprintf(stderr, "Error: malloc(%lu {sizeof(char) * (column - offset)}) failed: %s\n", sizeof(char) * (column - offset), strerror(errno));
						goto SEMANTX_TIDE_DEV_NAV_EXIT;
					}
					local->name = pointer;
					if (fseek(iFile, offset - column, SEEK_CUR)) {
						fprintf(stderr, "Error: fseek(iFile, %li {offset - column}, %i {SEEK_CUR}) failed: %s\n", offset - column, SEEK_CUR, strerror(errno));
						goto SEMANTX_TIDE_DEV_NAV_EXIT;
					}
					column = offset;
					offset = -1;
					context = '\x02';
				} else if (c < 32 || c > 126) {
					break;
				}
				goto SEMANTX_TIDE_DEV_NAV_INPUT;
			case '\x02':
				++offset;
				if (c == 93) {
					local->name[offset] = '\0';
					offset = 0;
					context = '\x03';
				} else {
					local->name[offset] = (char)c;
				}
				goto SEMANTX_TIDE_DEV_NAV_INPUT;
			case '\x03':
			case '\x04':
			case '\x09':
				if (c == 10) {
					++row;
					column = 0;
					context = '\0';
					goto SEMANTX_TIDE_DEV_NAV_INPUT;
				} else if (context != '\x03') {
					if (((context <<= 1) & '\x18') == '\x08' && c == 47) {
						offset = 0;
						local->modifiers >>= 4;
						local->shortcuts <<= 8;
						goto SEMANTX_TIDE_DEV_NAV_INPUT;
					}
					break;
				}
			case '\x08':
				if (offset) {
					switch(c) {
					case 'b':
						offset = 128;
						break;
					case 'd':
						offset = 129;
						break;
					case 'i':
						offset = 130;
						break;
					default:
						offset = 0;
					}
					if (offset) {
						local->shortcuts |= offset;
						++context;
						goto SEMANTX_TIDE_DEV_NAV_INPUT;
					}
					break;
				}
				switch (c) {
				case 83:
					c = 72;
				case 65:
				case 67:
				case 70:
					local->modifiers |= 268435456 >> c / 3;
					goto SEMANTX_TIDE_DEV_NAV_INPUT;
				case 92:
					offset = 1;
					goto SEMANTX_TIDE_DEV_NAV_INPUT;
				}
				if (local->modifiers & ('\x38' - ++context * '\x06')) {
					if (c > 47 && c < 58) {
						local->shortcuts |= c - 48;
						goto SEMANTX_TIDE_DEV_NAV_INPUT;
					} else if (c > 96 && c < 112) {
						local->shortcuts |= c - 87;
						goto SEMANTX_TIDE_DEV_NAV_INPUT;
					}
				} else if (c > 96 && c < 123 || c > 47 && c < 58) {
					local->shortcuts |= c;
					goto SEMANTX_TIDE_DEV_NAV_INPUT;
				}
				break;
			}
		} else if (errno) {
			perror("Error: fgetc(iFile) failed");
			goto SEMANTX_TIDE_DEV_NAV_EXIT;
		}
	}
	if (c != EOF || !(context & context - '\x01') && context & '\xFB') {
		fprintf(stderr, "Error: Unexpected character '%c' (ASCII %i, row %li, column %li)\n", c, c, row, column);
		goto SEMANTX_TIDE_DEV_NAV_EXIT;
	}
	if (size > 254) {
		fprintf(stderr, "Error: %i {size} exceeds 254\n", size);
		errnum = EXIT_FAILURE;
	} else if (fprintf(oFile, "\\x%02x", 255 - size) < 0) {
		fprintf(stderr, "Error: fprintf(oFile, \"\\\\x%%02x\", %i {255 - size}) failed: %s\n", 255 - size, strerror(errno));
		errnum = EXIT_FAILURE;
	} else {
		if (output(global, (char)size, oFile)) {
			SEMANTX_TIDE_DEV_NAV_EXIT: {
				errnum = EXIT_FAILURE;
			}
		} else if (fputc(10, oFile) == EOF) {
			perror("Error: fputc(10, oFile) failed");
			errnum = EXIT_FAILURE;
		}
	}
	if (fclose(iFile)) {
		perror("Error: fclose(iFile) failed");
	}
	if (fflush(oFile)) {
		perror("Error: fflush(oFile) failed");
		errnum = EXIT_FAILURE;
	}
	if (fclose(oFile)) {
		perror("Error: fclose(oFile) failed");
		errnum = EXIT_FAILURE;
	}
	if (errnum == EXIT_SUCCESS) {
		puts("Output:");
		success(global, (char)size, 0);
	} else {
		fputs("Error: Unsuccessful output\n", stderr);
		failure(global, (char)size);
	}
	fflush(stdout);
	fflush(stderr);
	return errnum;
}

char output(struct nav *nav, char size, FILE *oFile) {
	if (size > 254) {
		fprintf(stderr, "Error: %i {size} exceeds 254\n", size);
		return '\x01';
	}
	for (int i = '\0'; i < size; ++i) {
		if (nav[i].name) {
			if (fputs(nav[i].name, oFile) == EOF) {
				fprintf(stderr, "Error: fputs(\"%s\" {nav[%i {i}].name}, oFile) failed: %s\n", nav[i].name, i, strerror(errno));
			} else if (fprintf(oFile, "\\x0%i", nav[i].shortcuts & 65280 ? 3 : nav[i].shortcuts ? 2 : 1) < 0) {
				fprintf(stderr, "Error: fprintf(oFile, \"\\\\x0%%i\", %i {nav[%i {i}].shortcuts & 65280 ? 3 : nav[%i {i}].shortcuts ? 2 : 1}) failed: %s\n", nav[i].shortcuts & 65280 ? 3 : nav[i].shortcuts ? 2 : 1, i, i, strerror(errno));
			} else if (nav[i].shortcuts & 255) {
				if (fprintf(oFile, "\\x%02x", nav[i].modifiers) < 0) {
					fprintf(stderr, "Error: fprintf(oFile, \"\\\\x%%02x\", %i {nav[%i {i}].modifiers}) failed: %s\n", nav[i].modifiers, i, strerror(errno));
				} else if (fprintf(oFile, "\\x%02x", nav[i].shortcuts & 255) < 0) {
					fprintf(stderr, "Error: fprintf(oFile, \"\\\\x%%02x\", %i {nav[%i {i}].shortcuts & 255}) failed: %s\n", nav[i].shortcuts & 255, i, strerror(errno));
				} else if (nav[i].shortcuts & 65280 && fprintf(oFile, "\\x%02x", nav[i].shortcuts >> 4 & 255) < 0) {
					fprintf(stderr, "Error: fprintf(oFile, \"\\\\x%%02x\", %i {nav[%i {i}].shortcuts >> 4 & 255}) failed: %s\n", nav[i].shortcuts >> 4 & 255, i, strerror(errno));
				} else {
					SEMANTX_TIDE_DEV_NAV_SIZE: {
						if (fprintf(oFile, "\\x%02x", 255 - nav[i].size) < 0) {
							fprintf(stderr, "Error: fprintf(oFile, \"\\\\x%%02x\", %i {255 - nav[%i {i}].size}) failed: %s\n", 255 - nav[i].size, i, strerror(errno));
						} else if (!(nav[i].size && output(nav[i].children, nav[i].size, oFile))) {
							continue;
						}
					}
				}
			} else {
				goto SEMANTX_TIDE_DEV_NAV_SIZE;
			}
		} else if (fputs("\\x04", oFile) != EOF) {
			continue;
		} else {
			perror("Error: fputs(\"\\\\x04\", oFile) failed");
		}
		return '\x01';
	}
	return '\0';
}

void success(struct nav *nav, char size, unsigned short depth) {
	for (int i = 0; i < size; ++i) {
		for (unsigned short j = 0; j < depth; ++j) {
			fputc(9, stdout);
		}
		if (nav[i].name) {
			printf("[ %s ]", nav[i].name);
			char *buf = " > ";
			while (nav[i].shortcuts & 255) {
				char shortcut = nav[i].shortcuts & 255;
				fputs(buf, stdout);
				if (nav[i].modifiers & '\x40') {
					fputs("Ctrl-", stdout);
				}
				if (nav[i].modifiers & '\x80') {
					fputs("Alt-", stdout);
				}
				if (nav[i].modifiers & '\x10') {
					fputs("Shift-", stdout);
				}
				if (nav[i].modifiers & '\x20') {
					fputc(70, stdout);
					if (shortcut > 9) {
						fputc(shortcut / 10 + 48, stdout);
					}
					fputc(shortcut % 10 + 48, stdout);
				} else {
					if (shortcut > 96 && shortcut < 123) {
						fputc(shortcut & 95, stdout);
					} else {
						fputs(shortcut == 128 ? "Backspace" : shortcut == 129 ? "Delete" : "Insert", stdout);
					}
				}
				buf = " or ";
				nav[i].modifiers <<= 4;
				nav[i].shortcuts >>= 8;
			}
			fputc(10, stdout);
			free(nav[i].name);
			if (nav[i].size) {
				success(nav[i].children, nav[i].size, depth + 1);
			}
		} else {
			puts("----------------");
		}
	}
	free(nav);
}

void failure(struct nav *nav, char size) {
	for (char i = '\0'; i < size; ++i) {
		if (nav[i].name) {
			free(nav[i].name);
			if (nav[i].size) {
				failure(nav[i].children, nav[i].size);
			}
		}
	}
	free(nav);
}
