/**
 * @file ini.c
 * @brief Simple INI parser implementation (ANSI, no CRT).
 * @license MIT
 * @copyright Copyright (c) 2025 Bashkardin Pavel
 */

#include "ini.h"
#include <windows.h>

static size_t StrLen(const char* s) {
    const char* p = s;
    while (*p) ++p;
    return (size_t)(p - s);
}

static char* StrDup(const char* s, HANDLE heap) {
    if (!s) return NULL;
    size_t len = StrLen(s) + 1;
    char* d = (char*)HeapAlloc(heap, 0, len);
    if (d) {
        char* pd = d;
        while ((*pd++ = *s++));
    }
    return d;
}

static int StrCmp(const char* a, const char* b) {
    while (*a && *b && *a == *b) { ++a; ++b; }
    return (*(unsigned char*)a - *(unsigned char*)b);
}

static char* StrChr(const char* s, int c) {
    while (*s) {
        if (*s == (char)c) return (char*)s;
        ++s;
    }
    return NULL;
}

static int IsSpace(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static const char* SkipSpaces(const char* s) {
    while (IsSpace(*s)) ++s;
    return s;
}

static void TrimRight(char* s) {
    size_t len = StrLen(s);
    while (len > 0 && IsSpace(s[len-1])) {
        s[--len] = '\0';
    }
}

typedef struct KeyValue {
    char* key;
    char* value;
    struct KeyValue* next;
} KeyValue;

typedef struct Section {
    char* name;
    KeyValue* first_kv;
    struct Section* next;
    HANDLE heap;
} Section;

struct IniFile {
    HANDLE heap;
    Section* first_section;
};

static void FreeKeyValues(KeyValue* kv, HANDLE heap) {
    while (kv) {
        KeyValue* next = kv->next;
        if (kv->key) HeapFree(heap, 0, kv->key);
        if (kv->value) HeapFree(heap, 0, kv->value);
        HeapFree(heap, 0, kv);
        kv = next;
    }
}

static void FreeSections(Section* sec, HANDLE heap) {
    while (sec) {
        Section* next = sec->next;
        if (sec->name) HeapFree(heap, 0, sec->name);
        FreeKeyValues(sec->first_kv, heap);
        HeapFree(heap, 0, sec);
        sec = next;
    }
}

static Section* FindSection(IniFile* ini, const char* name) {
    Section* sec = ini->first_section;
    while (sec) {
        if (StrCmp(sec->name, name) == 0)
            return sec;
        sec = sec->next;
    }
    return NULL;
}

static void AddKeyValue(Section* sec, char* key, char* value) {
    KeyValue* kv = (KeyValue*)HeapAlloc(sec->heap, 0, sizeof(KeyValue));
    if (!kv) return;
    kv->key = key;
    kv->value = value;
    kv->next = NULL;
    if (!sec->first_kv) {
        sec->first_kv = kv;
    } else {
        KeyValue* last = sec->first_kv;
        while (last->next) last = last->next;
        last->next = kv;
    }
}

IniFile* IniParse(const char* data, HANDLE heap, DWORD flags) {
    (void)flags;
    if (!data) return NULL;
    if (!heap) heap = GetProcessHeap();

    IniFile* ini = (IniFile*)HeapAlloc(heap, HEAP_ZERO_MEMORY, sizeof(IniFile));
    if (!ini) return NULL;
    ini->heap = heap;

    size_t len = StrLen(data);
    char* buf = (char*)HeapAlloc(heap, 0, len + 1);
    if (!buf) {
        HeapFree(heap, 0, ini);
        return NULL;
    }
    char* p = buf;
    while ((*p++ = *data++));
    p = buf;

    Section* current_section = NULL;

    while (*p) {
        char* eol = StrChr(p, '\n');
        if (eol) *eol = '\0';
        char* next_line = eol ? eol + 1 : p + StrLen(p);

        char* line = (char*)SkipSpaces(p);
        if (*line && *line != ';' && *line != '#') {
            if (*line == '[') {
                char* end = StrChr(line + 1, ']');
                if (end) {
                    *end = '\0';
                    char* sec_name = (char*)SkipSpaces(line + 1);
                    TrimRight(sec_name);
                    if (*sec_name) {
                        Section* sec = FindSection(ini, sec_name);
                        if (!sec) {
                            sec = (Section*)HeapAlloc(heap, HEAP_ZERO_MEMORY, sizeof(Section));
                            if (sec) {
                                sec->name = StrDup(sec_name, heap);
                                sec->heap = heap;
                                sec->next = ini->first_section;
                                ini->first_section = sec;
                            }
                        }
                        current_section = sec;
                    }
                }
            } else if (current_section) {
                char* eq = StrChr(line, '=');
                if (eq) {
                    *eq = '\0';
                    char* key = (char*)SkipSpaces(line);
                    TrimRight(key);
                    char* val = (char*)SkipSpaces(eq + 1);
                    TrimRight(val);
                    if (*key) {
                        char* key_dup = StrDup(key, heap);
                        char* val_dup = StrDup(val, heap);
                        AddKeyValue(current_section, key_dup, val_dup);
                    }
                }
            }
        }
        p = next_line;
    }

    HeapFree(heap, 0, buf);
    return ini;
}

void IniFree(IniFile* ini) {
    if (!ini) return;
    FreeSections(ini->first_section, ini->heap);
    HeapFree(ini->heap, 0, ini);
}

const char* IniGetValue(IniFile* ini, const char* section, const char* key) {
    if (!ini || !section || !key) return NULL;
    Section* sec = FindSection(ini, section);
    if (!sec) return NULL;
    KeyValue* kv = sec->first_kv;
    while (kv) {
        if (StrCmp(kv->key, key) == 0)
            return kv->value;
        kv = kv->next;
    }
    return NULL;
}

const char* IniGetString(IniFile* ini, const char* section, const char* key, const char* defaultValue) {
    const char* val = IniGetValue(ini, section, key);
    return val ? val : defaultValue;
}

static int StringToInt(const char* s) {
    int result = 0;
    int base = 10;
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        base = 16;
        s += 2;
    }
    while (*s) {
        int digit;
        if (*s >= '0' && *s <= '9') digit = *s - '0';
        else if (base == 16 && *s >= 'a' && *s <= 'f') digit = *s - 'a' + 10;
        else if (base == 16 && *s >= 'A' && *s <= 'F') digit = *s - 'A' + 10;
        else break;
        result = result * base + digit;
        ++s;
    }
    return result;
}

int IniGetInteger(IniFile* ini, const char* section, const char* key, int defaultValue) {
    const char* val = IniGetValue(ini, section, key);
    if (!val) return defaultValue;
    // skip leading spaces
    while (*val && (*val == ' ' || *val == '\t')) ++val;
    if (*val == '\0') return defaultValue;
    return StringToInt(val);
}