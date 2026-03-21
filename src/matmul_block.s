	.section	__TEXT,__text,regular,pure_instructions
	.build_version macos, 14, 0	sdk_version 14, 4
	.globl	_matmul_block_set_tiles         ; -- Begin function matmul_block_set_tiles
	.p2align	2
_matmul_block_set_tiles:                ; @matmul_block_set_tiles
	.cfi_startproc
; %bb.0:
	cbz	x0, LBB0_2
; %bb.1:
	adrp	x8, __MergedGlobals@PAGE
	str	x0, [x8, __MergedGlobals@PAGEOFF]
LBB0_2:
	cbz	x1, LBB0_4
; %bb.3:
	adrp	x8, __MergedGlobals@PAGE+8
	str	x1, [x8, __MergedGlobals@PAGEOFF+8]
LBB0_4:
	cbz	x2, LBB0_6
; %bb.5:
	adrp	x8, __MergedGlobals@PAGE+16
	str	x2, [x8, __MergedGlobals@PAGEOFF+16]
LBB0_6:
	ret
	.cfi_endproc
                                        ; -- End function
	.globl	_matmul_block_get_tiles         ; -- Begin function matmul_block_get_tiles
	.p2align	2
_matmul_block_get_tiles:                ; @matmul_block_get_tiles
	.cfi_startproc
; %bb.0:
	cbz	x0, LBB1_2
; %bb.1:
Lloh0:
	adrp	x8, __MergedGlobals@PAGE
Lloh1:
	ldr	x8, [x8, __MergedGlobals@PAGEOFF]
	str	x8, [x0]
LBB1_2:
	cbz	x1, LBB1_4
; %bb.3:
Lloh2:
	adrp	x8, __MergedGlobals@PAGE+8
Lloh3:
	ldr	x8, [x8, __MergedGlobals@PAGEOFF+8]
	str	x8, [x1]
LBB1_4:
	cbz	x2, LBB1_6
; %bb.5:
Lloh4:
	adrp	x8, __MergedGlobals@PAGE+16
Lloh5:
	ldr	x8, [x8, __MergedGlobals@PAGEOFF+16]
	str	x8, [x2]
LBB1_6:
	ret
	.loh AdrpLdr	Lloh0, Lloh1
	.loh AdrpLdr	Lloh2, Lloh3
	.loh AdrpLdr	Lloh4, Lloh5
	.cfi_endproc
                                        ; -- End function
	.globl	_matmul_block                   ; -- Begin function matmul_block
	.p2align	2
_matmul_block:                          ; @matmul_block
	.cfi_startproc
