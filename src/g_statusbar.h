// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

// easy statusbar wrapper
typedef struct {
    size_t size;
    char buffer[1400];
} statusbar_t;

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

static void sb_printf(statusbar_t *sb, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    sb->size += Q_vscnprintf(sb->buffer + sb->size, sizeof(sb->buffer) - sb->size, fmt, ap);
    va_end(ap);
}

static void sb_stat_str(statusbar_t *sb, const char *cmd, const char *str)
{
    if (sb_need_quotes(str))
        sb_printf(sb, "%s \"%s\" ", cmd, str);
    else
        sb_printf(sb, "%s %s ", cmd, str);
}

#define sb_yb(ofs) sb_printf(sb_ptr, "yb %d ", ofs);
#define sb_yt(ofs) sb_printf(sb_ptr, "yt %d ", ofs);
#define sb_yv(ofs) sb_printf(sb_ptr, "yv %d ", ofs);

#define sb_xl(ofs) sb_printf(sb_ptr, "xl %d ", ofs);
#define sb_xr(ofs) sb_printf(sb_ptr, "xr %d ", ofs);
#define sb_xv(ofs) sb_printf(sb_ptr, "xv %d ", ofs);

#define sb_if(stat) sb_printf(sb_ptr, "if %d ", stat);
#define sb_endif() sb_printf(sb_ptr, "endif ");

#define sb_pic(stat) sb_printf(sb_ptr, "pic %d ", stat);
#define sb_picn(name) sb_printf(sb_ptr, "picn %s ", name);

#define sb_anum() sb_printf(sb_ptr, "anum ");
#define sb_rnum() sb_printf(sb_ptr, "rnum ");
#define sb_hnum() sb_printf(sb_ptr, "hnum ");

#define sb_num(width, stat) sb_printf(sb_ptr, "num %d %d ", width, stat);

#define sb_stat_string(stat) sb_printf(sb_ptr, "stat_string %d ", stat);
#define sb_stat_rstring(stat) sb_printf(sb_ptr, "stat_rstring %d ", stat);
#define sb_stat_cstring(stat) sb_printf(sb_ptr, "stat_cstring %d ", stat);

#define sb_stat_string2(stat) sb_printf(sb_ptr, "stat_string2 %d ", stat);
#define sb_stat_rstring2(stat) sb_printf(sb_ptr, "stat_rstring2 %d ", stat);
#define sb_stat_cstring2(stat) sb_printf(sb_ptr, "stat_cstring2 %d ", stat);

#define sb_string(str) sb_stat_str(sb_ptr, "string", str);
#define sb_rstring(str) sb_stat_str(sb_ptr, "rstring", str);
#define sb_cstring(str) sb_stat_str(sb_ptr, "cstring", str);

#define sb_string2(str) sb_stat_str(sb_ptr, "string2", str);
#define sb_rstring2(str) sb_stat_str(sb_ptr, "rstring2", str);
#define sb_cstring2(str) sb_stat_str(sb_ptr, "cstring2", str);

#define sb_health_bars(stat, name) sb_printf(sb_ptr, "health_bars %d %d ", stat, name);
