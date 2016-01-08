#define LON_IMPLEMENTATION
#include "lon.h"

static void on_error(void *ud, const char *errmsg) {
    printf("%s\n", errmsg);
}

static size_t writer(void *ud, const char *s, size_t len) {
    printf("%.*s", len, s);
    return len;
}

#define LOAD(str) lon_load_buffer(&L, "" str, sizeof(str)-1)

int main(void) {
    lon_Loader L;
    lon_Dumper D;
    lon_initloader(&L);
    lon_initdumper(&D);
    lon_setdumper(&L, &D);
    lon_setpanicf(&L, on_error, NULL);
    lon_setwriter(&D, writer, NULL);
    lon_setdumpopt(&D, LON_OPT_INDENT, 4);

    /* test */
    LOAD("return 1,2,3,4,5 -- comment");
    LOAD("return {1,2,3,['foo']='bar',4,5,{6,7,8},['zzz']={a='a',b='b',c='c'}}");
    LOAD("return\v\f'a\0a' \v\f\f");
    LOAD("'\\\n\\\"\\'\\\\'");
    LOAD("'\a\b\f\\n\\r\t\v'");
    LOAD("'\\09912'");
    LOAD("'\\099\\10'");
    LOAD("'\\0\\0\\0alo'");
    LOAD("\"\\x00\\x05\\x10\\x1f\\x3C\\xfF\\xe8\"");
    LOAD("\"abc\\z\n      def\\z\n       ghi\\z\n           \"");
    LOAD("\"\\u{0}\\u{5}\\u{10}\\u{1f}\\u{3C}\\u{fF}\\u{e8}\"");
    LOAD("\"\\u{0}\\u{00000000000}\\x00\\0\"");
    LOAD("\"\\u{0}\\u{7f}\"");
    LOAD("\"\\u{80}\\u{7FF}\"");
    LOAD("\"\\u{800}\\u{FFFF}\"");
    LOAD("\"\\u{10000}\\u{10FFFF}\"");
    LOAD("--abc");
    LOAD("--[[abc]]");
    LOAD("--[[abc]]1");

    return 0;
}

