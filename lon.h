/* lon: the Lua Object Notation data format endec
 * author: starwing
 * copyright: MIT licence (c) 2016 */
#ifndef lon_h
#define lon_h


#ifndef LON_NS_BEGIN
# ifdef __cplusplus
#   define LON_NS_BEGIN extern "C" {
#   define LON_NS_END   }
# else
#   define LON_NS_BEGIN
#   define LON_NS_END
# endif
#endif /* LON_NS_BEGIN */

#ifdef LON_STATIC_API
# ifndef LON_IMPLEMENTATION
#  define LON_IMPLEMENTATION
# endif
# if __GNUC__
#   define LON_API static __attribute((unused))
# else
#   define LON_API static
# endif
#endif

#ifndef LON_API
# define LON_API extern
#endif

#define LON_BUFFERSIZE 1024
#define LON_MAX_LEVEL  256

#define LON_OK        (0)
#define LON_ERR      (-1)
#define LON_ERRMEM   (-2)
#define LON_ERRFILE  (-3)

#if !defined(lon_getlocaledecpoint)
# ifdef __ANDROID__
#   define lon_getlocaledecpoint() '.'
# else
#   define lon_getlocaledecpoint() (localeconv()->decimal_point[0])
# endif /* __ANDROID__ */
#endif

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
# define _CRT_SECURE_NO_WARNINGS
#endif

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>


LON_NS_BEGIN

typedef struct lon_Buffer lon_Buffer;
typedef struct lon_Loader lon_Loader;
typedef struct lon_Dumper lon_Dumper;
typedef struct lon_Callbacks lon_Callbacks;
typedef struct lon_LoaderDumper lon_LoaderDumper;

#if LON_USE_LONGLONG
typedef long long lon_Integer;
#else
typedef ptrdiff_t lon_Integer;
#endif

#if LON_USE_DOUBLE
typedef double lon_Number;
#else
typedef float lon_Number;
#endif

typedef const char *lon_Reader (void *ud, size_t *plen);
typedef size_t      lon_Writer (void *ud, const char *buff, size_t len);
typedef void        lon_Panic  (void *ud, const char *errmsg);


/* lon buffer */

#define lon_buffer(B)         ((B)->buff)
#define lon_buffsize(B)       ((B)->size)
#define lon_resetbuffer(B)    ((B)->size = 0)
#define lon_truncbuffer(B, i) ((B)->size = \
        ((size_t)(i) > (B)->size ? 0 : (B)->size - (i)))
#define lon_addstring(B, s)   lon_addlstring((B),(s),strlen(s))

LON_API void lon_initbuffer (lon_Buffer *B, jmp_buf *jbuf);
LON_API void lon_freebuffer (lon_Buffer *B);

LON_API char *lon_prepbuffsize(lon_Buffer *B, size_t len);

LON_API int lon_addchar     (lon_Buffer *B, int ch);
LON_API int lon_addlstring  (lon_Buffer *B, const char *s, size_t len);
LON_API int lon_addvfstring (lon_Buffer *B, const char *fmt, va_list l);
LON_API int lon_addfstring  (lon_Buffer *B, const char *fmt, ...);


/* lon parser */

LON_API void lon_initloader (lon_Loader *L);

LON_API void lon_setcallbacks (lon_Loader *L, lon_Callbacks *cb);
LON_API void lon_setpanicf    (lon_Loader *L, lon_Panic *p, void *ud);
LON_API void lon_setdumper    (lon_Loader *L, lon_Dumper *ld);

LON_API int  lon_load   (lon_Loader *L, lon_Reader *reader, void *ud);
LON_API void lon_break  (lon_Loader *L, int res);
LON_API int  lon_status (lon_Loader *L);
LON_API int  lon_levels (lon_Loader *L);

LON_API int lon_load_buffer (lon_Loader *L, const char *s, size_t len);
LON_API int lon_load_string (lon_Loader *L, const char *s);
LON_API int lon_load_file   (lon_Loader *L, const char *filename);


/* lon dumper */

#define LON_OPT_COMPAT     1  /* default: 0(false) */
#define LON_OPT_INDENT     2  /* default: 3 */
#define LON_OPT_NEWLINE    3  /* default: 1(true) */
#define LON_OPT_RETURN     4  /* default: 1(true) */
#define LON_OPT_INTHEXA    5  /* default: 0(false) */
#define LON_OPT_FLTHEXA    6  /* default: 0(false) */
#define LON_OPT_FLTPREC    7  /* default: 0(default precision) */
#define LON_OPT_QUOTE     8  /* default: 0("", 0="", 1='', 2=[[]]) */

LON_API void lon_initdumper (lon_Dumper *D);
LON_API void lon_setwriter  (lon_Dumper *D, lon_Writer *writer, void *ud);
LON_API void lon_setbuffer  (lon_Dumper *D, lon_Buffer *buffer);

LON_API int lon_setdumpopt (lon_Dumper *D, int opt, int value);

LON_API void lon_dump_begin (lon_Dumper *D);
LON_API void lon_dump_flush (lon_Dumper *D);
LON_API void lon_dump_end   (lon_Dumper *D);

LON_API int lon_dump_nil     (lon_Dumper *D);
LON_API int lon_dump_boolean (lon_Dumper *D, int v);
LON_API int lon_dump_integer (lon_Dumper *D, lon_Integer v);
LON_API int lon_dump_number  (lon_Dumper *D, lon_Number v);
LON_API int lon_dump_string  (lon_Dumper *D, const char *s);
LON_API int lon_dump_buffer  (lon_Dumper *D, const char *s, size_t len);

LON_API int lon_dump_table_begin (lon_Dumper *D);
LON_API int lon_dump_table_end   (lon_Dumper *D);

#ifdef LON_LUA_API

typedef struct lua_State lua_State;

LON_API void lon_setluastate (lon_Loader *L, lua_State *LS);
LON_API void lon_dump_value  (lon_Dumper *D, lua_State *L, int idx);

#endif /* LON_LUA_API */


/* structs */

struct lon_Buffer {
    size_t size, capacity;
    jmp_buf *jbuf;
    char *buff;
    char init_buffer[LON_BUFFERSIZE];
};

struct lon_Callbacks {
    lon_Loader *loader;

    void (*on_error) (lon_Callbacks *cb, const char *errmsg);

    void (*on_begin) (lon_Callbacks *cb);
    void (*on_end)   (lon_Callbacks *cb);

    void (*on_nil)     (lon_Callbacks *cb);
    void (*on_boolean) (lon_Callbacks *cb, int value);
    void (*on_integer) (lon_Callbacks *cb, lon_Integer value);
    void (*on_number)  (lon_Callbacks *cb, lon_Number value);
    void (*on_string)  (lon_Callbacks *cb, const char *s, size_t len);

    void (*on_table_begin) (lon_Callbacks *cb);
    void (*on_table_end)   (lon_Callbacks *cb);
};

struct lon_Loader {
    jmp_buf jbuf;
    lon_Callbacks *cb;
    lon_Panic *panicf;
    lon_Reader *reader;
    void *ud, *panic_ud;

    /* special load callbacks */
    lon_Dumper *dumper;
    void *lua_state;

    int levels;       /* table levels */
    int status;       /* callback status */
#define LON_STATUS_TOP    0
#define LON_STATUS_KEY    1
#define LON_STATUS_VALUE  2

    size_t n;         /* bytes still unread */
    const char *p;    /* bytes still unread */

    const char *name; /* name of readed chunk */
    int line;         /* current line number */
    int current;      /* current char */
    int seplen;       /* length of string's delimiter */
    int token;        /* current token */
    int decpoint;     /* decimal point */

    lon_Integer iv;
    lon_Number nv;
    lon_Buffer buffer; /* token data */
    lon_Buffer errmsg; /* error message */
};

