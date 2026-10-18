.equ RENDER, 0

.data
stack_storage:
    .zero 128
stack_end:

cube_input:
    .asciz "21345671111111"
p:
    .zero 7
o:
    .zero 7
solution:
    .zero 11
.align 4
unrank_weights:
    .word 720, 120, 24, 6, 2, 1, 1
orientation_weights:
    .half 243, 81, 27, 9, 3, 1
    
move_r:   .asciz "R"
move_r2:  .asciz "R2"
move_ri:  .asciz "R'"
move_b:   .asciz "B"
move_b2:  .asciz "B2"
move_bi:  .asciz "B'"
move_d:   .asciz "D"
move_d2:  .asciz "D2"
move_di:  .asciz "D'"

.align 4
move_names:
    .word move_r, move_r2, move_ri
    .word move_b, move_b2, move_bi
    .word move_d, move_d2, move_di
cube_sources:
    .byte 1, 4, 2, 0, 3, 5, 6     #R
    .byte 0, 1, 2, 4, 5, 6, 3     #B
    .byte 0, 2, 5, 3, 1, 4, 6     #D

cube_twists:
    .byte 1, 2, 0, 2, 1, 0, 0     #R
    .byte 0, 0, 0, 1, 2, 1, 2     #B
    .byte 0, 0, 0, 0, 0, 0, 0     #D
.align 4
.if RENDER
led_colors:
    .word 0x00FFFFFF    # ID 0:white (U)
    .word 0x00FFA500    # ID 1:orange (L)
    .word 0x0000FF00    # ID 2:green (F)
    .word 0x00FF0000    # ID 3:red (R)
    .word 0x000000FF    # ID 4:blue (B)
    .word 0x00FFFF00    # ID 5:yellow (D)
corner_colors:
    .byte 0, 2, 1      #cubie 0: U F L — white, green, orange
    .byte 0, 3, 2      #cubie 1: U R F — white, red, green
    .byte 5, 2, 3      #cubie 2: D F R — yellow, green, red
    .byte 5, 1, 2      #cubie 3: D L F — yellow, orange, green
    .byte 0, 4, 3      #cubie 4: U B R — white, blue, red
    .byte 5, 3, 4      #cubie 5: D R B — yellow, red, blue
    .byte 5, 4, 1      #cubie 6: D B L — yellow, blue, orange
    .byte 0, 1, 4      #cubie 7: U L B — white, orange, blue

corner_screen: #xy pair for each slot (face)
    .byte  9, 3,   9, 7,   4, 7     # 0: U, F, L
    .byte 13, 3,  18, 7,  13, 7     # 1: U, R, F
    .byte 13,14,  13,10,  18,10     # 2: D, F, R
    .byte  9,14,   4,10,   9,10     # 3: D, L, F
    .byte 13, 0,  27, 7,  22, 7     # 4: U, B, R
    .byte 13,17,  22,10,  27,10     # 5: D, R, B
    .byte  9,17,  31,10,   0,10     # 6: D, B, L
    .byte  9, 0,   0, 7,  31, 7     # 7: U, L, B
.endif    
.text
.globl main
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

.if RENDER
    la a0, p
    la a1, o
    jal ra, render_cube
    jal ra, render_delay
.endif
     la a0, p
     la a1, o
     jal ra, rank
     mv s0, a0 #starting p, o
     mv s1, a1
     la a2, solution
     jal ra, solve_ida
    
    li t0, -1
    beq a0, t0, search_failed
    
    mv s5, a0    #lenght
    li s6, 0 #i=0
    la s7, solution
print_move_loop:
    bge s6, s5, replay_finished
    
    beq s6, zero, print_move_id
    li a0, 32
    li a7, 11 #ripes printChar
    ecall
    
print_move_id:
    add t0, s7, s6 #addr of sol
    lbu t1, 0(t0)
    
    la t0, move_names
    slli t1, t1, 2 #offset 
    add t0, t0, t1
    lw a0, 0(t0)
    li a7, 4 #ripes printint
    ecall
    
    
    add t0, s7, s6
    lbu a2, 0(t0)

    mv a0, s0
    mv a1, s1
    jal ra, replay_move

    #save new rank 
    mv s0, a0
    mv s1, a1

