#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <inttypes.h>
#include <math.h>

#include "progress.h"

static double gettimed() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (double)tv.tv_sec + (double)tv.tv_usec / 1000000;
}

static int get_terminal_width(int fd) {
	size_t width = 80;

#ifdef TIOCGWINSZ
	struct winsize wsz;

	if(ioctl(fd, TIOCGWINSZ, &wsz) < 0) {
		perror("ioctl(TIOCGWINSZ)");
		width = -1;
	} else {
		width = wsz.ws_col;
	}
#endif

	return width;
}

static char *human_size(char *buf, size_t buf_len, off_t size, int base, const char *posfix) {
	char *suf = "KMGTPEZY";
	char c[3] = "\0\0\0";
	double size_d = size;
	char *t;

	while(size_d >= base && *suf) {
		size_d /= base;
		c[0] = *suf;
		suf++;
	}

	if(c[0] && base == 1024)
		c[1] = 'i';

	snprintf(buf, buf_len, "%.1f", size_d);
	t = buf + strlen(buf) - 1;

	while(t >= buf && (*t == '0' || *t == '.')) {
		if(*t == '.') {
			*t = '\0';
			break;
		}
		*t = '\0';
		t--;
	}

	strncat(buf, " ", buf_len);
	strncat(buf, c, buf_len);
	strncat(buf, posfix, buf_len);

	return buf;
}

static char *human_time(char *buf, size_t buf_len, time_t interval) {
	int days = interval / (24*60*60);
	int hours = (interval % (24*60*60)) / (60*60);
	int minutes = (interval % (60*60)) / 60;
	int seconds = interval % 60;

	if(days)
		snprintf(buf, buf_len, "%02dd %02dh", days, hours);
	else if(hours)
		snprintf(buf, buf_len, "%02dh %02dm", hours, minutes);
	else
		snprintf(buf, buf_len, "%02dm %02ds", minutes, seconds);

	return buf;
}

void progress_init(struct progress *p, int bar_fd) {
	p->bar_fd = bar_fd;
	p->bar_file = fdopen(bar_fd, "w");
	setvbuf(p->bar_file, NULL, _IONBF, 0);

	p->size = 0;
	p->size_cb = NULL;
	p->size_lines = 0;

	p->current_position = 0;

	p->start_time = gettimed();
	p->last_update_time = 0;

	p->lines_mode = 0;
	p->force_done = 0;
}

void progress_free(struct progress *p) {
}

void progress_move(struct progress *p, off_t offset) {
	p->current_position += offset;
}

void progress_move_lines(struct progress *p, off_t offset) {
	p->current_position_lines += offset;
}

void progress_draw(struct progress *p) {
	char size_buf[32], time_buf[32];
	char percent_buf[8];
	char head[64];
	char tail[64];

	off_t avg_speed;
	off_t position;

	double complete_state;
	char *bar;
	int terminal_width;

	unsigned int bar_full_len, bar_len;
	off_t size;
	double ela, eta;

	terminal_width = get_terminal_width(p->bar_fd) - 3;
	if(terminal_width <= 25)
		return;

	ela = gettimed() - p->start_time;

	if(p->lines_mode) {
		size = p->size_lines;
		position = p->current_position_lines;
	} else {
		size = p->size_cb ? p->size_cb() : p->size;
		position = p->current_position;
	}

	avg_speed = (off_t)(ela ? position / ela : 0);

	if(!size && p->lines_mode) {
		if(p->current_position_lines && (p->current_position / p->current_position_lines))
			size = p->size / (p->current_position / p->current_position_lines);
	}

	eta = 0;

	if(size && position) {
		if(position > size)
			complete_state = 100500.0;
		else
			complete_state = (double)position / size;

		eta = (double)size / avg_speed - ela;
		if(eta < 0)
			eta = 0;
	} else {
		complete_state = -1.0;
	}

	snprintf(
		head,
		sizeof(head),
		"%10s",
		human_size(
			size_buf,
			sizeof(size_buf),
			position,
			p->lines_mode ? 1000 : 1024,
			p->lines_mode ? "ln" : "B"
		)
	);

	if(size) {
		snprintf(
			head + strlen(head),
			sizeof(head) - strlen(head),
			" / %s%-8s",
			(p->lines_mode && !p->size_lines) ? "~" : "",
			human_size(
				size_buf,
				sizeof(size_buf),
				size,
				p->lines_mode ? 1000 : 1024,
				p->lines_mode ? "ln" : "B"
			)
		);
	}

	strcat(head, " [");

	snprintf(
		tail,
		sizeof(tail),
		"] %-11s (%s %s)",
		human_size(
			size_buf,
			sizeof(size_buf),
			avg_speed,
			p->lines_mode ? 1000 : 1024,
			p->lines_mode ? "ln/s" : "B/s"
		),
		size && !p->force_done ? "eta" : "ela",
		human_time(
			time_buf,
			sizeof(time_buf),
			(time_t)(size && !p->force_done ?
				 eta
				:ela
			)
		)
	);

	if(complete_state < 0) {
		strncpy(percent_buf, " N/A ", sizeof(percent_buf));
	} else if(complete_state > 1.0) {
		strncpy(percent_buf, " >100% ", sizeof(percent_buf));
	} else {
		snprintf(
			percent_buf,
			sizeof(percent_buf),
			" %d%% ",
			p->force_done && p->lines_mode && !p->size_lines ? 100 : (unsigned int)(complete_state * 100)
		);
	}

	bar_full_len = terminal_width - (strlen(head) +  strlen(tail));

	if(bar_full_len < strlen(percent_buf))
		bar_full_len = strlen(percent_buf);

	if((p->force_done && p->lines_mode && !p->size_lines) || complete_state > 1.0) {
		bar_len = bar_full_len;
	} else {
		bar_len = (unsigned int)(complete_state * bar_full_len);
	}

	if(bar_len > bar_full_len)
		bar_len = bar_full_len;

	bar = (char *)alloca(bar_full_len + 1);
	memset(bar, '=', bar_len);
	memset(bar + bar_len, '-', bar_full_len - bar_len);
	memcpy(bar + bar_full_len / 2 - strlen(percent_buf) / 2, percent_buf, strlen(percent_buf));
	bar[bar_full_len] = 0;

	fputs("\r", p->bar_file);
	fputs(head, p->bar_file);
	fputs(bar, p->bar_file);
	fputs(tail, p->bar_file);
}

/* */
