.data
stack_storage:
    .zero 64
stack_end:

cube_input:
    .asciz "21345671111111"
p:
    .zero 7
o:
    .zero 7
.align 4
unrank_weights:
    .word 720, 120, 24, 6, 2, 1, 1
.text

main:
     la sp, stack_end
     andi sp, sp, -16
     la s0, p
     la s1, o
     la a0, cube_input
     li t0, 0
     li t4, 7 #stop condition  for array_p
array_p:
     add t1, a0, t0 # t1 is the address of target index for input
     lbu t2, 0(t1)
     addi t2, t2, -49 #convert into zero-base
     add t3, s0, t0
     sb t2, 0(t3)
     addi t0, t0, 1 # got the next index offset
     blt t0, t4, array_p #loop
     
     li t4, 14
array_o:
     add t1, a0, t0 # t1 is the address of target index for input
     lbu t2, 0(t1)
     addi t2, t2, -49 #convert into zero-base
     addi t5, t0, -7 
     add t3, s1, t5
     sb t2, 0(t3)
     addi t0, t0, 1 # got the next index offset
     blt t0, t4, array_o

     la a0, p
     la a1, o
     jal ra, rank

after_rank:
     la a2, p
     la a3, o
     jal ra, unrank

test_done:
     li a7, 10
     ecall

    
rank:
    li t0, 0 #p
    li t1, 0 #o
    li a2, 7 #CUBIES
    li t2, 0 # index i
rank_p_loop:
    li t3, 0 #smaller = 0
    addi t4, t2, 1 #j-i+1
    bge t4, a2, rank_p_next
    add t6, a0, t2
    lbu t6, 0(t6)    #p[i]
    
rank_p_inner:
    add t5, a0, t4
    lbu t5, 0(t5) #p[j]
    bgeu t5, t6, rank_p_next_j
    addi t3, t3, 1

rank_p_next_j:
    addi t4, t4, 1
    blt t4, a2, rank_p_inner
    
    # product = p_rank*(7-i)
    sub t5, a2, t2 
    li t6, 0
    
rank_p_multiply:
    add t6, t6, t0
    addi t5, t5, -1
    bne t5, zero, rank_p_multiply
    add t0, t6, t3 # p = product+smaller
    
rank_p_next:
    addi t2, t2, 1
    blt t2, a2, rank_p_loop

    li t2, 0    # i = 0
    li a2, 6    # i<6

rank_o_loop:
    slli t5, t1, 1            # 2 × o_rank
    add t1, t1, t5            # 3 × o_rank

    add t5, a1, t2
    lbu t5, 0(t5)             # o[i]
    add t1, t1, t5

    addi t2, t2, 1
    blt t2, a2, rank_o_loop

    mv a0, t0                 # Return p
    mv a1, t1                 # Return o , those are not be express as p*ORIEN..+o, return sperately
    ret
    
unrank:
    addi sp, sp, -48 #reserve stack frame for this func
    
    sw ra, 44(sp) 
    sw s2, 40(sp)    #save caller's register
    sw s3, 36(sp)
    sw s4, 32(sp)
    sw s5, 28(sp)
    sw s6, 24(sp)
    sw s7, 20(sp)
    
    mv s2, a0    #keep p 
    mv s3, a1    #keep o
    mv s4, a2    #keep p[] address
    mv s5, a3    #keep o[] address
    
    sb zero, 0(sp)
    li t0, 1
    sb t0, 1(sp)
    li t0, 2
    sb t0, 2(sp)
    li t0, 3
    sb t0, 3(sp)
    li t0, 4
    sb t0, 4(sp)
    li t0, 5
    sb t0, 5(sp)
    li t0, 6
    sb t0, 6(sp)

    
    li s6, 0    #i=0
    li t0, 7    #end i<7
    
unrank_loop:
    bge s6, t0, unrank_loop_done
    mv a0, s2     #load dividend
    la t1, unrank_weights
    slli t2, s6, 2 
    add t1, t1, t2
    lw a1, 0(t1)     #load divisor
    jal ra, unsigned_divmod     #return q in a0, remainder in a1
    mv s2, a1 #p %=f
    
    add t3, s4, s6 #addr of p[i]
    add t4, sp, a0 #addr of available[q]
    lbu t5, 0(t4)
    sb t5, 0(t3)
    
    mv t3, a0 # j = q
    li t0, 7
    sub t5, t0, s6 #count = 7-i    
unrank_inner_loop:
    addi t4, t3, 1    # j + 1
    bge t4, t5, unrank_inner_done

    add t4, sp, t4    # addr of available[j + 1]
    lbu t6, 0(t4)     #available[j + 1]

    add t4, sp, t3    #addr available[j]
    sb  t6, 0(t4)     # available[j] = available[j + 1]

    addi t3, t3, 1    # j += 1
    j unrank_inner_loop

unrank_inner_done:
    #no need of f/ , since we have unrank_weights
    addi s6, s6, 1        # i += 1
    li t0, 7
    blt s6, t0, unrank_loop

unrank_loop_done:
    li s6, 5 #i=6
    li s7, 0 #sum = 0
unrank_loop_o:
    mv a0, s3 # dividend = o
    li a1, 3
    jal ra, unsigned_divmod #reminder in a1
    mv s3, a0 #o /=3
    add t1, s5, s6 #addr of o[i]
    sb a1, 0(t1)
    add s7, s7, a1 #sum = sum + o[i]
    addi s6, s6, -1 #i--
    bge s6, zero, unrank_loop_o
    
    #sum % 3.
    mv a0, s7
    li a1, 3
    jal ra, unsigned_divmod  # a1 = sum % 3

    # (3 - sum % 3) % 3.
    li t1, 3             
    sub a0, t1, a1
    li a1, 3
    jal ra, unsigned_divmod  # a1 = final orientation

    sb a1, 6(s5)            # o[6] = final orientation

    #restore and release stack frame.
    lw s7, 20(sp)
    lw s6, 24(sp)
    lw s5, 28(sp)
    lw s4, 32(sp)
    lw s3, 36(sp)
    lw s2, 40(sp)
    lw ra, 44(sp)
    addi sp, sp, 48
    ret

    
    
unsigned_divmod: #q in a0, remainder in a1
    mv t1, a1                 # divisor
    li t2, 0                  # quotient
    mv t3, a0                 # remainder starts as dividend

unsigned_divmod_loop:
    bltu t3, t1, unsigned_divmod_done
    sub  t3, t3, t1            # remainder -= divisor
    addi t2, t2, 1             # quotient += 1
    j unsigned_divmod_loop

unsigned_divmod_done:
    mv a0, t2                 # Return quotient
    mv a1, t3                 # Return remainder
    ret
    
