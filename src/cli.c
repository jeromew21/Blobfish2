#include "chess.h"
#include "test.h"
#include "uci.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#if defined(_WIN32) || defined(WIN32)

int clock_gettime(int unused, struct timespec *spec)      //C-file part
{
    (void) unused;
    __int64 wintime;
    GetSystemTimeAsFileTime((FILETIME *) &wintime);
    wintime -= 116444736000000000i64;  //1jan1601 to 1jan1970
    spec->tv_sec = wintime / 10000000i64;           //seconds
    spec->tv_nsec = wintime % 10000000i64 * 100;      //nano-seconds
    return 0;
}

#endif

#define LINE_BUFFER_SIZE 32768

void read_input_into_buffer(char *line_buffer) {
    // Technically this line buffer could overflow...
    fgets(line_buffer, LINE_BUFFER_SIZE, stdin);
}

int main(void) {
    srand(0);
    setbuf(stdout, NULL);
    setbuf(stdin, NULL);
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stdin, NULL, _IONBF, 0);
    engine_initialize();
    char *line_buffer =
            malloc(sizeof(char) * LINE_BUFFER_SIZE); // what's max input len?
    while (1) {
        read_input_into_buffer(line_buffer);
        engine_command(line_buffer);
        EngineContext *ctx = engine_get_context();
        if (ctx->quit) {
            break;
        }
    }
    free(line_buffer);
    engine_cleanup();
}

#undef LINE_BUFFER_SIZE
