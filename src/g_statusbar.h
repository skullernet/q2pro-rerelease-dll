// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#pragma once

// easy statusbar wrapper
void sb_begin(void);
void sb_puts(const char *str);
void sb_printf(const char *fmt, ...) q_printf(1, 2);
void sb_stat_str(const char *cmd, const char *str);
const char *sb_buffer(void);

#define sb_yb(ofs) sb_printf("yb %d ", ofs)
#define sb_yt(ofs) sb_printf("yt %d ", ofs)
#define sb_yv(ofs) sb_printf("yv %d ", ofs)

#define sb_xl(ofs) sb_printf("xl %d ", ofs)
#define sb_xr(ofs) sb_printf("xr %d ", ofs)
#define sb_xv(ofs) sb_printf("xv %d ", ofs)

#define sb_if(stat) sb_printf("if %d ", stat)
#define sb_endif() sb_puts("endif ")

#define sb_pic(stat) sb_printf("pic %d ", stat)
#define sb_picn(name) sb_printf("picn %s ", name)

#define sb_anum() sb_puts("anum ")
#define sb_rnum() sb_puts("rnum ")
#define sb_hnum() sb_puts("hnum ")

#define sb_num(width, stat) sb_printf("num %d %d ", width, stat)

#define sb_stat_string(stat) sb_printf("stat_string %d ", stat)
#define sb_stat_rstring(stat) sb_printf("stat_rstring %d ", stat)
#define sb_stat_cstring(stat) sb_printf("stat_cstring %d ", stat)

#define sb_stat_string2(stat) sb_printf("stat_string2 %d ", stat)
#define sb_stat_rstring2(stat) sb_printf("stat_rstring2 %d ", stat)
#define sb_stat_cstring2(stat) sb_printf("stat_cstring2 %d ", stat)

#define sb_string(str) sb_stat_str("string", str)
#define sb_rstring(str) sb_stat_str("rstring", str)
#define sb_cstring(str) sb_stat_str("cstring", str)

#define sb_string2(str) sb_stat_str("string2", str)
#define sb_rstring2(str) sb_stat_str("rstring2", str)
#define sb_cstring2(str) sb_stat_str("cstring2", str)

#define sb_health_bars(stat, name) sb_printf("health_bars %d %d ", stat, name)