.if RENDER
    #decode updated rank to array
    mv a0, s0
    mv a1, s1
    la a2, p
    la a3, o
    jal ra, unrank

    #draw arrays
    la a0, p
    la a1, o
    jal ra, render_cube
    jal ra, render_delay
.endif

    addi s6, s6, 1
    j print_move_loop
    
replay_finished:
    or t0, s0, s1  #solve when both rank =0
    bne t0, zero, replay_failed
    j test_done

replay_failed:
    li a0, -2   #replay failed
    li a7, 1
    ecall
    j test_done
    
search_failed:
    li a0, -1  #print -1
    li a7, 1
    ecall
    j test_done
    
test_done:
     li a7, 10
     ecall
replay_move:
    addi sp, sp, -32
    sw ra, 28(sp)
    sw s0, 24(sp)
    sw s1, 20(sp)
    sw s2, 16(sp)
    sw s3, 12(sp)

    mv s0, a0 # p rank
    mv s1, a1 # o rank

    # face: R=0, B=1, D=2
    li s2, 0
    li t0, 3
    bltu a2, t0, replay_face_ready

    li s2, 1
    li t0, 6
    bltu a2, t0, replay_face_ready

    li s2, 2

replay_face_ready:
    # turns=moveID-face*3+1
    slli t0, s2, 1
    add t0, t0, s2
    sub s3, a2, t0
    addi s3, s3, 1

replay_quarter_loop:
    mv a0, s0
    mv a1, s1
    mv a2, s2
    jal ra, move_quarter

    mv s0, a0
    mv s1, a1

    addi s3, s3, -1
    bne s3, zero, replay_quarter_loop

    #return new p o
    mv a0, s0
    mv a1, s1

    lw ra, 28(sp)
    lw s0, 24(sp)
    lw s1, 20(sp)
    lw s2, 16(sp)
    lw s3, 12(sp)
    addi sp, sp, 32
    ret
    
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
    li s6, 0 #i=0
    li s7, 0 #sum 
    la t0, orientation_weights
unrank_loop_o:
    lhu t1, 0(t0) # weight
    slli t2, t1, 1 # twice the weight
    li t3, 0 # digit
    bltu s3, t2, unrank_o_try_one
    sub s3, s3, t2
    li t3, 2
    j unrank_o_store
unrank_o_try_one:
    bltu s3, t1, unrank_o_store
    sub s3, s3, t1
    li t3, 1
unrank_o_store:
    add t4, s5, s6
    sb t3, 0(t4) # o[i] = digit
    add s7, s7, t3
    li t5, 3
    bltu s7, t5, unrank_o_next
    addi s7, s7, -3
unrank_o_next:
    addi t0, t0, 2
    addi s6, s6, 1
    li t5, 6
    blt s6, t5, unrank_loop_o

    li t3, 0
    beq s7, zero, unrank_o_last
    li t3, 3
    sub t3, t3, s7
unrank_o_last:
    sb t3, 6(s5) # final o

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
    
#inpute: a0 p rank, a1 0 rank, a2 face
#output: a0 new p, a1 new o
move_quarter: 
    la t0, permutation_moves
    la t1, orientation_moves
    mv t2, a2
    li t3, 10080              #5040 permutaition per face, each occupied 2 byte
    li t4, 1458               #720 orientation per face, each 2 btye
move_select_row:
    beq t2, zero, move_read
    add t0, t0, t3
    add t1, t1, t4
    addi t2, t2, -1
    j move_select_row
move_read:
    slli t2, a0, 1 # rank -> index
    add t0, t0, t2
    slli t2, a1, 1
    add t1, t1, t2
    lhu a0, 0(t0)
    lhu a1, 0(t1)
    ret