struct lon_Dumper {
    lon_Buffer *outbuffer;
    lon_Writer *writer;
    void *ud;

    unsigned opt_indent     : 4;
    unsigned opt_compat     : 1;
    unsigned opt_no_newline : 1;
    unsigned opt_no_return  : 1;
    unsigned opt_int_hexa   : 1;
    unsigned opt_flt_hexa   : 1;
    unsigned opt_flt_prec   : 4;
    unsigned opt_str_quote    : 4;

    size_t levels;
    struct {
        unsigned iskey : 1;
        unsigned subsq : 1;
        unsigned index : 30;
    } stack[LON_MAX_LEVEL];
    size_t buff_size;
    char buffer[LON_BUFFERSIZE];
};


LON_NS_END

#endif /* lon_h */


#if defined(LON_IMPLEMENTATION) && !defined(lon_implemented)
#define lon_implemented


#include <assert.h>
#include <limits.h>
#include <locale.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


LON_NS_BEGIN


/* Lua interface */

#ifdef LON_LUA_API

#include <lua.h>
#include <lauxlib.h>

typedef struct lonL_LuaState {
    lua_State *L;
    int size;
    int capacity;
} lonL_LuaState;

LON_API void lon_setluastate(lon_Loader *L, lua_State *LS)
{ L->lua_state = (void*)LS; }

static void lonL_growstack(lonL_LuaState *ls, size_t n) {
    if (ls->size + n > ls->capacity) {
        size_t newsize = ls->capacity * 2;
        luaL_checkstack(ls->L, newsize, "too many values");
        ls->capacity = newsize;
    }
}

static void lonL_updatelua(lon_Callbacks *cb) {
    lonL_LuaState *ls = (lonL_LuaState*)cb->loader->lua_state;
    switch (lon_status(cb->loader)) {
    case LON_STATUS_TOP:
        lonL_growstack(ls, 1);
        ++ls->size;
        break;
    case LON_STATUS_VALUE:
        lua_rawset(ls->L, -3);
        break;
    }
}

static void lonL_lua_onerror(lon_Callbacks *cb, const char *errmsg) {
    lonL_LuaState *ls = (lonL_LuaState*)cb->loader->lua_state;
    luaL_error(ls->L, "%s", errmsg);
}

static void lonL_lua_onnil(lon_Callbacks *cb) {
    lonL_LuaState *ls = (lonL_LuaState*)cb->loader->lua_state;
    lua_pushnil(ls->L);
    lonL_updatelua(cb);
}

static void lonL_lua_onboolean(lon_Callbacks *cb, int value) {
    lonL_LuaState *ls = (lonL_LuaState*)cb->loader->lua_state;
    lua_pushboolean(ls->L, value);
    lonL_updatelua(cb);
}

static void lonL_lua_oninteger(lon_Callbacks *cb, lon_Integer value) {
    lonL_LuaState *ls = (lonL_LuaState*)cb->loader->lua_state;
    lua_pushinteger(ls->L, value);
    lonL_updatelua(cb);
}

static void lonL_lua_onnumber(lon_Callbacks *cb, lon_Number value) {
    lonL_LuaState *ls = (lonL_LuaState*)cb->loader->lua_state;
    lua_pushnumber(ls->L, value);
    lonL_updatelua(cb);
}

static void lonL_lua_onstring(lon_Callbacks *cb, const char *s, size_t len) {
    lonL_LuaState *ls = (lonL_LuaState*)cb->loader->lua_state;
    lua_pushlstring(ls->L, s, len);
    lonL_updatelua(cb);
}

static void lonL_lua_ontablebegin(lon_Callbacks *cb) {
    lonL_LuaState *ls = (lonL_LuaState*)cb->loader->lua_state;
    lonL_growstack(ls, 3);
    lua_newtable(ls->L);
}

static void lonL_lua_ontableend(lon_Callbacks *cb) {
    lonL_updatelua(cb);
}

static void lonL_initluacb(lon_Loader *L, lon_Callbacks *cb) {
    cb->on_error       = lonL_lua_onerror;
    cb->on_nil         = lonL_lua_onnil;
    cb->on_boolean     = lonL_lua_onboolean;
    cb->on_integer     = lonL_lua_oninteger;
    cb->on_number      = lonL_lua_onnumber;
    cb->on_string      = lonL_lua_onstring;
    cb->on_table_begin = lonL_lua_ontablebegin;
    cb->on_table_end   = lonL_lua_ontableend;
    L->cb = cb;
}

static void lonD_pushvalue(lon_Dumper *D, lua_State *L, int idx, int t);

static int lonD_relindex(int idx, int onstack) {
    return idx >= 0 || idx <= LUA_REGISTRYINDEX ?
        idx : idx - onstack;
}

static void lonD_pushtable(lon_Dumper *D, lua_State *L, int idx, int t) {
    luaL_checkstack(L, 6, "too many tables");
    lua_pushnil(L);
    idx = lonD_relindex(idx, 1);
    lon_dump_table_begin(D);
    while (lua_next(L, idx)) {
        const void *pk = lua_topointer(L, -2);
        const void *pv = lua_topointer(L, -1);
        if ((pk != NULL && lua_rawgetp(L, t, pk) != LUA_TNIL)
                || (pv != NULL && lua_rawgetp(L, t, pv) != LUA_TNIL))
            luaL_argerror(L, idx, "attempt to dump a recursion table");
        lonD_pushvalue(D, L, -4, t);
        lonD_pushvalue(D, L, -3, t);
        lua_pop(L, 3);
    }
    lon_dump_table_end(D);
}

static void lonD_pushvalue(lon_Dumper *D, lua_State *L, int idx, int t) {
    switch (lua_type(L, idx)) {
    case LUA_TNIL:
        lon_dump_nil(D);
        break;
    case LUA_TBOOLEAN:
        lon_dump_boolean(D, lua_toboolean(L, idx));
        break;
    case LUA_TNUMBER:
        if (lua_isinteger(L, idx))
            lon_dump_integer(D, lua_tointeger(L, idx));
        else
            lon_dump_number(D, lua_tonumber(L, idx));
        break;
    case LUA_TSTRING:
        {
            size_t len;
            const char *s = lua_tolstring(L, idx, &len);
            lon_dump_buffer(D, s, len);
        }
        break;
    case LUA_TTABLE:
        if (t != 0)
            lonD_pushtable(D, L, idx, t);
        else {
            lua_createtable(L, 0, 2);
            lua_pushboolean(L, 1);
            lua_rawsetp(L, -2, lua_topointer(L, lonD_relindex(idx, 2)));
            t = lua_gettop(L);
            lonD_pushtable(D, L, lonD_relindex(idx, 1), t);
            assert(lua_gettop(L) == t);
            lua_pop(L, 1);
        }
        break;
    default:
        lua_pushfstring(L,
                "attempt to dump a %s value",
                luaL_typename(L, idx));
        luaL_argerror(L, idx, lua_tostring(L, -1));
    }
}

LON_API void lon_dump_value(lon_Dumper *D, lua_State *L, int idx)
{ lonD_pushvalue(D, L, idx, 0); }

#endif /* LON_LUA_API */


/* utils */

#define LON_XNUM_MAXSIGDIG  30
#define LON_UTF8_BUFFERSIZE 8

#define lon_isnewline(ch)      ((ch) == '\n' || (ch) == '\r')
#define lon_mask(F)            (1 << LON_##F)
#define lon_checkmask(ch,mask) ((lon_charmap[((ch)&0xFF)+1] & (mask)) != 0)

