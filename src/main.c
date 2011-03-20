#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/ttycom.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <ctype.h>

#include "progress.h"

struct copy_options {
	int read_only;
	int lines_mode;

	struct progress *bar;
};

void copy_options_init(struct copy_options *opt);

int copy(int src, int dst, struct copy_options *opt);
static struct progress PROGRESS;
static int DRAW_PROGRESS = 0;
off_t parse_size(const char *str, int base, char **suf);

void draw_progress(int sig);
off_t filesize(int fd);
void usage();

int main(int argc, char *argv[]) {
	int res;
	int src;
	char opt;
	int i;
	char *size_suf;

	int size_in_lines = -1;
	off_t size_base1000 = 0;
	off_t size_base1024 = 0;
	off_t user_defined_size = 0;
	off_t user_defined_size_lines = 0;

	struct copy_options copy_options;

	copy_options_init(&copy_options);

	copy_options.lines_mode = 0;
	copy_options.read_only = 0;
	copy_options.bar = &PROGRESS;

	while((opt = getopt(argc, argv, "rlhs:")) != -1) {
		switch(opt) {
			case 'r':
				copy_options.read_only = 1;
			break;
			case 'l':
				copy_options.lines_mode = 1;
				if(size_in_lines < 0)
					size_in_lines = 1;
			break;
			case 's':
				size_base1000 = parse_size(optarg, 1000, &size_suf);
				size_base1024 = parse_size(optarg, 1024, &size_suf);
				if(size_base1000 == -1) {
					fprintf(stderr, "Invalid argument: %s\n", optarg);
					exit(255);
				}

				if(strcasecmp(size_suf, "b") == 0) {
					size_in_lines = 0;
				} else if(strcasecmp(size_suf, "ln") == 0 || strcasecmp(size_suf, "l") == 0) {
					size_in_lines = 1;
				} else if(*size_suf != 0) {
					fprintf(stderr, "Invalid argument: %s\n", optarg);
					exit(255);
				}
			break;
			case 'h':
			case '?':
			default:
				usage();
				exit(255);
		}
	}

	if(size_base1000 || size_base1024) {
		if(size_in_lines > 0) {
			if(!copy_options.lines_mode) {
				fprintf(stderr, "Error: size in lines has no effect without -l option!\n");
				exit(255);
			}

			user_defined_size_lines = size_base1000;
		} else {
			user_defined_size = size_base1024;
		}
	}

	signal(SIGALRM, draw_progress);
	alarm(1);

	if(argc > optind) {
		for(i = optind; i < argc; i++) {
			if(argc - optind > 1)
				fprintf(stderr, "File %s:\n", argv[i]);

			if((src = open(argv[i], O_RDONLY)) < 1) {
				perror(argv[i]);
				exit(errno);
			}

			progress_init(&PROGRESS, STDERR_FILENO);
			PROGRESS.lines_mode = copy_options.lines_mode;
			PROGRESS.size = user_defined_size ? user_defined_size : (off_t)filesize(src);
			PROGRESS.size_lines = user_defined_size_lines;

			DRAW_PROGRESS = 1;
			res = copy(src, STDOUT_FILENO, &copy_options);
			if(res >= 0)
				PROGRESS.force_done = 1;

			progress_draw(&PROGRESS);
			DRAW_PROGRESS = 0;

			fputs("\n", stderr);
			if(res < 0) {
				perror("copy");
				exit(errno);
			}
			close(src);
		}
	} else {
		progress_init(&PROGRESS, STDERR_FILENO);
		PROGRESS.lines_mode = copy_options.lines_mode;

		/* if used `pp < file` form */
		PROGRESS.size = user_defined_size ? user_defined_size : (off_t)filesize(STDIN_FILENO);
		PROGRESS.size_lines = user_defined_size_lines;

		DRAW_PROGRESS = 1;
		res = copy(STDIN_FILENO, STDOUT_FILENO, &copy_options);
		if(res >= 0)
			PROGRESS.force_done = 1;
		progress_draw(&PROGRESS);
		DRAW_PROGRESS = 0;

		fputs("\n", stderr);
	}

	return EXIT_SUCCESS;
}

