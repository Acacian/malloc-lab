/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* 기본 상수 및 매크로 */
#define WSIZE 4 // 워드와 헤더 및 풋터 크기 정의
#define DSIZE 8 // 더블 워드 크기 정의
#define CHUNKSIZE (1<<12) // 초기 가용 블록과 확장시 추가되는 블록 크기 정의

#define MAX(x, y) ((x) > (y)? (x) : (y)) // 최대값 구하는 함수 정의

/* 헤더 및 풋터의 값(크기와 할당 여부) 반환 */
#define PACK(size, alloc) ((size) | (alloc)) // 크기와 할당 비트를 통합해서 헤더와 풋터에 저장할 수 있는 값 반환

/* 주소 p에서 워드를 읽거나 쓰는 함수 */
#define GET(p) (*(unsigned int *)(p)) // 포인터 p가 가리키는 블록의 데이터 반환
#define PUT(p, val) (*(unsigned int *)(p) = (val)) // 포인터 p가 가리키는 블록의 값 저장

/* 헤더 또는 풋터에서 블록의 크기와 할당된 구역을 읽어옴 */
// & ~0x7 => 0x7:0000 0111 ~0x7:1111 1000이므로 ex. 1011 0111 & 1111 1000 = 1011 0000 : size 176bytes
#define GET_SIZE(p) (GET(p) & ~0x7) // 포인터 p가 가리키는 헤더 또는 풋터의 크기 반환
// & 0x1 => ex. 1011 0111 | 0000 0001 = 1 : Allocated
#define GET_ALLOC(p) (GET(p) & 0x1) // 포인터 p가 가리키는 헤더 또는 풋터의 할당 비트 반환

/* 각각 블록 헤더와 풋터가 가리키는 포인터 반환 */
// bp : 현재 블록 포인터
#define HDRP(bp) ((char *)(bp) - WSIZE) // 현재 블록 헤더의 위치 반환(bp - 1워드)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE) // 현재 블록 풋터의 위치 반환(bp + 현재 블록 크기 - 더블 워드 크기)

/* 각각 이전 블록과 다음 블록을 가리키는 포인터 반환 */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE))) // 다음 블록의 블록 포인터 반환(bp + 현재 블록 크기 - 1워드)
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE))) // 이전 블록의 블록 포인터 반환(bp - 현재 블록 크기 - 2워드)

// 전역 힙 변수 및 함수 선언
// 추가해야됨



int mm_init(void)
{
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1) return -1; // 초기 가용 블록 생성
    PUT(heap_listp, 0); // 정렬을 위한 패딩
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1)); // 프롤로그 헤더
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1)); // 프롤로그 풋터
    PUT(heap_listp + (3*WSIZE), PACK(0, 1)); // 에필로그 헤더
    heap_listp += (2*WSIZE);

    if (extend_heap(CHUNKSIZE/WSIZE) == NULL) return -1;
    return 0;
}

static void *extend_heap(size_t words) // 새 가용 블록 생성 및 기존 가용 블록과 합침
{
    char *bp; //char 쓰는 이유 : 1바이트면 주소 읽기 충분
    size_t size;

    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE; //정렬 유지를 위해 짝수 개의 워드를 할당
    if ((long)(bp = mem_sbrk(size)) == -1) return NULL; // 홀수면 워드 하나 더 할당

    PUT(HDRP(bp), PACK(size, 0)); // 새로운 블록 헤더
    PUT(FTRP(bp), PACK(size, 0)); // 새로운 블록 풋터
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); // 새로운 에필로그 헤더

    return coalesce(bp); // 통합된 블록 반환
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize; // 조정된 블록 크기
    size_t extendsize; // 가용 블록 확장 크기
    char *bp;
    if (size == 0) return NULL; // 요청 크기가 0이면 NULL 반환
    if (size <= DSIZE) asize = 2*DSIZE; // 요청 크기가 8바이트 이하면 16바이트로 조정
    else asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE); // 요청 크기가 8바이트 초과면 8의 배수로 조정
    if ((bp = find_fit(asize)) != NULL) { // 가용 블록을 찾으면 할당
    place(bp, asize);
    return bp;
    }
    extendsize = MAX(asize, CHUNKSIZE); // 가용 블록을 찾지 못하면 힙을 확장
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL) return NULL;
    place(bp, asize);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    size_t size = GET_SIZE(HDRP(bp));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}

static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) return bp; // 이전 블록과 다음 블록이 모두 할당되어 있으면 그대로 반환 case 1

    else if (prev_alloc && !next_alloc) { // 이전 블록은 할당되어 있고 다음 블록은 가용되어 있으면 다음 블록과 통합 case 2
    size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    }

    else if (!prev_alloc && next_alloc) { // 이전 블록은 가용되어 있고 다음 블록은 할당되어 있으면 이전 블록과 통합 case 3
    size += GET_SIZE(HDRP(PREV_BLKP(bp)));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
    bp = PREV_BLKP(bp);
    }

    else {
    size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp))); // 이전 블록과 다음 블록 모두 가용되어 있으면 이전 블록과 다음 블록과 통합 case 4
    PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
    PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
    bp = PREV_BLKP(bp);
    }
    return bp;
}


/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}














