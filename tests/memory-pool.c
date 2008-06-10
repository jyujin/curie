/*
 *  memory-pool.c
 *  atomic-libc
 *
 *  Created by Magnus Deininger on 08/06/2008.
 *  Copyright 2008 Magnus Deininger. All rights reserved.
 *
 */

/*
 * Copyright (c) 2008, Magnus Deininger All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer. *
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution. *
 * Neither the name of the project nor the names of its contributors may
 * be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*@-usedef@*/
/*@-unqualifiedtrans@*/

#include "atomic/memory.h"

#define MAXSIZE 23
#define MAXENTITIES 1026

static unsigned int test_pool(unsigned int poolentitysize, unsigned int usepoolentities, int do_negate) {
    struct memory_pool *pool = create_memory_pool(poolentitysize);
	char *entities[usepoolentities];
	unsigned int i, j, rv = 0;

    for (i = 0; i < usepoolentities; i++) {
	    entities[i] = get_pool_mem(pool);
	}

    for (i = 0; i < usepoolentities; i++) {
	    for (j = 0; j < poolentitysize; j++) {
		    if (do_negate == 1)
                entities[i][j] = (char)~j;
			else
                entities[i][j] = (char)j;
        }
	}

    for (i = 0; i < usepoolentities; i++) {
	    for (j = 0; j < poolentitysize; j++) {
            if (((do_negate == 1) && (entities[i][j] != (char)~j)) ||
				((do_negate == 0) && (entities[i][j] != (char)j))){
			    rv = 1;
				goto do_return;
			}
        }
	}
	
	do_return:

    for (i = 0; i < usepoolentities; i++) {
        free_pool_mem ((void *)(entities[i]));
	}

	free_memory_pool(pool);

	return rv;
}

int atomic_main(void) {
    unsigned int i, j;

	for (i = 1; i < MAXSIZE; i++) {
		for (j = 1; j < MAXENTITIES; j++) {
            if (test_pool(i, j, 1) == 1) return 1;
            if (test_pool(i, j, 0) == 1) return 1;
        }
	}

	return 0;
}