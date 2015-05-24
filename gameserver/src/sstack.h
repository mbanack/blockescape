/*
THERE IS NO WARRANTY FOR THE PROGRAM, TO THE EXTENT PERMITTED BY
APPLICABLE LAW.  EXCEPT WHEN OTHERWISE STATED IN WRITING THE COPYRIGHT
HOLDERS AND/OR OTHER PARTIES PROVIDE THE PROGRAM "AS IS" WITHOUT WARRANTY
OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE.  THE ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE OF THE PROGRAM
IS WITH YOU.  SHOULD THE PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF
ALL NECESSARY SERVICING, REPAIR OR CORRECTION.
IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING
WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MODIFIES AND/OR CONVEYS
THE PROGRAM AS PERMITTED ABOVE, BE LIABLE TO YOU FOR DAMAGES, INCLUDING ANY
GENERAL, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THE
USE OR INABILITY TO USE THE PROGRAM (INCLUDING BUT NOT LIMITED TO LOSS OF
DATA OR DATA BEING RENDERED INACCURATE OR LOSSES SUSTAINED BY YOU OR THIRD
PARTIES OR A FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER PROGRAMS),
EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE POSSIBILITY OF
SUCH DAMAGES.
*/
// sstack.h
// solver stack implementation
//   keyed by board hash (*bsref)

#ifndef SSTACK_H
#define SSTACK_H

#define SSTACK_SIZE 10240

// solver stack
typedef struct sstack {
    bsref arr[SSTACK_SIZE];
    // next free index
    int idx;
} sstack;

void sstack_init(sstack *s);
int sstack_contains(sstack *s, bsref *key);
int sstack_empty(sstack *s);
int sstack_size(sstack *s);
void sstack_push(sstack *s, bsref *key);
void sstack_pop(sstack *s, bsref *key_out);
void sstack_peek(sstack *s, bsref *key_out);
void sstack_print(sstack *s);

// dep stack
#define DSTACK_SIZE 32
typedef struct dstack {
    int arr[DSTACK_SIZE];
    // next free index
    int idx;
} dstack;

void dstack_init(dstack *s);
int dstack_contains(dstack *s, int key);
int dstack_empty(dstack *s);
int dstack_size(dstack *s);
void dstack_push(dstack *s, int key);
int dstack_pop(dstack *s);
int dstack_peek(dstack *s);

#endif // SSTACK_H