#define lon_isalpha(ch)  lon_checkmask(ch, lon_mask(ALPHA))
#define lon_isdigit(ch)  lon_checkmask(ch, lon_mask(DIGIT))
#define lon_isprint(ch)  lon_checkmask(ch, lon_mask(PRINT))
#define lon_isspace(ch)  lon_checkmask(ch, lon_mask(SPACE))
#define lon_isxdigit(ch) lon_checkmask(ch, lon_mask(XDIGIT))
#define lon_isalnum(ch)  lon_checkmask(ch, lon_mask(ALPHA)|lon_mask(DIGIT))

enum lon_CType { LON_ALPHA, LON_DIGIT, LON_PRINT, LON_SPACE, LON_XDIGIT };

static const unsigned char lon_charmap[UCHAR_MAX + 2] = {
    0x00, /* EOZ */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0. */
    0x00, 0x08, 0x08, 0x08, 0x08, 0x08, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 1. */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x0c, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, /* 2. */
    0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
    0x16, 0x16, 0x16, 0x16, 0x16, 0x16, 0x16, 0x16, /* 3. */
    0x16, 0x16, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
    0x04, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x05, /* 4. */
    0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
    0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, /* 5. */
    0x05, 0x05, 0x05, 0x04, 0x04, 0x04, 0x04, 0x05,
    0x04, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x05, /* 6. */
    0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
    0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, /* 7. */
    0x05, 0x05, 0x05, 0x04, 0x04, 0x04, 0x04, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 8. */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 9. */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* a. */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* B. */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* c. */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* d. */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* e. */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* f. */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static int lon_hexavalue(int ch) {
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    return 0;
}

int lon_encode_utf8(char *buff, unsigned long x) {
    int n = 1;  /* number of bytes put in buffer (backwards) */
    assert(x <= 0x10FFFF);
    if (x < 0x80)  /* ascii? */
        buff[LON_UTF8_BUFFERSIZE - 1] = (char)x;
    else {  /* need continuation bytes */
        unsigned int mfb = 0x3f;  /* maximum that fits in first byte */
        do {
            buff[LON_UTF8_BUFFERSIZE - (n++)] = (char)(0x80 | (x & 0x3f));
            x >>= 6;
            mfb >>= 1;
        } while (x > mfb);
        buff[LON_UTF8_BUFFERSIZE - n] = (char)((~mfb << 1) | x);
    }
    return n;
}

static int lon_checkneg(const char **s) {
    if (**s == '-') { ++(*s); return 1; }
    else if (**s == '+') ++(*s);
    return 0;
}

static lon_Integer lon_str2integer(const char *s, char **endptr) {
    unsigned long long a = 0;
    int empty = 1;
    int neg = lon_checkneg(&s);
    *endptr = (char*)s;
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {  /* hex? */
        s += 2;  /* skip '0x' */
        for (; lon_isxdigit(*s & 0xFF); ++s) {
            a = a * 16 + lon_hexavalue(*s);
            empty = 0;
        }
    }
    else {  /* decimal */
        for (; lon_isdigit(*s & 0xFF); ++s) {
            a = a * 10 + *s - '0';
            empty = 0;
        }
    }
    if (empty || *s != '\0') /* something wrong in the numeral */
        return 0;
    *endptr = (char*)s;
    return (lon_Integer)(neg ? 0ull - a : a);
}

static lon_Number lon_strx2number(const char *s, char **endptr) {
    lon_Number r = 0.0;
    int sigdig = 0;  /* number of significant digits */
    int nosigdig = 0;  /* number of non-significant digits */
    int e = 0;  /* exponent correction */
    int neg;  /* 1 if number is negative */
    int hasdot = 0;  /* true after seen a dot */
    *endptr = (char*)s;  /* nothing is valid yet */
    while (lon_isspace(*s)) s++;  /* skip initial spaces */
    neg = lon_checkneg(&s);
    if (!(*s == '0' && (*(s + 1) == 'x' || *(s + 1) == 'X')))
        return 0.0;
    for (s += 2; ; s++) {
        if (*s == '.') {
            if (hasdot) break;
            else hasdot = 1;
        }
        else if (lon_isxdigit(*s)) {
            if (sigdig == 0 && *s == '0')
                nosigdig++;
            else if (++sigdig <= LON_XNUM_MAXSIGDIG)
                r = (r * (lon_Number)16.0) + lon_hexavalue(*s);
            else e++;
            if (hasdot) e--;
        }
        else break;
    }
    if (nosigdig + sigdig == 0)
        return 0.0;
    *endptr = (char*)s;
    e *= 4;  /* each digit multiplies/divides value by 2^4 */
    if (*s == 'p' || *s == 'P') {
        int exp1 = 0;  /* exponent value */
        int neg1;  /* exponent signal */
        s++;  /* skip 'p' */
        neg1 = lon_checkneg(&s);  /* signal */
        if (!lon_isdigit(*s))
            return 0.0;  /* invalid; must have at least one digit */
        while (lon_isdigit(*s))  /* read exponent */
            exp1 = exp1 * 10 + *(s++) - '0';
        if (neg1) exp1 = -exp1;
        e += exp1;
        *endptr = (char*)s;  /* valid up to here */
    }
    if (neg) r = -r;
    return (lon_Number)ldexp(r, e);
}

static int lon_isidentifier(const char *s, size_t len) {
    size_t i;
    if (!lon_isalpha(*s++)) return 0;
    for (i = 1; i < len; ++i)
        if (!lon_isalnum(*s++)) return 0;
    return 1;
}


/* lon buffer */

LON_API void lon_initbuffer(lon_Buffer *B, jmp_buf *jbuf) {
    B->jbuf = jbuf;
    B->size = 0;
    B->capacity = LON_BUFFERSIZE;
    B->buff = B->init_buffer;
}

LON_API void lon_freebuffer(lon_Buffer *B) {
    if (B->buff != B->init_buffer)
        free(B->buff);
    lon_initbuffer(B, NULL);
}

LON_API char *lon_prepbuffsize(lon_Buffer *B, size_t len) {
    if (B->size + len > B->capacity) {
        void *newptr;
        size_t newsize = LON_BUFFERSIZE;
        while (newsize < B->size + len && newsize < ~(size_t)0/2)
            newsize *= 2;
        if (B->buff != B->init_buffer) {
            newptr = realloc(B->buff, newsize);
            if (newptr == NULL) goto nomem;
        }
        else {
            newptr = malloc(newsize);
            if (newptr == NULL) goto nomem;
            memcpy(newptr, B->buff, B->size);
        }
        B->buff = newptr;
        B->capacity = newsize;
    }
    return &B->buff[B->size];
nomem:
    if (B->jbuf) longjmp(*B->jbuf, LON_ERRMEM);
    return NULL;
}

LON_API int lon_addchar(lon_Buffer *B, int ch) {
    char *ptr = (char*)lon_prepbuffsize(B, 1);
    if (ptr == NULL) return 0;
    *ptr = ch;
    return ++B->size;
}

LON_API int lon_addlstring(lon_Buffer *B, const char *s, size_t len) {
    char *ptr = (char*)lon_prepbuffsize(B, 1);
    if (ptr == NULL) return 0;
    memcpy(ptr, s, len);
    return B->size += len;
}

LON_API int lon_addvfstring(lon_Buffer *B, const char *fmt, va_list l) {
    const size_t init_size = 80;
    void *ptr;
    size_t len;
    va_list l_count;
    if ((ptr = lon_prepbuffsize(B, init_size+1)) == NULL)
        return 0;
    va_copy(l_count, l);
    len = vsnprintf(ptr, init_size, fmt, l_count);
    va_end(l_count);
    if (len < 0) return 0;
    if (len > init_size) {
        if ((ptr = lon_prepbuffsize(B, len+1)) == NULL)
            return 0;
        vsnprintf(ptr, len, fmt, l);
    }
    return B->size += len;
}

LON_API int lon_addfstring(lon_Buffer *B, const char *fmt, ...) {
    int ret;
    va_list l;
    va_start(l, fmt);
    ret = lon_addvfstring(B, fmt, l);
    va_end(l);
    return ret;
}


/* lon lexer */

#define LON_EOZ (-1) /* end of buffer */
#define LON_FIRST_RESERVED  257

#define lonX_save(L,ch)    lon_addchar(&(L)->buffer,(ch))
#define lonX_endstring(L)  (*lon_prepbuffsize(&(L)->buffer, 1) = '\0')
#define lonX_save_next(L)  (lonX_save(L,(L)->current), lonX_next(L))

enum LON_RESERVED {
    /* terminal symbols denoted by reserved words */
    TK_AND = LON_FIRST_RESERVED, TK_BREAK,
    TK_DO, TK_ELSE, TK_ELSEIF, TK_END, TK_FALSE, TK_FOR, TK_FUNCTION,
    TK_GOTO, TK_IF, TK_IN, TK_LOCAL, TK_NIL, TK_NOT, TK_OR, TK_REPEAT,
    TK_RETURN, TK_THEN, TK_TRUE, TK_UNTIL, TK_WHILE,
    /* other terminal symbols */
    TK_IDIV, TK_CONCAT, TK_DOTS, TK_EQ, TK_GE, TK_LE, TK_NE,
    TK_SHL, TK_SHR,
    TK_DBCOLON, TK_EOS,
    TK_FLT, TK_INT, TK_NAME, TK_STRING
};

static const char *const lonX_tokens[] = {
    "and", "break", "do", "else", "elseif",
    "end", "false", "for", "function", "goto", "if",
    "in", "local", "nil", "not", "or", "repeat",
    "return", "then", "true", "until", "while",
    "//", "..", "...", "==", ">=", "<=", "~=",
    "<<", ">>", "::", "<eof>",
    "<number>", "<integer>", "<name>", "<string>"
};

static int lonX_next(lon_Loader *L) {
    size_t size;
    const char *buff;
    if (L->n-- == 0) {
        if (L->current == LON_EOZ)
            return LON_EOZ;
        buff = L->reader(L->ud, &size);
        if (buff == NULL || size == 0) {
            L->n = 0;
            L->p = NULL;
            return L->current = LON_EOZ;
        }
        L->n = size - 1;
        L->p = buff;
    }
    return L->current = *L->p++;
}

static int lonX_check_next2(lon_Loader *L, const char *set) {
    assert(set[2] == '\0');
    if (L->current == set[0] || L->current == set[1]) {
        lonX_save_next(L);
        return 1;
    }
    return 0;
}

static void lonX_addinfo(lon_Loader *L) {
    lon_addfstring(&L->errmsg, "%s:%d: ",
            (L->name ? L->name : "[=loader]"), L->line+1);
}

static void lonX_addtoken(lon_Loader *L, int token) {
    switch (token) {
    case TK_NAME: case TK_STRING:
    case TK_FLT: case TK_INT:
        lonX_save(L, '\0');
        lon_addfstring(&L->errmsg, "'%s'", lon_buffer(&L->buffer));
        break;
    default:
        if (token < LON_FIRST_RESERVED) {  /* single-byte symbols? */
            assert(token == (unsigned char)token);
            lon_addfstring(&L->errmsg, "'%c'", token);
        }
        else {
            const char *s = lonX_tokens[token - LON_FIRST_RESERVED];
            if (token < TK_EOS)
                lon_addfstring(&L->errmsg, "'%s'", s);
            else
                lon_addstring(&L->errmsg, s);
        }
    }
}

static void lonX_error(lon_Loader *L, const char *msg, int token) {
    if (msg) {
        lonX_addinfo(L);
        lon_addstring(&L->errmsg, msg);
    }
    if (token != 0) {
        lon_addstring(&L->errmsg, " near ");
        lonX_addtoken(L, token);
    }
    if (L->cb && L->cb->on_error)
        L->cb->on_error(L->cb, L->errmsg.buff);
    else if (L->panicf)
        L->panicf(L->panic_ud, L->errmsg.buff);
    longjmp(L->jbuf, LON_ERR);
}

static int lonX_checkkeyword(const char *s, size_t len) {
    switch (*s) {
#define KW(str, tok) \
        if (len == sizeof(str)-1 && memcmp(s+1, "" str+1, sizeof(str)-2) == 0) \
            return tok;
    case 'a': KW("and", TK_AND); break;
    case 'b': KW("break", TK_BREAK); break;
    case 'd': KW("do", TK_DO); break;
    case 'e': KW("else", TK_ELSE); KW("elseif", TK_ELSEIF);
              KW("end", TK_END); break;
    case 'f': KW("false", TK_FALSE); KW("for", TK_FOR);
              KW("function", TK_FUNCTION); break;
    case 'g': KW("goto", TK_GOTO); break;
    case 'i': KW("if", TK_IF); KW("in", TK_IN); break;
    case 'l': KW("local", TK_LOCAL); break;
    case 'n': KW("nil", TK_NIL); KW("not", TK_NOT); break;
    case 'o': KW("or", TK_OR); break;
    case 'r': KW("repeat", TK_REPEAT); KW("return", TK_RETURN); break;
    case 't': KW("then", TK_THEN); break;
    case 'u': KW("until", TK_UNTIL); break;
    case 'w': KW("while", TK_WHILE); break;
#undef KW
    }
    return TK_NAME;
}

static int lonX_checknumber(lon_Loader *L) {
    size_t i, size = lon_buffsize(&L->buffer);
    char *s = lon_buffer(&L->buffer), *endptr;
    L->iv = lon_str2integer(s, (char**)&endptr);
    if (endptr-s == size)
        return TK_INT;
    L->nv = lon_strx2number(s, (char**)&endptr);
    if (endptr-s == size)
        return TK_FLT;
    L->nv = (lon_Number)strtod(s, (char**)&endptr);
    if (endptr-s == size)
        return TK_FLT;
    if (L->decpoint == '\0')
        L->decpoint = lon_getlocaledecpoint();
    if (L->decpoint != '.') {
        for (i = 0; i < size; ++i)
            if (s[i] == '.')
                s[i] = L->decpoint;
        L->nv = (lon_Number)strtod(s, (char**)&endptr);
        if (endptr-s == size)
            return TK_FLT;
    }
    lonX_error(L, "malformed number", TK_FLT);
    return 0;
}

static void lonX_checkescape(lon_Loader *L, int check, const char *msg) {
    if (!check) {
        if (L->current != LON_EOZ)
            lonX_save_next(L);  /* add current to buffer for error message */
        lonX_error(L, msg, TK_STRING);
    }
}

static int lonX_checkhexa(lon_Loader *L) {
    lonX_save_next(L);
    lonX_checkescape(L, lon_isxdigit(L->current),
            "hexadecimal digit expected");
    return lon_hexavalue(L->current);
}

static int lonX_sep(lon_Loader *L) {
    int count = 0;
    int s = L->current;
    assert(s == '[' || s == ']');
    lonX_save_next(L);
    while (L->current == '=') {
        lonX_save_next(L);
        count++;
    }
    return (L->current == s) ? count : (-count) - 1;
}

static void lonX_hexaesc(lon_Loader *L) {
    int r;
    r = lonX_checkhexa(L);
    r = (r << 4) + lonX_checkhexa(L);
    lon_truncbuffer(&L->buffer, 3);
    lonX_save(L, r);
    lonX_next(L);
}

static void lonX_utf8esc(lon_Loader *L) {
    unsigned long r;
    int i = 4;  /* chars to be removed: '\', 'u', '{', and first digit */
    lonX_save_next(L);  /* skip 'u' */
    lonX_checkescape(L, L->current == '{', "missing '{'");
    r = lonX_checkhexa(L);  /* must have at least one digit */
    while ((lonX_save_next(L), lon_isxdigit(L->current))) {
        i++;
        r = (r << 4) + lon_hexavalue(L->current);
        lonX_checkescape(L, r <= 0x10FFFF, "UTF-8 value too large");
    }
    lonX_checkescape(L, L->current == '}', "missing '}'");
    lonX_next(L);  /* skip '}' */
    lon_truncbuffer(&L->buffer, i);
    {
        char buff[LON_UTF8_BUFFERSIZE];
        int n = lon_encode_utf8(buff, r);
        for (; n > 0; --n)
            lonX_save(L, buff[LON_UTF8_BUFFERSIZE - n]);
    }
}

static void lonX_decesc(lon_Loader *L) {
    int i, r = 0;  /* result accumulator */
    for (i = 0; i < 3 && lon_isdigit(L->current); ++i) {
        r = 10*r + L->current - '0';
        lonX_save_next(L);
    }
    lonX_checkescape(L, r <= UCHAR_MAX, "decimal escape too large");
    lon_truncbuffer(&L->buffer, i+1);  /* remove read digits from buffer */
    lonX_save(L, r);
}

static void lonX_newline(lon_Loader *L) {
    int old = L->current;
    assert(lon_isnewline(L->current));
    lonX_next(L);  /* skip '\n' or '\r' */
    if (lon_isnewline(L->current) && L->current != old)
        lonX_next(L);  /* skip '\n\r' or '\r\n' */
    if (++L->line >= INT_MAX)
        lonX_error(L, "chunk has too many lines", 0);
}

static void lonX_long_string(lon_Loader *L, int iscomment, int sep) {
    int line = L->line;  /* initial line (for error message) */
    lonX_save_next(L);  /* skip 2nd '[' */
    if (lon_isnewline(L->current))  /* string starts with a newline? */
        lonX_newline(L);  /* skip it */
    for (;;) {
        switch (L->current) {
        case LON_EOZ:
            lonX_addinfo(L);
            lon_addfstring(&L->errmsg,
                    "unfinished long %s (starting at line %d)",
                    (iscomment ? "string" : "comment"), line);
            lonX_error(L, NULL, TK_EOS);
            break;
        case ']':
            if (lonX_sep(L) == sep) {
                lonX_save_next(L);
                if (iscomment) lon_resetbuffer(&L->buffer);
                return;
            }
            break;
        case '\n': case '\r':
            if (!iscomment) lonX_save(L, '\n');
            lonX_newline(L);
            break;
        default: 
            if (!iscomment) lonX_save(L, L->current);
            lonX_next(L);
        }
    }
}

static void lonX_string(lon_Loader *L, int del) {
    int c;  /* final character to be saved */
    lonX_save_next(L);  /* keep delimiter (for error messages) */
    while (L->current != del) {
        switch (L->current) {
        case LON_EOZ:
            lonX_error(L, "unfinished string", TK_EOS);
            break;
        case '\n':
        case '\r':
            lonX_error(L, "unfinished string", TK_STRING);
            break;
        case '\\': /* escape sequences */
            lonX_save_next(L);  /* keep '\\' for error messages */
            switch (L->current) {
            case 'a': c = '\a'; goto read_save;
            case 'b': c = '\b'; goto read_save;
            case 'f': c = '\f'; goto read_save;
            case 'n': c = '\n'; goto read_save;
            case 'r': c = '\r'; goto read_save;
            case 't': c = '\t'; goto read_save;
            case 'v': c = '\v'; goto read_save;
            case 'x': lonX_hexaesc(L); goto no_save;
            case 'u': lonX_utf8esc(L); goto no_save;
            case '\n': case '\r':
                lonX_newline(L); c = '\n'; goto only_save;
            case '\\': case '\"': case '\'':
                c = L->current; goto read_save;
            case LON_EOZ: goto no_save;
            case 'z': /* zap following span of spaces */
                lon_truncbuffer(&L->buffer, 1);
                lonX_next(L);  /* skip the 'z' */
                while (lon_isspace(L->current)) {
                    if (lon_isnewline(L->current)) lonX_newline(L);
                    else lonX_next(L);
                }
                goto no_save;
            default:
                lonX_checkescape(L, lon_isdigit(L->current),
                        "invalid escape sequence");
                lonX_decesc(L);  /* digital escape '\ddd' */
                goto no_save;
            }
read_save:
            lonX_next(L);
            /* FALLTHROUGH */
only_save:
            lon_truncbuffer(&L->buffer, 1);
            lonX_save(L, c);
            /* FALLTHROUGH */
no_save:
            break;
        default:
            lonX_save_next(L);
        }
    }
    lonX_save_next(L);  /* skip delimiter */
}

static int lonX_numeral(lon_Loader *L) {
    const char *expo = "Ee";
    int first = L->current;
    assert(lon_isdigit(L->current));
    lonX_save_next(L);
    if (first == '0' && lonX_check_next2(L, "xX"))  /* hexadecimal? */
        expo = "Pp";
    for (;;) {
        if (lonX_check_next2(L, expo))  /* exponent part? */
            lonX_check_next2(L, "-+");  /* optional exponent sign */
        if (lon_isxdigit(L->current))
            lonX_save_next(L);
        else if (L->current == '.')
            lonX_save_next(L);
        else break;
    }
    lonX_endstring(L);
    return lonX_checknumber(L);
}

static int lon_lexer(lon_Loader *L) {
    lon_resetbuffer(&L->buffer);
    for (;;) {
        switch (L->current) {
        case '\n': case '\r': /* line breaks */
            lonX_newline(L);
            break;
        case ' ': case '\f': case '\t': case '\v':
            lonX_next(L);
            break;
        case '-':
            lonX_next(L);
            if (L->current != '-') return '-';
            lonX_next(L);
            if (L->current == '[') {
                int sep = lonX_sep(L);
                if (sep >= 0) {
                    lonX_long_string(L, 1, sep);
                    break;
                }
            }
            /* else short comment */
            while (!lon_isnewline(L->current) && L->current != LON_EOZ)
                lonX_next(L);  /* skip until end of line (or end of file) */
            break;
        case '[': /* long string or simply '[' */
            {
                int sep = lonX_sep(L);
                if (sep >= 0) {
                    lonX_long_string(L, 0, sep);
                    L->seplen = sep + 2;
                    return TK_STRING;
                }
                else if (sep != -1)  /* '[=...' missing second bracket */
                    lonX_error(L, "invalid long string delimiter", TK_STRING);
            }
            return '[';
        case '"': case '\'': /* short literal strings */
            lonX_string(L, L->current);
            L->seplen = 1;
            return TK_STRING;
        case '.': /* '.', '..', '...', or number */
            lonX_save_next(L);
            if (!lon_isdigit(L->current)) return '.';
            else return lonX_numeral(L);
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            return lonX_numeral(L);
        case LON_EOZ: 
            return TK_EOS;
        default:
            if (lon_isalpha(L->current)) {  /* identifier or reserved word? */
                do {
                    lonX_save_next(L);
                } while (lon_isalnum(L->current));
                lonX_endstring(L);
                return lonX_checkkeyword(lon_buffer(&L->buffer),
                        lon_buffsize(&L->buffer));
            }
            else {  /* single-char tokens (+ - / ...) */
                int c = L->current;
                lonX_next(L);
                return c;
            }
        }
    }
    return 0;
}


/* lon parser */

#define lonY_next(L) ((L)->token = lon_lexer(L))

static void lonY_expr(lon_Loader *L);

static void lonY_check(lon_Loader *L, int tok) {
    if (L->token != tok) {
        lonX_addinfo(L);
        lonX_addtoken(L, tok);
        lon_addstring(&L->errmsg, " expected");
        lonX_error(L, NULL, L->token);
    }
}

static int lonY_testnext(lon_Loader *L, int tok) {
  if (L->token == tok) {
    lonY_next(L);
    return 1;
  }
  else return 0;
}

static void lonY_checknext(lon_Loader *L, int tok) {
    lonY_check(L, tok);
    lonY_next(L);
}

static void lonY_checkmatch(lon_Loader *L, int what, int who, int where) {
    if (!lonY_testnext(L, what)) {
        if (where == L->line)
            lonY_check(L, what);
        else {
            lonX_addinfo(L);
            lonX_addtoken(L, what);
            lon_addstring(&L->errmsg, " expected (to close ");
            lonX_addtoken(L, who);
            lon_addfstring(&L->errmsg, " at line %d)", where);
            lonX_error(L, NULL, L->token);
        }
    }
}

static int lonY_field(lon_Loader *L, int index) {
    /* field: exp | (NAME | '[' exp ']') '=' exp */
    L->status = LON_STATUS_KEY;
    switch (L->token) {
        if (L->token == TK_NAME) {
    case TK_NAME:
            if (L->cb && L->cb->on_string)
                L->cb->on_string(L->cb, lon_buffer(&L->buffer),
                        lon_buffsize(&L->buffer));
            lonY_next(L);
        }
        else /*if (L->token == '[')*/ {
    case '[':
            lonY_next(L);
            lonY_expr(L);
            lonY_checknext(L, ']');
        }
        lonY_checknext(L, '=');
        L->status = LON_STATUS_VALUE;
        lonY_expr(L);
        return 0;
    default:
        if (L->cb && L->cb->on_integer)
            L->cb->on_integer(L->cb, index);
        L->status = LON_STATUS_VALUE;
        lonY_expr(L);
        return 1;
    }
}

static void lonY_table(lon_Loader *L) {
    /* table -> '{' [ field { sep field } [sep] ] '}'
       sep -> ',' | ';' */
    int index = 1;
    int line = L->line;
    int status = L->status;
    lonY_checknext(L, '{');
    ++L->levels;
    if (L->cb && L->cb->on_table_begin)
        L->cb->on_table_begin(L->cb);
    do {
        if (L->token == '}') break;
        index += lonY_field(L, index);
    } while (lonY_testnext(L, ',') || lonY_testnext(L, ';'));
    lonY_checkmatch(L, '}', '{', line);
    L->status = status;
    if (L->cb && L->cb->on_table_end)
        L->cb->on_table_end(L->cb);
    --L->levels;
}

static void lonY_expr(lon_Loader *L) {
    /* exp -> nil | boolean | integer | number | string | table */
    switch (L->token) {
    case TK_EOS:
        return;
    case TK_NIL:
        if (L->cb && L->cb->on_nil)
            L->cb->on_nil(L->cb);
        break;
    case TK_TRUE:
        if (L->cb && L->cb->on_boolean)
            L->cb->on_boolean(L->cb, 1);
        break;
    case TK_FALSE:
        if (L->cb && L->cb->on_boolean)
            L->cb->on_boolean(L->cb, 0);
        break;
    case TK_INT:
        if (L->cb && L->cb->on_integer)
            L->cb->on_integer(L->cb, L->iv);
        break;
    case TK_FLT:
        if (L->cb && L->cb->on_number)
            L->cb->on_number(L->cb, L->nv);
        break;
    case TK_STRING:
        if (L->cb && L->cb->on_string)
            L->cb->on_string(L->cb, lon_buffer(&L->buffer)+L->seplen,
                    lon_buffsize(&L->buffer)-L->seplen*2);
        break;
    case '{':
        lonY_table(L);
        return;
    default:
        lonX_error(L, "unexpected symbol", L->token);
    }
    lonY_next(L);
}

static void lonY_expr_list(lon_Loader *L) {
    /* expr_list -> expr { ',' expr } */
    L->status = LON_STATUS_TOP;
    lonY_expr(L);
    for (;;) {
        switch (L->token) {
        case TK_EOS:
            return;
        case ',':
            lonY_next(L);
            L->status = LON_STATUS_TOP;
            lonY_expr(L);
            break;
        default:
            lonX_error(L, "<eof> or ',' expected", L->token);
        }
    }
}

static void lon_parser(lon_Loader *L) {
    L->levels = 0;
    L->status = LON_STATUS_TOP;
    if (L->cb && L->cb->on_begin)
        L->cb->on_begin(L->cb);
    switch (lonY_next(L)) {
    case TK_EOS:
        return;
    case '{': lonY_table(L); break;
    case TK_RETURN:
        lonY_next(L);
        /* FALLTHROUGH */
    default:
        lonY_expr_list(L);
        break;
    }
    L->status = LON_STATUS_TOP;
    if (L->cb && L->cb->on_end)
        L->cb->on_end(L->cb);
}


/* loader routines */

typedef struct lon_StringCtx {
    size_t len, loaded;
    const char *s;
} lon_StringCtx;

typedef struct lon_FileCtx {
    size_t len, loaded;
    FILE *fp;
    char buff[LON_BUFFERSIZE];
} lon_FileCtx;

LON_API void lon_initloader(lon_Loader *L)
{ memset(L, 0, sizeof(*L)); }

LON_API void lon_setcallbacks(lon_Loader *L, lon_Callbacks *cb)
{ L->cb = cb; if (cb) cb->loader = L; }

LON_API void lon_setpanicf(lon_Loader *L, lon_Panic *f, void *ud)
{ L->panicf = f; L->panic_ud = ud; }

LON_API void lon_setdumper(lon_Loader *L, lon_Dumper *dumper)
{ L->dumper = dumper; }

LON_API int lon_status(lon_Loader *L)
{ return L->status; }

LON_API int lon_levels(lon_Loader *L)
{ return L->levels; }

static void lonL_on_begin(lon_Callbacks *cb)
{ lon_dump_begin(cb->loader->dumper); }
static void lonL_on_end(lon_Callbacks *cb)
{ lon_dump_end(cb->loader->dumper); }
static void lonL_on_nil(lon_Callbacks *cb)
{ lon_dump_nil(cb->loader->dumper); }
static void lonL_on_boolean(lon_Callbacks *cb, int value)
{ lon_dump_boolean(cb->loader->dumper, value); }
static void lonL_on_integer(lon_Callbacks *cb, lon_Integer value)
{ lon_dump_integer(cb->loader->dumper, value); }
static void lonL_on_number(lon_Callbacks *cb, lon_Number value)
{ lon_dump_number(cb->loader->dumper, value); }
static void lonL_on_string(lon_Callbacks *cb, const char *s, size_t len)
{ lon_dump_buffer(cb->loader->dumper, s, len); }
static void lonL_on_table_begin(lon_Callbacks *cb)
{ lon_dump_table_begin(cb->loader->dumper); }
static void lonL_on_table_end(lon_Callbacks *cb)
{ lon_dump_table_end(cb->loader->dumper); }

static void lonL_initdumpcb(lon_Loader *L, lon_Callbacks *cb) {
    if (L->cb == NULL && L->dumper != NULL) {
        cb->on_begin       = lonL_on_begin;
        cb->on_end         = lonL_on_end;
        cb->on_nil         = lonL_on_nil;
        cb->on_boolean     = lonL_on_boolean;
        cb->on_integer     = lonL_on_integer;
        cb->on_number      = lonL_on_number;
        cb->on_string      = lonL_on_string;
        cb->on_table_begin = lonL_on_table_begin;
        cb->on_table_end   = lonL_on_table_end;
        L->cb = cb;
    }
}

static const char *lonL_stringreader(void *ud, size_t *plen) {
    lon_StringCtx *ctx = (lon_StringCtx*)ud;
    if (ctx->loaded) return NULL;
    ctx->loaded += ctx->len;
    if (plen) *plen = ctx->len;
    return ctx->s;
}

static const char *lonL_filereader(void *ud, size_t *plen) {
    lon_FileCtx *ctx = (lon_FileCtx*)ud;
    size_t bytes = fread(ctx->buff, 1, LON_BUFFERSIZE, ctx->fp);
    ctx->loaded += bytes;
    if (bytes == 0) return NULL;
    if (plen) *plen = bytes;
    return ctx->buff;
}

static void lonL_outofmem(lon_Loader *L) {
    char buff[80];
    snprintf(buff, 80, "%s:%d: out of memory",
            (L->name ? L->name : "[=loader]"), L->line+1);
    if (L->cb && L->cb->on_error)
        L->cb->on_error(L->cb, buff);
    else if (L->panicf)
        L->panicf(L->panic_ud, buff);
}

LON_API int lon_load(lon_Loader *L, lon_Reader *reader, void *ud) {
    int res;
    lon_Callbacks cb = { NULL };
#ifdef LON_LUA_API
    lonL_LuaState ls;
    if (L->lua_state != NULL) {
        ls.L = (lua_State*)L->lua_state;
        ls.size = 0;
        ls.capacity = 16;
        luaL_checkstack(ls.L, 16, "too many tables");
        L->lua_state = (void*)&ls;
        lonL_initluacb(L, &cb);
    }
    else
#endif
    lonL_initdumpcb(L, &cb);
    if (L->cb) L->cb->loader = L;
    L->reader = reader;
    L->ud = ud;
    L->line = 0;
    L->current = 0, L->n = 0, L->p = NULL;
    lon_initbuffer(&L->errmsg, &L->jbuf);
    lon_initbuffer(&L->buffer, &L->jbuf);
    if ((res = setjmp(L->jbuf)) == 0) {
        lonX_next(L);
        lon_parser(L);
    }
    else if (res == LON_ERRMEM)
        lonL_outofmem(L);
    lon_break(L, LON_OK);
    return res;
}

LON_API void lon_break(lon_Loader *L, int res) {
#ifdef LON_LUA_API
    if (L->lua_state) {
        L->lua_state = ((lonL_LuaState*)L->lua_state)->L;
        L->cb = NULL;
    }
    else
#endif
    if (L->dumper) L->cb = NULL;
    lon_freebuffer(&L->buffer);
    lon_freebuffer(&L->errmsg);
    L->name = NULL;
    if (res != LON_OK) longjmp(L->jbuf, res);
}

LON_API int lon_load_string(lon_Loader *L, const char *s) {
    lon_StringCtx ctx = { strlen(s), 0, s };
    L->name = "[=string]";
    return lon_load(L, lonL_stringreader, &ctx);
}

LON_API int lon_load_buffer(lon_Loader *L, const char *s, size_t len) {
    lon_StringCtx ctx = { len, 0, s };
    L->name = "[=buffer]";
    return lon_load(L, lonL_stringreader, &ctx);
}

LON_API int lon_load_file(lon_Loader *L, const char *filename) {
    lon_FileCtx ctx = { 0 };
    ctx.fp = fopen(filename, "r");
    if (ctx.fp == NULL) return LON_ERRFILE;
    L->name = filename;
    return lon_load(L, lonL_filereader, &ctx);
}


/* lon dumper */

#define lonD_iskey(D) ((D)->stack[D->levels].iskey)
#define lonD_subsq(D) ((D)->stack[D->levels].subsq)
#define lonD_index(D) ((D)->stack[D->levels].index)

#define lonD_addstring(D,s) lonD_addlstring((D),(s),strlen(s))

LON_API void lon_initdumper(lon_Dumper *D)
{ memset(D, 0, sizeof(*D)); }

LON_API void lon_setwriter(lon_Dumper *D, lon_Writer *writer, void *ud)
{ D->writer = writer; D->ud = ud; }

LON_API int lon_dump_string(lon_Dumper *D, const char *s) 
{ return lon_dump_buffer(D, s, strlen(s)); }

static size_t lonD_buffwriter(void *ud, const char *s, size_t len) {
    lon_Dumper *D = (lon_Dumper*)ud;
    if (lon_addlstring(D->outbuffer, s, len))
        return len;
    return 0;
}

static void lonD_addchar(lon_Dumper *D, int ch) {
    if (D->buff_size >= LON_BUFFERSIZE)
        lon_dump_flush(D);
    D->buffer[D->buff_size++] = ch;
}

static void lonD_addlstring(lon_Dumper *D, const char *s, size_t len) {
    if (D->buff_size + len > LON_BUFFERSIZE)
        lon_dump_flush(D);
    if (len <= LON_BUFFERSIZE) {
        memcpy(&D->buffer[D->buff_size], s, len);
        D->buff_size += len;
    }
    else if (D->writer)
        D->writer(D->ud, s, len);
}

static void lonD_addfstring(lon_Dumper *D, const char *fmt, ...) {
    size_t len, remain = LON_BUFFERSIZE - D->buff_size;
    va_list l, l_try;
    va_start(l, fmt);
    va_copy(l_try, l);
    len = vsnprintf(D->buffer+D->buff_size, remain, fmt, l_try);
    va_end(l_try);
    if (len <= remain) goto out;
    lon_dump_flush(D);
    if (len <= LON_BUFFERSIZE)
        vsnprintf(D->buffer+D->buff_size, LON_BUFFERSIZE, fmt, l);
    else if (D->writer) {
        char *buff = (char*)malloc(len);
        if (buff == NULL) goto out;
        vsnprintf(buff, len, fmt, l);
        D->writer(D->ud, buff, len);
        free(buff);
        len = 0;
    }
out:
    D->buff_size += len;
    va_end(l);
}

static void lonD_addindent(lon_Dumper *D) {
    size_t len = D->opt_indent * D->levels;
    if (D->opt_compat || len == 0) return;
    if (D->opt_no_newline) {
        lonD_addchar(D, ' ');
        return;
    }
    if (D->buff_size + len > LON_BUFFERSIZE)
        lon_dump_flush(D);
    if (len <= LON_BUFFERSIZE) {
        memset(&D->buffer[D->buff_size], ' ', len);
        D->buff_size += len;
    }
    else if (D->writer) {
        char buff[LON_BUFFERSIZE];
        memset(buff, ' ', LON_BUFFERSIZE);
        while (len > LON_BUFFERSIZE) {
            D->writer(D->ud, buff, LON_BUFFERSIZE);
            len -= LON_BUFFERSIZE;
        }
        D->writer(D->ud, buff, len);
    }
}

static void lonD_begin(lon_Dumper *D) {
    /* 1. for top level object, add comma and newline */
    /* 2. for table key object, add comma, newline and indent */
    if (D->levels == 0 || lonD_iskey(D)) {
        int subsq = lonD_subsq(D);
        if (subsq) lonD_addchar(D, ',');
        if (D->levels == 0 && !subsq) return;
        if (D->levels == 0 || D->opt_no_newline)
            lonD_addchar(D, ' ');
        else if (!D->opt_compat) {
            lonD_addchar(D, '\n');
            lonD_addindent(D);
        }
    }
}

static void lonD_end(lon_Dumper *D) {
    if (!lonD_subsq(D)) lonD_subsq(D) = 1;
    if (D->levels == 0) return;
    if (!lonD_iskey(D)) lonD_iskey(D) = 1;
    else {
        if (!D->opt_compat) lonD_addchar(D, ' ');
        lonD_addchar(D, '=');
        if (!D->opt_compat) lonD_addchar(D, ' ');
        lonD_iskey(D) = 0;
    }
}

static void lonD_escapechar(lon_Dumper *D, int ch) {
    int esc = 0;
    switch (ch) {
    case '\a': esc = 'a'; break;
    case '\b': esc = 'b'; break;
    case '\f': esc = 'f'; break;
    case '\n': esc = 'n'; break;
    case '\r': esc = 'r'; break;
    case '\t': esc = 't'; break;
    case '\v': esc = 'v'; break;
    case '\\': esc = '\\'; break;
    default:
       if (!lon_isprint(ch))
           lonD_addfstring(D, "\\%d", (unsigned char)ch);
       else
           lonD_addchar(D, ch);
       return;
    }
    lonD_addfstring(D, "\\%c", esc);
}

static void lonD_addescape(lon_Dumper *D, const char *s, size_t len) {
    size_t i;
    if (D->opt_str_quote == 0) {
        lonD_addchar(D, '"');
        for (i = 0; i < len; ++i) {
            if (s[i] == '"') lonD_addstring(D, "\\\"");
            else lonD_escapechar(D, s[i]);
        }
        lonD_addchar(D, '"');
    }
    else if (D->opt_str_quote == 1) {
        lonD_addchar(D, '\'');
        for (i = 0; i < len; ++i) {
            if (s[i] == '\'') lonD_addstring(D, "\\'");
            else lonD_escapechar(D, s[i]);
        }
        lonD_addchar(D, '\'');
    }
    else {
        /* XXX */
    }
}

LON_API void lon_setbuffer(lon_Dumper *D, lon_Buffer *buffer) {
    D->outbuffer = buffer;
    D->writer = lonD_buffwriter;
    D->ud = (void*)D;
}

LON_API int lon_setdumpopt(lon_Dumper *D, int opt, int value) {
    int oldvalue;
    switch (opt) {
#define clamp(v,min,max) ((v) < min ? min : (v) >= max ? max-1 : (v))
    default:
        return 0;
    case LON_OPT_COMPAT:
        oldvalue = D->opt_compat;
        D->opt_compat = !!value;
        break;
    case LON_OPT_INDENT:
        oldvalue = D->opt_indent;
        D->opt_indent = clamp(value, 0, 16);
        break;
    case LON_OPT_NEWLINE:
        oldvalue = !D->opt_no_newline;
        D->opt_no_newline = !value;
        break;
    case LON_OPT_RETURN:
        oldvalue = !D->opt_no_return;
        D->opt_no_return = !value;
        break;
    case LON_OPT_INTHEXA:
        oldvalue = D->opt_int_hexa;
        D->opt_int_hexa = !!value;
        break;
    case LON_OPT_FLTHEXA:
        oldvalue = D->opt_flt_hexa;
        D->opt_flt_hexa = !!value;
        break;
    case LON_OPT_FLTPREC:
        oldvalue = D->opt_flt_prec;
        D->opt_flt_prec = clamp(value, 0, 16);
        break;
    case LON_OPT_QUOTE:
        oldvalue = D->opt_str_quote;
        D->opt_str_quote = clamp(value, 0, 16);
        break;
#undef clamp
    }
    return oldvalue;
}

LON_API void lon_dump_begin(lon_Dumper *D) {
    if (D->opt_indent == 0) D->opt_indent = 3;
    D->levels     = 0;
    D->buff_size  = 0;
    lonD_iskey(D) = 0;
    lonD_subsq(D) = 0;
    lonD_index(D) = 0;
    if (!D->opt_no_return)
        lonD_addstring(D, "return ");
}

LON_API void lon_dump_flush(lon_Dumper *D) {
    if (D->writer)
        D->writer(D->ud, D->buffer, D->buff_size);
    D->buff_size = 0;
}

LON_API void lon_dump_end(lon_Dumper *D) {
    do lonD_iskey(D) = 0;
    while (lon_dump_table_end(D));
    if (!D->opt_no_newline && !D->opt_compat)
        lonD_addchar(D, '\n');
    lon_dump_flush(D);
}

LON_API int lon_dump_table_begin(lon_Dumper *D) {
    if (D->levels >= LON_MAX_LEVEL-1)
        return 0;
    lonD_begin(D);
    if (lonD_iskey(D)) lonD_addchar(D, '[');
    lonD_addchar(D, '{');
    ++D->levels;
    lonD_iskey(D) = 1;
    lonD_subsq(D) = 0;
    lonD_index(D) = 0;
    return 1;
}

LON_API int lon_dump_table_end(lon_Dumper *D) {
    int subsq = lonD_subsq(D);
    if (D->levels == 0) return 0;
    --D->levels;
    if (D->opt_no_newline)
        lonD_addchar(D, ' ');
    else if (subsq && !D->opt_compat) {
        lonD_addchar(D, '\n');
        lonD_addindent(D);
    }
    lonD_addchar(D, '}');
    if (lonD_iskey(D)) lonD_addchar(D, ']');
    lonD_end(D);
    return 1;
}

LON_API int lon_dump_nil(lon_Dumper *D) {
    lonD_begin(D);
    if (lonD_iskey(D)) lonD_addstring(D, "['nil']");
    else lonD_addstring(D, "nil");
    lonD_end(D);
    return 0;
}

LON_API int lon_dump_boolean(lon_Dumper *D, int v) {
    int iskey = lonD_iskey(D);
    lonD_begin(D);
    if (iskey) lonD_addchar(D, '[');
    if (v) lonD_addstring(D, "true");
    else lonD_addstring(D, "false");
    if (iskey) lonD_addchar(D, ']');
    lonD_end(D);
    return 1;
}

LON_API int lon_dump_integer(lon_Dumper *D, lon_Integer v) {
    int iskey = lonD_iskey(D);
    lonD_begin(D);
    if (iskey && lonD_index(D) + 1 == v) {
        ++lonD_index(D);
        lonD_subsq(D) = 1;
        lonD_iskey(D) = !iskey; /* do not write '=' */
    }
    else {
        if (iskey) lonD_addchar(D, '[');
        if (D->opt_int_hexa)
            lonD_addfstring(D, "0x%x", v);
        else
            lonD_addfstring(D, "%d", v);
        if (iskey) lonD_addchar(D, ']');
        lonD_end(D);
    }
    return 1;
}

LON_API int lon_dump_number(lon_Dumper *D, lon_Number v) {
    int iskey = lonD_iskey(D);
    lonD_begin(D);
    if (iskey) lonD_addchar(D, '[');
    if (D->opt_flt_hexa)
        lonD_addfstring(D, "%a", v);
    else if (D->opt_flt_prec == 0)
        lonD_addfstring(D, "%g", v);
    else
        lonD_addfstring(D, "%.*g", (int)D->opt_flt_prec, v);
    if (iskey) lonD_addchar(D, ']');
    lonD_end(D);
    return 1;
}

LON_API int lon_dump_buffer(lon_Dumper *D, const char *s, size_t len) {
    int iskey = lonD_iskey(D);
    lonD_begin(D);
    if (iskey && lon_isidentifier(s, len)
            && lonX_checkkeyword(s, len) == TK_NAME)
        lonD_addlstring(D, s, len);
    else {
        if (iskey) lonD_addchar(D, '[');
        lonD_addescape(D, s, len);
        if (iskey) lonD_addchar(D, ']');
    }
    lonD_end(D);
    return 1;
}


LON_NS_END

#endif /* LON_IMPLEMENTATION */
/* cc: flags+='-s -O3 -mdll -DLON_IMPLEMENTATION'
 * cc: flags+='-DLON_API="__declspec(dllexport)" -xc'
 * cc: output='lon.dll' */