off_t filesize(int fd) {
	struct stat stat;
	off_t size;

	if(fstat(fd, &stat) < 0) {
		perror("stat");
		exit(errno);
	}

	if(stat.st_mode & S_IFBLK) {
		off_t old_pos = lseek(fd, 0, SEEK_CUR);

		if(old_pos < 0) {
			perror("lseek");
			exit(errno);
		}

		size = lseek(fd, 0, SEEK_END);
		if(size < 0) {
			perror("lseek");
			exit(errno);
		}

		if(lseek(fd, old_pos, SEEK_SET) < 0) {
			perror("lseek");
			exit(errno);
		}
	} else if(S_ISREG(stat.st_mode)) {
		size = stat.st_size;
	} else {
		size = 0;
	}

	return size;
}

int copy(int src, int dst, struct copy_options *opt) {
	char buf[1024*64];
	struct progress *p = opt->bar;
	int readed;
	int written, w;

	while(1) {
		readed = read(src, buf, sizeof(buf));
		if(readed == 0)
			break;
		if(readed < 0) {
			if(errno == EAGAIN) { /* signals */
				usleep(10000);
				continue;
			}
			return -1;
		}
		written = 0;

		while(written < readed) {
			if(opt->read_only)
				w = readed - written;
			else
				w = write(dst, buf + written, readed - written);

			if(w == 0) /* wtf? */
				return -1;

			if(w < 0) {
				if(errno == EAGAIN) {
					usleep(10000);
					continue;
				}
				return -1;
			}

			if(p->lines_mode) {
				off_t new_lines_count = 0;
				char *cur = buf + written;
				char *end = buf + written + w;

				while(cur != end) {
					if(*cur == '\n')
						new_lines_count++;
					cur++;
				}
				progress_move_lines(p, new_lines_count);
			}

			progress_move(p, (off_t)w);

			written += w;
		}
	}

	return 0;
}

void copy_options_init(struct copy_options *opt) {
	memset(opt, 0, sizeof(*opt));
}

void draw_progress(int s) {
	if(DRAW_PROGRESS)
		progress_draw(&PROGRESS);
	alarm(1);
}

off_t parse_size(const char *str, int base, char **suf) {
	char mul[] = {'\0', 'k', 'm', 'g', 't', 'p', 'e'/*, 'z', 'y'*/};
	char *inv;
	off_t size;

	unsigned long long l = strtoull(str, &inv, 0);

	if(l == ULLONG_MAX)
		return -1;

	size = (off_t)l;

	if(*inv != '\0') {
		size_t i;
		off_t mn = 1;

		for(i=0; i<sizeof(mul); i++) {
			if(tolower(*inv) == mul[i]) {
				size *= mn;
				inv++;
				break;
			}
			mn *= base;
		}
	}

	*suf = inv;

	return size;
}


void usage() {
	fputs(
		"Usage: pp [-lsr] [file-to-read]\n"
		"\t-l: line mode, show progress based on approximately\n"
		"\t    size of file in lines\n"
		"\t-r: do not write data to STDOUT\n"
		"\t-s: manually set size of pipe,\n"
		"\t    need when STDIN is not a file (10Mb, 1Gb, 1Tb etc)\n"
		"\n"
		"EXAMPLES:\n"
		"\tpp bigfile | gzip > bigfile.gz   progress of compression\n"
		"\tpp -r /dev/ad0                   determine HDD line read speed\n"
		"\tsomecmd | pp | gzip > file.gz    progress of pipe compression\n"
		"\tpp file > /path/to/copy          copy file with bar\n"
		,
		stderr
	);
}
/* */
