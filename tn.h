#ifndef _TN_H
#define _TN_H

#define TN_MAX_CMD_LEN (32)
#define TN_MAX_OPT (7)

struct tn_opt {
	unsigned char opt;
	unsigned char i_do : 1;
	unsigned char can_will : 1;
	unsigned char can_wont : 1;
	unsigned char sent_will : 1;
	unsigned char sent_do : 1;
};

struct tn_contex {
	unsigned char telwish;
	int state;
	struct tn_opt opt[TN_MAX_OPT + 1];

	void (*send)(void *data, void *buf, int len);
	void *data;
};

void tn_init(struct tn_contex *ctx,
             void (*send_cb)(void *data, void *buf, int len),
             void *data);
int tn_parse(struct tn_contex *ctx, void *buf, int len);
void tn_do_send(struct tn_contex *ctx, void *buf, int len);
int tn_prepare_send(void *buf, int len, void *outbuf, int olen);

#endif