; %bb.0:
	sub	sp, sp, #208
	.cfi_def_cfa_offset 208
	stp	x28, x27, [sp, #112]            ; 16-byte Folded Spill
	stp	x26, x25, [sp, #128]            ; 16-byte Folded Spill
	stp	x24, x23, [sp, #144]            ; 16-byte Folded Spill
	stp	x22, x21, [sp, #160]            ; 16-byte Folded Spill
	stp	x20, x19, [sp, #176]            ; 16-byte Folded Spill
	stp	x29, x30, [sp, #192]            ; 16-byte Folded Spill
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	.cfi_offset w19, -24
	.cfi_offset w20, -32
	.cfi_offset w21, -40
	.cfi_offset w22, -48
	.cfi_offset w23, -56
	.cfi_offset w24, -64
	.cfi_offset w25, -72
	.cfi_offset w26, -80
	.cfi_offset w27, -88
	.cfi_offset w28, -96
	str	x5, [sp, #56]                   ; 8-byte Folded Spill
	str	x4, [sp, #8]                    ; 8-byte Folded Spill
	cbz	x0, LBB2_29
; %bb.1:
	cbz	x1, LBB2_29
; %bb.2:
	cbz	x2, LBB2_29
; %bb.3:
	mov	x6, #0
Lloh6:
	adrp	x8, __MergedGlobals@PAGE
Lloh7:
	add	x8, x8, __MergedGlobals@PAGEOFF
	ldp	x10, x9, [x8]
	ldr	x11, [x8, #16]
	ldr	x8, [sp, #56]                   ; 8-byte Folded Reload
	add	x8, x8, #32
	str	x8, [sp, #48]                   ; 8-byte Folded Spill
	mul	x8, x10, x1
	lsl	x8, x8, #3
	stp	x8, x10, [sp, #16]              ; 16-byte Folded Spill
	stp	x1, x11, [sp, #72]              ; 16-byte Folded Spill
	lsl	x8, x11, #3
	str	x8, [sp, #64]                   ; 8-byte Folded Spill
	lsl	x15, x1, #3
	ldr	x8, [sp, #8]                    ; 8-byte Folded Reload
	add	x8, x8, #32
	str	x8, [sp]                        ; 8-byte Folded Spill
	mul	x8, x9, x1
	lsl	x17, x8, #3
	str	x0, [sp, #32]                   ; 8-byte Folded Spill
	b	LBB2_5
LBB2_4:                                 ;   in Loop: Header=BB2_5 Depth=1
	ldr	x8, [sp, #48]                   ; 8-byte Folded Reload
	ldr	x10, [sp, #16]                  ; 8-byte Folded Reload
	add	x8, x8, x10
	str	x8, [sp, #48]                   ; 8-byte Folded Spill
	ldr	x8, [sp, #56]                   ; 8-byte Folded Reload
	add	x8, x8, x10
	str	x8, [sp, #56]                   ; 8-byte Folded Spill
	ldp	x0, x8, [sp, #32]               ; 16-byte Folded Reload
	mov	x6, x8
	cmp	x8, x0
	b.hs	LBB2_29
LBB2_5:                                 ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB2_8 Depth 2
                                        ;       Child Loop BB2_20 Depth 3
                                        ;         Child Loop BB2_22 Depth 4
                                        ;           Child Loop BB2_24 Depth 5
                                        ;             Child Loop BB2_25 Depth 6
                                        ;             Child Loop BB2_28 Depth 6
                                        ;       Child Loop BB2_12 Depth 3
                                        ;         Child Loop BB2_13 Depth 4
                                        ;           Child Loop BB2_14 Depth 5
                                        ;             Child Loop BB2_15 Depth 6
	ldr	x8, [sp, #24]                   ; 8-byte Folded Reload
	add	x8, x6, x8
	cmp	x8, x0
	str	x8, [sp, #40]                   ; 8-byte Folded Spill
	csel	x7, x8, x0, lo
	cmp	x6, x7
	b.hs	LBB2_4
; %bb.6:                                ;   in Loop: Header=BB2_5 Depth=1
	mov	x24, #0
	ldp	x8, x10, [sp]                   ; 16-byte Folded Reload
	ldp	x25, x5, [sp, #48]              ; 16-byte Folded Reload
	stp	x8, xzr, [sp, #96]              ; 16-byte Folded Spill
	b	LBB2_8
LBB2_7:                                 ;   in Loop: Header=BB2_8 Depth=2
	ldp	x11, x8, [sp, #96]              ; 16-byte Folded Reload
	add	x8, x8, #1
	str	x8, [sp, #104]                  ; 8-byte Folded Spill
	ldp	x8, x1, [sp, #64]               ; 16-byte Folded Reload
	add	x25, x25, x8
	add	x11, x11, x8
	str	x11, [sp, #96]                  ; 8-byte Folded Spill
	add	x5, x5, x8
	add	x10, x10, x8
	ldr	x8, [sp, #88]                   ; 8-byte Folded Reload
	mov	x24, x8
	cmp	x8, x1
	b.hs	LBB2_4
LBB2_8:                                 ;   Parent Loop BB2_5 Depth=1
                                        ; =>  This Loop Header: Depth=2
                                        ;       Child Loop BB2_20 Depth 3
                                        ;         Child Loop BB2_22 Depth 4
                                        ;           Child Loop BB2_24 Depth 5
                                        ;             Child Loop BB2_25 Depth 6
                                        ;             Child Loop BB2_28 Depth 6
                                        ;       Child Loop BB2_12 Depth 3
                                        ;         Child Loop BB2_13 Depth 4
                                        ;           Child Loop BB2_14 Depth 5
                                        ;             Child Loop BB2_15 Depth 6
	ldr	x8, [sp, #80]                   ; 8-byte Folded Reload
	add	x8, x24, x8
	cmp	x1, x8
	str	x8, [sp, #88]                   ; 8-byte Folded Spill
	csel	x26, x1, x8, lo
	cmp	x24, x26
	b.hs	LBB2_7
; %bb.9:                                ;   in Loop: Header=BB2_8 Depth=2
	add	x8, x24, #1
	cmp	x26, x8
	csinc	x8, x26, x24, hi
	ldr	x11, [sp, #80]                  ; 8-byte Folded Reload
	ldr	x12, [sp, #104]                 ; 8-byte Folded Reload
	msub	x27, x11, x12, x8
	cmp	x27, #8
	b.hs	LBB2_18
; %bb.10:                               ;   in Loop: Header=BB2_8 Depth=2
	mov	x12, #0
	mov	x8, x10
	b	LBB2_12
LBB2_11:                                ;   in Loop: Header=BB2_12 Depth=3
	add	x8, x8, x17
	mov	x12, x14
	cmp	x14, x2
	b.hs	LBB2_7
LBB2_12:                                ;   Parent Loop BB2_5 Depth=1
                                        ;     Parent Loop BB2_8 Depth=2
                                        ; =>    This Loop Header: Depth=3
                                        ;         Child Loop BB2_13 Depth 4
                                        ;           Child Loop BB2_14 Depth 5
                                        ;             Child Loop BB2_15 Depth 6
	add	x14, x12, x9
	cmp	x14, x2
	csel	x16, x14, x2, lo
	mov	x0, x5
	mov	x1, x6
	cmp	x12, x16
	b.hs	LBB2_11
LBB2_13:                                ;   Parent Loop BB2_5 Depth=1
                                        ;     Parent Loop BB2_8 Depth=2
                                        ;       Parent Loop BB2_12 Depth=3
                                        ; =>      This Loop Header: Depth=4
                                        ;           Child Loop BB2_14 Depth 5
                                        ;             Child Loop BB2_15 Depth 6
	mul	x11, x1, x2
	mov	x13, x8
	mov	x4, x12
LBB2_14:                                ;   Parent Loop BB2_5 Depth=1
                                        ;     Parent Loop BB2_8 Depth=2
                                        ;       Parent Loop BB2_12 Depth=3
                                        ;         Parent Loop BB2_13 Depth=4
                                        ; =>        This Loop Header: Depth=5
                                        ;             Child Loop BB2_15 Depth 6
	mov	x19, #0
	add	x20, x4, x11
	ldr	d0, [x3, x20, lsl #3]
LBB2_15:                                ;   Parent Loop BB2_5 Depth=1
                                        ;     Parent Loop BB2_8 Depth=2
                                        ;       Parent Loop BB2_12 Depth=3
                                        ;         Parent Loop BB2_13 Depth=4
                                        ;           Parent Loop BB2_14 Depth=5
                                        ; =>          This Inner Loop Header: Depth=6
	lsl	x20, x19, #3
	ldr	d1, [x13, x20]
	ldr	d2, [x0, x20]
	fmadd	d1, d0, d1, d2
	str	d1, [x0, x20]
	add	x19, x19, #1
	add	x20, x24, x19
	cmp	x20, x26
	b.lo	LBB2_15
; %bb.16:                               ;   in Loop: Header=BB2_14 Depth=5
	add	x4, x4, #1
	add	x13, x13, x15
	cmp	x4, x16
	b.lo	LBB2_14
; %bb.17:                               ;   in Loop: Header=BB2_13 Depth=4
	add	x1, x1, #1
	add	x0, x0, x15
	cmp	x1, x7
	b.lo	LBB2_13
	b	LBB2_11
LBB2_18:                                ;   in Loop: Header=BB2_8 Depth=2
	mov	x4, #0
	and	x19, x27, #0xfffffffffffffff8
	mov	x20, x10
	ldr	x13, [sp, #96]                  ; 8-byte Folded Reload
	b	LBB2_20
LBB2_19:                                ;   in Loop: Header=BB2_20 Depth=3
	add	x13, x13, x17
	add	x20, x20, x17
	mov	x4, x1
	cmp	x1, x2
	b.hs	LBB2_7
LBB2_20:                                ;   Parent Loop BB2_5 Depth=1
                                        ;     Parent Loop BB2_8 Depth=2
                                        ; =>    This Loop Header: Depth=3
                                        ;         Child Loop BB2_22 Depth 4
                                        ;           Child Loop BB2_24 Depth 5
                                        ;             Child Loop BB2_25 Depth 6
                                        ;             Child Loop BB2_28 Depth 6
	add	x1, x4, x9
	cmp	x1, x2
	csel	x12, x1, x2, lo
	mov	x21, x5
	mov	x11, x25
	mov	x14, x6
	cmp	x4, x12
	b.lo	LBB2_22
	b	LBB2_19
LBB2_21:                                ;   in Loop: Header=BB2_22 Depth=4
	add	x14, x14, #1
	add	x11, x11, x15
	add	x21, x21, x15
	cmp	x14, x7
	b.hs	LBB2_19
LBB2_22:                                ;   Parent Loop BB2_5 Depth=1
                                        ;     Parent Loop BB2_8 Depth=2
                                        ;       Parent Loop BB2_20 Depth=3
                                        ; =>      This Loop Header: Depth=4
                                        ;           Child Loop BB2_24 Depth 5
                                        ;             Child Loop BB2_25 Depth 6
                                        ;             Child Loop BB2_28 Depth 6
	mul	x0, x14, x2
	mov	x30, x20
	mov	x16, x13
	mov	x8, x4
	b	LBB2_24
LBB2_23:                                ;   in Loop: Header=BB2_24 Depth=5
	add	x8, x8, #1
	add	x16, x16, x15
	add	x30, x30, x15
	cmp	x8, x12
	b.hs	LBB2_21
LBB2_24:                                ;   Parent Loop BB2_5 Depth=1
                                        ;     Parent Loop BB2_8 Depth=2
                                        ;       Parent Loop BB2_20 Depth=3
                                        ;         Parent Loop BB2_22 Depth=4
                                        ; =>        This Loop Header: Depth=5
                                        ;             Child Loop BB2_25 Depth 6
                                        ;             Child Loop BB2_28 Depth 6
	add	x22, x8, x0
	ldr	d0, [x3, x22, lsl #3]
	dup.2d	v1, v0[0]
	mov	x22, x16
	mov	x23, x11
	mov	x28, x19
LBB2_25:                                ;   Parent Loop BB2_5 Depth=1
                                        ;     Parent Loop BB2_8 Depth=2
                                        ;       Parent Loop BB2_20 Depth=3
                                        ;         Parent Loop BB2_22 Depth=4
                                        ;           Parent Loop BB2_24 Depth=5
                                        ; =>          This Inner Loop Header: Depth=6
	ldp	q2, q3, [x22, #-32]
	ldp	q4, q5, [x22], #64
	ldp	q6, q7, [x23, #-32]
	ldp	q16, q17, [x23]
	fmla.2d	v6, v2, v1
	fmla.2d	v7, v3, v1
	fmla.2d	v16, v4, v1
	fmla.2d	v17, v5, v1
	stp	q6, q7, [x23, #-32]
	stp	q16, q17, [x23], #64
	subs	x28, x28, #8
	b.ne	LBB2_25
; %bb.26:                               ;   in Loop: Header=BB2_24 Depth=5
	cmp	x27, x19
	b.eq	LBB2_23
; %bb.27:                               ;   in Loop: Header=BB2_24 Depth=5
	mov	x22, x19
LBB2_28:                                ;   Parent Loop BB2_5 Depth=1
                                        ;     Parent Loop BB2_8 Depth=2
                                        ;       Parent Loop BB2_20 Depth=3
                                        ;         Parent Loop BB2_22 Depth=4
                                        ;           Parent Loop BB2_24 Depth=5
                                        ; =>          This Inner Loop Header: Depth=6
	lsl	x23, x22, #3
	ldr	d1, [x30, x23]
	ldr	d2, [x21, x23]
	fmadd	d1, d0, d1, d2
	str	d1, [x21, x23]
	add	x22, x22, #1
	add	x23, x24, x22
	cmp	x23, x26
	b.lo	LBB2_28
	b	LBB2_23
LBB2_29:
	ldp	x29, x30, [sp, #192]            ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #176]            ; 16-byte Folded Reload
	ldp	x22, x21, [sp, #160]            ; 16-byte Folded Reload
	ldp	x24, x23, [sp, #144]            ; 16-byte Folded Reload
	ldp	x26, x25, [sp, #128]            ; 16-byte Folded Reload
	ldp	x28, x27, [sp, #112]            ; 16-byte Folded Reload
	add	sp, sp, #208
	ret
	.loh AdrpAdd	Lloh6, Lloh7
	.cfi_endproc
                                        ; -- End function
	.section	__DATA,__data
	.p2align	3, 0x0                          ; @_MergedGlobals
__MergedGlobals:
	.quad	64                              ; 0x40
	.quad	32                              ; 0x20
	.quad	64                              ; 0x40

.subsections_via_symbols
