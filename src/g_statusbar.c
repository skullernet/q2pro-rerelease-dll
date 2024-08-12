// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#include "q_shared.h"
#include "g_statusbar.h"

static struct {
    size_t size;
    char buffer[1400];
} sb;

static bool sb_need_quotes(const char *s)
{
    if (!*s)
        return true;

    if (*s == '\"')
        return false;

    do {
        int c = *s++;
        if (!Q_isgraph(c))
            return true;
    } while (*s);

    return false;
}

void sb_begin(void)
{
    sb.size = 0;
    sb.buffer[0] = 0;
}

void sb_puts(const char *str)
{
    size_t siz = sizeof(sb.buffer) - sb.size;
    if (siz) {
        size_t len = min(strlen(str), siz - 1);
        memcpy(sb.buffer + sb.size, str, len);
        sb.size += len;
        sb.buffer[sb.size] = 0;
    }
}

void sb_printf(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    sb.size += Q_vscnprintf(sb.buffer + sb.size, sizeof(sb.buffer) - sb.size, fmt, ap);
    va_end(ap);
}

void sb_stat_str(const char *cmd, const char *str)
{
    if (sb_need_quotes(str))
        sb_printf("%s \"%s\" ", cmd, str);
    else
        sb_printf("%s %s ", cmd, str);
}

const char *sb_buffer(void)
{
    return sb.buffer;
}
