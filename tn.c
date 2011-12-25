#include <string.h>
#include <arpa/telnet.h>
#include <stdio.h>
#include "tn.h"

enum {
	TS_NORMAL	= 0,
	TS_COPY		= 1,
	TS_IAC,
	TS_OPT,
	TS_SUB1,
	TS_SUB2,
	TS_CR,
};

static struct tn_opt def_opts[] = {
	{TELOPT_SGA,	1, 1, 0, 1, 0},
	{TELOPT_ECHO,	1, 1, 1, 1, 1},
	{TELOPT_BINARY, 1, 1, 0, 0, 1},
	{255},
};

static unsigned char init_seq[] = {
	IAC, WILL, TELOPT_SGA,
	IAC, WILL, TELOPT_ECHO,
	IAC, DONT, TELOPT_ECHO,
	IAC, DO, TELOPT_BINARY
};

static struct tn_opt *find_opt(struct tn_contex *ctx, unsigned char opt)
{
	struct tn_opt *opts = &ctx->opt[0];

	while (opts->opt != 255) {
		if (opts->opt == opt)
			return opts;
		opts++;
	}
	return NULL;
}

static void tn_sendi(struct tn_contex *ctx, unsigned char type,
                     unsigned char opt)
{
	unsigned char iac[3];

	iac[0] = IAC;
	iac[1] = type;
	iac[2] = opt;
	ctx->send(ctx->data, iac, 3);
}

static void tn_do_opt(struct tn_contex *ctx, unsigned char opt)
{
	struct tn_opt *m_opt = find_opt(ctx, opt);

	if (m_opt == NULL) {
		if (ctx->telwish == DO || ctx->telwish == DONT)
			tn_sendi(ctx, WONT, opt);
		if (ctx->telwish == WILL || ctx->telwish == WONT)
			tn_sendi(ctx, DONT, opt);
		return;
	}

	switch (ctx->telwish) {
	case WILL:
		if (!m_opt->sent_do) {
			if (!m_opt->can_will)
				tn_sendi(ctx, DONT, opt);
			else
				tn_sendi(ctx, DO, opt);
		}
		m_opt->sent_do = !m_opt->sent_do;
		break;
	case WONT:
		if (!m_opt->sent_do) {
			if (!m_opt->can_wont)
				tn_sendi(ctx, DO, opt);
			else
				tn_sendi(ctx, DONT, opt);
		}
		m_opt->sent_do = !m_opt->sent_do;
		break;
	case DO:
		if (!m_opt->sent_will) {
			if (!m_opt->i_do)
				tn_sendi(ctx, WONT, opt);
			else
				tn_sendi(ctx, WILL, opt);
		}
		m_opt->sent_will = !m_opt->sent_will;
		break;
	case DONT:
		if (!m_opt->sent_will) {
			if (!m_opt->i_do)
				tn_sendi(ctx, WONT, opt);
			else
				tn_sendi(ctx, WILL, opt);
		}
		m_opt->sent_will = !m_opt->sent_will;
		break;
	}
}

static void tn_do_subneg(struct tn_contex *ctx, unsigned char c)
{
	switch (ctx->state) {
	case TS_SUB1:
		if (c == IAC)
			ctx->state = TS_SUB2;
		break;
	case TS_SUB2:
		if (c == SE) {
			ctx->state = TS_COPY;
			return;
		}
		ctx->state = TS_SUB1;
		break;
	}
}

int tn_parse(struct tn_contex *ctx, void *buf, int len)
{
	unsigned char *cbuf = buf;
	int i;
	int cstart = 0;
	unsigned char c;

	for (i = 0; i < len; i++) {
		c = cbuf[i];

		if (ctx->state == TS_NORMAL) {
			if (c == IAC) {
				cstart = i;
				ctx->state = TS_IAC;
			} else if (c == '\r') {
				cstart = i + 1;
				ctx->state = TS_CR;
			}
			continue;
		}
		switch (ctx->state) {
		case TS_CR:
			ctx->state = TS_COPY;
			if (c == '\0')
				break;
			/* fall through */
		case TS_COPY:
			if (c == IAC)
				ctx->state = TS_IAC;
			else
				cbuf[cstart++] = c;
			if (c == '\r')
				ctx->state = TS_CR;
			break;
		case TS_IAC:
			if (c == IAC) {
				cbuf[cstart++] = c;
				ctx->state = TS_COPY;
				break;
			}
			switch (c) {
			case SB:
				ctx->state = TS_SUB1;
				break;
			case DO:
			case DONT:
			case WILL:
			case WONT:
				ctx->telwish = c;
				ctx->state = TS_OPT;
				break;
			default:
				ctx->state = TS_COPY;
			}
			break;
		case TS_OPT:
			tn_do_opt(ctx, c);
			ctx->state = TS_COPY;
			break;
		case TS_SUB1:
		case TS_SUB2:
			tn_do_subneg(ctx, c);
			break;
		}
	}

	if (ctx->state != TS_NORMAL) {
		if (ctx->state == TS_COPY)
			ctx->state = TS_NORMAL;
		len = cstart;
	}

	return len;
}

int tn_prepare_send(void *buf, int len, void *outbuf, int olen)
{
	char *p = buf;
	int j = 0;
	char *obuf = outbuf;

	for (; len > 0 && j < olen; len--, p++) {
		char c = *p;

		obuf[j++] = c;
		if (j >= olen)
			break;

		if (c == IAC)
			obuf[j++] = c;
		else if (c == '\r')
			obuf[j++] ='\0';
	}

	if (len == 1 && (*p != '\r' && *p != IAC))
		return j;

	if (len > 0)
		return -1;

	return j;
}

void tn_init(struct tn_contex *ctx,
             void (*send_cb)(void *data, void *buf, int len),
             void *data)
{
	memset(ctx, 0, sizeof(*ctx));
	ctx->data = data;
	ctx->send = send_cb;
	memcpy(ctx->opt, def_opts, sizeof(def_opts));
	ctx->send(data, init_seq, sizeof(init_seq));
}
