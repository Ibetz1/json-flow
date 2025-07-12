#include "jf.h"
#include "stdio.h"

/*
    load a json file: parse it into jf_node linked B tree thingy??
    make a dif function comparing nodes

    recursively load a file tree into memory (filtering out json data, yml data, etc)
    actively monitor each file and evey time it is changed, commit a snapshot

    UI :)
*/

int main() {
    jf_start();

    jf_String str;
    jf_string_alloc(&str, "hello", sizeof("hello"));

    printf("%s\n", str.str);
    
    jf_String str2;
    jf_string_alloc(&str2, "hel", sizeof("hel"));

    printf("%s\n", str2.str);

    jf_string_copy(&str2, &str);

    printf("%s\n", str2.str);

    jf_string_free(&str);
    jf_string_free(&str2);

    jf_finish();
    return 0;
}