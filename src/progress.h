#ifndef PROGREES_H
#define PROGRESS_H
#include <stdint.h>
#include <stdio.h>

struct progress {
	int bar_fd;
	FILE *bar_file;

	int lines_mode;
	int force_done;

	off_t size;
	off_t size_lines;

	/* used instead .size if not NULL */
	off_t (*size_cb)();

	off_t current_position;
	off_t current_position_lines;

	double start_time;
	double last_update_time;

};

void progress_init(struct progress *p, int bar_fd);
void progress_free(struct progress *p);

void progress_move(struct progress *p, off_t offset);
void progress_move_lines(struct progress *p, off_t offset);
void progress_draw(struct progress *p);
#endif
