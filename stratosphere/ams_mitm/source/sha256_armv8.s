.section    .text.sha256_armv8, "ax", %progbits
.align      5
.arch       armv8-a+crypto

# SHA256 assembly implementation for ARMv8 AArch64 (based on linux source code)

.global	sha256_block_data_order
.type	sha256_block_data_order,%function
sha256_block_data_order:

.Lsha256prolog:

    stp       x29, x30, [sp,#-64]!
    mov       x29, sp
    adr       x3, .LKConstant256
    str       q8, [sp, #16]
    ld1       {v16.4s-v19.4s}, [x3], #64
    ld1       {v0.4s}, [x0], #16
    ld1       {v20.4s-v23.4s}, [x3], #64
    add       x2, x1, x2, lsl #6
    ld1       {v1.4s}, [x0]
    ld1       {v24.4s-v27.4s}, [x3], #64
    sub       x0, x0, #16
    str       q9, [sp, #32]
    str       q10, [sp, #48]
    ld1       {v28.4s-v31.4s}, [x3], #64
    
.Lsha256loop:

    ld1       {v5.16b-v8.16b}, [x1], #64
    mov       v2.16b, v0.16b
    mov       v3.16b, v1.16b

    rev32     v5.16b, v5.16b
    rev32     v6.16b, v6.16b
    add       v9.4s, v5.4s, v16.4s
    rev32     v7.16b, v7.16b
    add       v10.4s, v6.4s, v17.4s
    mov       v4.16b, v2.16b
    sha256h   q2, q3, v9.4s
    sha256h2  q3, q4, v9.4s
    sha256su0 v5.4s, v6.4s
    rev32     v8.16b, v8.16b
    add       v9.4s, v7.4s, v18.4s
    mov       v4.16b, v2.16b
    sha256h   q2, q3, v10.4s
    sha256h2  q3, q4, v10.4s
    sha256su0 v6.4s, v7.4s
    sha256su1 v5.4s, v7.4s, v8.4s
    add       v10.4s, v8.4s, v19.4s
    mov       v4.16b, v2.16b
    sha256h   q2, q3, v9.4s
    sha256h2  q3, q4, v9.4s
    sha256su0 v7.4s, v8.4s
    sha256su1 v6.4s, v8.4s, v5.4s
    add       v9.4s, v5.4s, v20.4s
    mov       v4.16b, v2.16b
    sha256h   q2, q3, v10.4s
    sha256h2  q3, q4, v10.4s
    sha256su0 v8.4s, v5.4s
    sha256su1 v7.4s, v5.4s, v6.4s
    add       v10.4s, v6.4s, v21.4s
    mov       v4.16b, v2.16b
    sha256h   q2, q3, v9.4s
    sha256h2  q3, q4, v9.4s
    sha256su0 v5.4s, v6.4s
    sha256su1 v8.4s, v6.4s, v7.4s
    add       v9.4s, v7.4s, v22.4s
    mov       v4.16b, v2.16b
    sha256h   q2, q3, v10.4s
    sha256h2  q3, q4, v10.4s
    sha256su0 v6.4s, v7.4s
    sha256su1 v5.4s, v7.4s, v8.4s
    add       v10.4s, v8.4s, v23.4s
    mov       v4.16b, v2.16b
    sha256h   q2, q3, v9.4s
    sha256h2  q3, q4, v9.4s
    sha256su0 v7.4s, v8.4s
    sha256su1 v6.4s, v8.4s, v5.4s
    add       v9.4s, v5.4s, v24.4s
    mov       v4.16b, v2.16b
    sha256h   q2, q3, v10.4s
    sha256h2  q3, q4, v10.4s
    sha256su0 v8.4s, v5.4s
    sha256su1 v7.4s, v5.4s, v6.4s
    add       v10.4s, v6.4s, v25.4s
    mov       v4.16b, v2.16b
    sha256h   q2, q3, v9.4s
    sha256h2  q3, q4, v9.4s
    sha256su0 v5.4s, v6.4s
    sha256su1 v8.4s, v6.4s, v7.4s
    add       v9.4s, v7.4s, v26.4s
    mov       v4.16b, v2.16b
    sha256h   q2, q3, v10.4s
    sha256h2  q3, q4, v10.4s
    sha256su0 v6.4s, v7.4s
    sha256su1 v5.4s, v7.4s, v8.4s
    add       v10.4s, v8.4s, v27.4s
    mov       v4.16b, v2.16b
    sha256h   q2, q3, v9.4s
    sha256h2  q3, q4, v9.4s
    sha256su0 v7.4s, v8.4s
    sha256su1 v6.4s, v8.4s, v5.4s
    add       v9.4s, v5.4s, v28.4s
    mov       v4.16b, v2.16b
    sha256h   q2, q3, v10.4s
    sha256h2  q3, q4, v10.4s
    sha256su0 v8.4s, v5.4s
    sha256su1 v7.4s, v5.4s, v6.4s
    add       v10.4s, v6.4s, v29.4s
    mov       v4.16b, v2.16b
    sha256h   q2, q3, v9.4s
    sha256h2  q3, q4, v9.4s
    sha256su1 v8.4s, v6.4s, v7.4s
    add       v9.4s, v7.4s, v30.4s
    mov       v4.16b, v2.16b
    sha256h   q2, q3, v10.4s
    sha256h2  q3, q4, v10.4s
    add       v10.4s, v8.4s, v31.4s
    mov       v4.16b, v2.16b
    sha256h   q2, q3, v9.4s
    sha256h2  q3, q4, v9.4s
    mov       v4.16b, v2.16b
    sha256h   q2, q3, v10.4s
    sha256h2  q3, q4, v10.4s
    cmp       x1, x2
    add       v1.4s, v1.4s, v3.4s
    add       v0.4s, v0.4s, v2.4s
    b.ne      .Lsha256loop

.Lsha256epilog:

    st1       {v0.4s,v1.4s}, [x0]
    ldr       q10, [sp, #48]
    ldr       q9, [sp, #32]
    ldr       q8, [sp, #16]
    ldr       x29, [sp], #64
    ret

.align  5
.LKConstant256:
.word   0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5
.word   0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5
.word   0xd807aa98,0x12835b01,0x243185be,0x550c7dc3
.word   0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174
.word   0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc
.word   0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da
.word   0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7
.word   0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967
.word   0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13
.word   0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85
.word   0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3
.word   0xd192e819,0xd6990624,0xf40e3585,0x106aa070
.word   0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5
.word   0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3
.word   0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208
.word   0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2

.size   sha256_block_data_order,.-sha256_block_data_order
.align	2



