#!/bin/bash

gcc -Wall -Wextra -Werror -D DEBUG_MODE -I../ test-buff.c ../lwjson.c && ./a.out tmp.json "[0].id"
gcc -Wall -Wextra -Werror -D DEBUG_MODE -I../ test-ptr.c ../lwjson.c && ./a.out tmp.json "[0].id"
gcc -Wall -Wextra -Werror -D DEBUG_MODE -I../ test-alloc.c ../lwjson.c && ./a.out tmp.json "[0].id"