#input: a0 p , a1 o
heuristic:
    la t0, permutation_distances
    la t1, orientation_distances
    add t0, t0, a0
    add t1, t1, a1
    lbu a0, 0(t0)
    lbu a1, 0(t1)
    bge a0, a1, heuristic_ret
    mv a0, a1
heuristic_ret:
    ret
    
    
#input: a0 p, a1 o, a2 addr of solution array
solve_ida:
    addi sp, sp, -32
    addi sp, sp, -96 # 12 frames x 8b each frame (0:p, 2:o, 4: next move, 5: previous face, 6: incomming move, 7: h
    
    sw ra, 116(sp)
    sw s0, 112(sp)
    sw s1, 108(sp)
    sw s2, 104(sp)
    sw s3, 100(sp)
    sw s4, 96(sp)
    
    
    sh a0, 0(sp)
    sh a1, 2(sp) #save start p, o
    jal ra, heuristic
    sb a0, 7(sp) # save start h
    li t0, 3
    sb t0, 5(sp)
    li t0, 0
    sb t0, 4(sp)
    sb t0, 6(sp) #init other field
    mv s1, a0 # s1 as bound (threshold)
    
    mv s3, a2 #addr solve array
solve_ida_loop:
    li s2, -1 # next bound
    li s0, 0 #depth
    mv s4, sp
    sb zero, 4(s4)
    li t0, 11 #MAX_DEPTH
    blt t0, s1, ida_return_neg
    
solve_ida_inner_loop:

    lbu t1, 7(s4)    # h
    add t1, s0, t1  # f = depth + h
    blt s1, t1, check_next_bound
    
    lhu t1, 0(s4) # p >=0
    lhu t2, 2(s4) # o >=0
    add t1, t1, t2 # p+o>=0 (usigned)
    beq t1, zero, ida_done # if(p==0&o==0) load solution
    
    li t1, 11 #MAX_DEPTH
    beq s0, t1, next_inner
    
after_new_frame:
    lbu t0, 4(s4)    
    li t1, 9 #MOVES
    beq t0, t1, next_inner
    
    li   t1, 0    # face = 0(R)
    li   t2, 3
    bltu t0, t2, face_done  #move<3:keep 0
    
    li   t1, 1    # face=1(B)
    li   t2, 6
    bltu t0, t2, face_done  #move<6: keep 1

    li   t1, 2    # face= 2(D)

face_done:
#t0=move
#t1=face index
    lbu t2, 5(s4) #previous face
    beq t1, t2, apply_next_move
    addi t2, t0, 1
    sb t2, 4(s4) #cur->next_move++
    slli t2, t1, 1 #face<<1
    add t2, t2, t1 #+face
    sub t2, t0, t2 #move - ((face << 1) + face) , t2 = remainder
    addi t3, s4, 8  # next frame (child) addr
    
    beq t2, zero, use_current_rank
    lhu a0, 0(t3) # p=child->p
    lhu a1, 2(t3) # o =child->o
    j rank_applied
    
use_current_rank:
    lhu a0, 0(s4)
    lhu a1, 2(s4)
    
rank_applied:
    mv a2, t1
    mv a3, t3
    mv a4, t0
    jal ra, move_quarter
    sh a0, 0(a3) # save child p
    sh a1, 2(a3) #o
    jal ra, heuristic
    sb a2, 5(a3) #face
    sb a4, 6(a3) #move
    sb a0, 7(a3) # h
    sb zero, 4(a3)
    
    addi s0, s0, 1
    addi s4, s4, 8
    j solve_ida_inner_loop
    
check_next_bound:
    bgeu t1, s2, next_inner #f>= next_bound -> go next
    mv s2, t1
next_inner:
    beq s0, zero, ida_inner_done
    addi s0, s0, -1
    addi s4, s4, -8
    j after_new_frame    

apply_next_move:
    addi t2, t0, 3 #move+3
    sb t2, 4(s4)
    j after_new_frame

ida_inner_done:
    li t0, -1
    beq s2, t0, ida_return_neg
    mv s1, s2
    j solve_ida_loop
    
ida_done: 
    # load solution
    li t0, 1 #i=1
    
