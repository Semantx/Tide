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
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct nav {
	char *name;
	char modifiers;
	int primaryShortcut;
	int secondaryShortcut;
	unsigned short size;
	struct nav *parent;
	struct nav *children;
};

void delete(struct nav *nav, int size);

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
				if (argv[i][2] == '\0') {
					if (++i < argc) {
						if (argv[i][1] == 'i') {
							iName = argv[i];
						} else {
							oName = argv[i];
						}
						continue;
					}
				} else {
					if (argv[i][1] == 'i') {
						iName = argv[i] + sizeof(char) * 2;
					} else {
						oName = argv[i] + sizeof(char) * 2;
					}
					continue;
				}
				fprintf(stderr, "Expected \"%s FILE\", got void.", argv[--i]);
				break;
			default:
				fprintf(stderr, "Expected \"-i\" or \"-o\", got \"%s\".\n", argv[i]);
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
			fprintf(stderr, "Error: Buffer overflow: strlen(\"%s%c%s\") > " SEMANTX_TIDE_DEV_NAV_STRING(PATH_MAX) ", a.k.a. PATH_MAX\n", buf, buf[size], iName);
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
			fprintf(stderr, "Error: fopen(\"%s\", \"%c\") failed: %s\n", buf, iFile ? 'w' : 'r', strerror(errno));
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
	void *ptr;
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
		if (c == EOF) {
			if (errno) {
				perror("Error: fgetc(iFile) failed");
				goto SEMANTX_TIDE_DEV_NAV_EXIT;
			}
		} else {
			switch (context) {
			case '\0':
				switch (c) {
				case 45:
				case 91:
					if (column == 1) {
						ptr = (struct nav *)realloc(global, sizeof(struct nav) * ++size);
						if (!ptr) {
							fprintf(stderr, "Error: realloc(global, %lu) failed: %s\n", sizeof(struct nav) * size, strerror(errno));
							goto SEMANTX_TIDE_DEV_NAV_EXIT;
						}
						global = ptr;
						local = global + sizeof(struct nav) * (size - 1);
						local->parent = NULL;
						SEMANTX_TIDE_DEV_NAV_NEW: {
							local->modifiers = '\0';
							local->primaryShortcut = 0;
							local->secondaryShortcut = 0;
							local->size = 0;
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
								ptr = (struct nav *)realloc(local->children, sizeof(struct nav) * (++local->size));
								if (!ptr) {
									fprintf(stderr, "Error: realloc(local->children, %lu) failed: %s\n", sizeof(struct nav) * local->size, strerror(errno));
									goto SEMANTX_TIDE_DEV_NAV_EXIT;
								}
								local->children = ptr;
								ptr = local;
								local = local->children + sizeof(struct nav) * (local->size - 1);
								local->parent = ptr;
								goto SEMANTX_TIDE_DEV_NAV_NEW;
							} else if (!local) {
								local = global + sizeof(struct nav) * (size - 1);
							} else if (local->children) {
								local = local->children + sizeof(struct nav) * (local->size - 1);
							} else {
								fprintf(stderr, "Error: '[' expected, got '\\t' (row %li, column %u).\n", row, i);
								goto SEMANTX_TIDE_DEV_NAV_EXIT;
							}
						}
					} else {
						fprintf(stderr, "Error: '[' expected, got '\\t' (row %li, column 1).\n", row);
						goto SEMANTX_TIDE_DEV_NAV_EXIT;
					}
				case 9:
					goto SEMANTX_TIDE_DEV_NAV_INPUT;
				}
				break;
			case '\x01':
				if (c == 93) {
					ptr = (char *)malloc(sizeof(char) * (column - offset));
					if (!ptr) {
						fprintf(stderr, "Error: malloc(%lu) failed: %s\n", sizeof(char) * (column - offset), strerror(errno));
						goto SEMANTX_TIDE_DEV_NAV_EXIT;
					}
					local->name = ptr;
					if (fseek(iFile, offset - column, SEEK_CUR)) {
						fprintf(stderr, "Error: fseek(iFile, %li, %i) failed: %s\n", offset - column, SEEK_CUR, strerror(errno));
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
					puts(local->name); // TEMPORARY
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
					if (c == 47 && (context += 4) == '\x08') {
						goto SEMANTX_TIDE_DEV_NAV_INPUT;
					}
					break;
				}
			case '\x08':
				if (offset) {
					switch(c) {
					case 'b':
						offset = KEY_BACKSPACE;
						break;
					case 'd':
						offset = KEY_DC;
						break;
					case 'i':
						offset = KEY_IC;
						break;
					default:
						offset = 0;
					}
					if (offset) {
						if (++context == '\x04') {
							local->primaryShortcut = offset;
						} else {
							local->secondaryShortcut = offset;
						}
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
					local->modifiers |= context * (50331648 >> c / 3) - (134217728 >> c / 3);
					goto SEMANTX_TIDE_DEV_NAV_INPUT;
				case 92:
					offset = 1;
					goto SEMANTX_TIDE_DEV_NAV_INPUT;
				}
				if (local->modifiers & context * 6 - 16) {
					if (c > 47 && c < 58) {
						if (++context == '\x04') {
							local->primaryShortcut = KEY_F(c - 48);
						} else {
							local->secondaryShortcut = KEY_F(c - 48);
						}
					} else if (c -= 87 > 9 && c < 25) {
						if (++context == '\x04') {
							local->primaryShortcut = KEY_F(c);
						} else {
							local->secondaryShortcut = KEY_F(c);
						}
					} else {
						break;
					}
					goto SEMANTX_TIDE_DEV_NAV_INPUT;
				}
				if (c > 96 && c < 123 || c > 47 && c < 58) {
					if (++context == '\x04') {
						local->primaryShortcut = c;
					} else {
						local->secondaryShortcut = c;
					}
					goto SEMANTX_TIDE_DEV_NAV_INPUT;
				}
				break;
			}
		}
	}
	if (c != EOF || !(context & context - 1)) {
		fprintf(stderr, "Error: Unexpected character '%c' (ASCII %i, row %li, column %li).\n", c, c, row, column);
		goto SEMANTX_TIDE_DEV_NAV_EXIT;
	}
	puts("\nLooping through after:"); // TEMPORARY
	for (int i = 0; i < size; ++i) { // TEMPORARY
		puts(global[i].name);
	}
	if (0) { // TEMPORARY
		SEMANTX_TIDE_DEV_NAV_EXIT: {
			errnum = EXIT_FAILURE;
		}
	}
	delete(global, size);
	if (fclose(iFile)) {
		perror("Error: fclose(iFile) failed");
		errnum = EXIT_FAILURE;
	}
	if (fclose(oFile)) {
		perror("Error: fclose(oFile) failed");
		return EXIT_FAILURE;
	}
	return errnum;
}

void delete(struct nav *nav, int size) {
	for (unsigned short i = 0; i < size; ++i) {
		printf("%i\n", nav[i].size);
		if (nav[i].size) {
			delete(nav[i].children, nav[i].size);
		}
	}
	free(nav);
}