#if defined(__aarch64__)
    /* The called routine is expected to preserve r19-r28 *** These registers are generally
        safe to use in your program. */
    #define JB_X19            0
    #define JB_X20            1
    #define JB_X21            2
    #define JB_X22            3
    #define JB_X23            4
    #define JB_X24            5
    #define JB_X25            6
    #define JB_X26            7
    #define JB_X27            8
    #define JB_X28            9
    /* r29 and r30 are used as the frame register and link register (avoid) */
    #define JB_X29           10
    #define JB_LR            11
    /* Register '31' is one of two registers depending on the instruction context:
        For instructions dealing with the stack, it is the stack pointer, named rsp */
    #define JB_SP		 13

    /* FP registers */
    #define JB_D8		 14
    #define JB_D9		 15
    #define JB_D10		 16
    #define JB_D11		 17
    #define JB_D12		 18
    #define JB_D13		 19
    #define JB_D14		 20
    #define JB_D15		 21

    	.file "md.S"
    	.text

    	/* _st_md_cxt_save(__jmp_buf env) */
    	.globl _st_md_cxt_save
    	.type _st_md_cxt_save, %function
    	.align 4
    _st_md_cxt_save:
    	stp	x19, x20, [x0, #JB_X19<<3]
    	stp	x21, x22, [x0, #JB_X21<<3]
    	stp	x23, x24, [x0, #JB_X23<<3]
    	stp	x25, x26, [x0, #JB_X25<<3]
    	stp	x27, x28, [x0, #JB_X27<<3]
    	stp	x29, x30, [x0, #JB_X29<<3]

    	stp	 d8,  d9, [x0, #JB_D8<<3]
    	stp	d10, d11, [x0, #JB_D10<<3]
    	stp	d12, d13, [x0, #JB_D12<<3]
    	stp	d14, d15, [x0, #JB_D14<<3]
    	mov	x2,  sp
    	str	x2,  [x0, #JB_SP<<3]

    	mov	x0, #0
    	ret
    	.size _st_md_cxt_save, .-_st_md_cxt_save

    /****************************************************************/

    	/* _st_md_cxt_restore(__jmp_buf env, int val) */
    	.globl _st_md_cxt_restore
    	.type _st_md_cxt_restore, %function
    	.align 4
    _st_md_cxt_restore:
    	ldp	x19, x20, [x0, #JB_X19<<3]
    	ldp	x21, x22, [x0, #JB_X21<<3]
    	ldp	x23, x24, [x0, #JB_X23<<3]
    	ldp	x25, x26, [x0, #JB_X25<<3]
    	ldp	x27, x28, [x0, #JB_X27<<3]

    	ldp	x29, x30, [x0, #JB_X29<<3]

    	ldp	 d8,  d9, [x0, #JB_D8<<3]
    	ldp	d10, d11, [x0, #JB_D10<<3]
    	ldp	d12, d13, [x0, #JB_D12<<3]
    	ldp	d14, d15, [x0, #JB_D14<<3]

    	ldr	x5, [x0, #JB_SP<<3]
    	mov	sp, x5

    	cmp	x1, #0
    	mov	x0, #1
    	csel	x0, x1, x0, ne
    	/* Use br instead of ret because ret is guaranteed to mispredict */
    	br	x30
    	.size _st_md_cxt_restore, .-_st_md_cxt_restore

    /****************************************************************/

#endif