load_sol_loop: #s3 addr
    addi t1, t0, -1
    blt s0, t0, load_sol_done
    add t2, s3, t1
    slli t3, t0, 3
    add t3, sp, t3
    lbu t3, 6(t3)
    sb t3, 0(t2) 
    addi t0, t0, 1 #i++
    j load_sol_loop
    
load_sol_done:
    mv a0, s0
    j clean_up
ida_return_neg:
    li a0, -1
    
clean_up:
    lw ra, 116(sp)
    lw s0, 112(sp)
    lw s1, 108(sp)
    lw s2, 104(sp)
    lw s3, 100(sp)
    lw s4, 96(sp)
    addi sp, sp, 128
    ret

.if RENDER
draw_sticker:
    li t0, LED_MATRIX_0_BASE
    li t1, LED_MATRIX_0_WIDTH
    slli t1, t1, 2
    mv t2, a1
draw_get_row:
    beq t2, zero, draw_get_column
    add t0, t0, t1
    addi t2, t2, -1
    j draw_get_row
    
draw_get_column:
    slli t2, a0, 2 #x * 4 bytes
    add t0, t0, t2
    li t2, 3
draw_sticker_row:
    sw a2, 0(t0)
    sw a2, 4(t0)
    sw a2, 8(t0)
    sw a2, 12(t0)             

    add t0, t0, t1    # next row
    addi t2, t2, -1
    bne t2, zero, draw_sticker_row
    ret
    
render_cube:
    addi sp, sp, -32
    sw ra, 28(sp)
    sw s0, 24(sp)
    sw s1, 20(sp)
    sw s2, 16(sp)
    sw s3, 12(sp)
    sw s4, 8(sp)
    sw s5, 4(sp)

    mv s0, a0           #p addr 
    mv s1, a1           #o addr
    li s2, 0  #position=0

get_position:
    li s4, 0 #cubie
    li s5, 0 #orientaion
    beq s2, zero, position_done
    
    addi t0, s2, -1 #index = pos -1
    add t1, s0, t0
    lbu s4, 0(t1)
    addi s4, s4, 1 #cubie=p[index] +1
    
    add t1, s1, t0
    lbu s5, 0(t1) #orientation = o[index]
    
position_done:
    li s3, 0   #slot= 0
    
get_color_slot:
    #colorslot = slot+orient ) %3
    add t0, s3, s5
    li t1, 3
    bltu t0, t1, color_slot_done
    addi t0, t0, -3
    
color_slot_done:
    #offset = cubie*3+color_slot
    slli t1, s4, 1 
    add t1, t1, s4 #cubie*3
    add t1, t1, t0
    
    la t2, corner_colors
    add t2, t2, t1
    lbu t0, 0(t2)  #colorID
    
    #4b for each entry
    slli t0, t0, 2
    la t1, led_colors
    add t1, t1, t0
    lw a2, 0(t1) #RGB
    
    #offset for corner_screen = pos*6+slot*2
    slli t0, s2, 2 
    slli t1, s2, 1 
    add t0, t0, t1 #position*6

    slli t1, s3, 1 #slot* 2
    add t0, t0, t1
    
    
    la t1, corner_screen
    add t1, t1, t0
    lbu a0, 0(t1) #x
    lbu a1, 1(t1) #y
    
    jal ra, draw_sticker
    
    #next sticker for htis pos
    addi s3, s3, 1
    li t0, 3
    blt s3, t0, get_color_slot
    
    #next pos
    addi s2, s2, 1
    li t0, 8
    blt s2, t0, get_position
    
render_done:
    lw ra, 28(sp)
    lw s0, 24(sp)
    lw s1, 20(sp)
    lw s2, 16(sp)
    lw s3, 12(sp)
    lw s4, 8(sp)
    lw s5, 4(sp)
    addi sp, sp, 32
    ret
render_delay:
    li t0, 10000000 
render_delay_loop:
    addi t0, t0, -1
    bne t0, zero, render_delay_loop
    ret
.endif
