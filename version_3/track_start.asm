;; R/C Drag Racing Track MSP430
;;
;; Copyright 2019 - By Michael Kohn
;; http://www.mikekohn.net/
;; mike@mikekohn.net
;;
;; Starting track for the MSP430 version of the R/C Drag Racing Track

.msp430
.include "msp430x2xx.inc"

RAM equ 0x0200

;  r4 =
;  r5 = tree state
;  r6 = set in interrupt
;  r7 =
;  r8 =
;  r9 =
; r10 =
; r11 =
; r12 =
; r13 =
; r14 =
; r15 =

  .org 0xc000
start:
  ;; Turn off watchdog
  mov.w #(WDTPW|WDTHOLD), &WDTCTL

  ;; Turn off interrupts
  dint

  ;; Setup stack pointer
  mov.w #0x0400, SP

  ;; Set MCLK to 1 MHz with DCO 
  mov.b #DCO_3, &DCOCTL
  mov.b #RSEL_7, &BCSCTL1
  mov.b #0, &BCSCTL2

  ;; Set SMCLK to 32.768kHz external crystal
  mov.b #XCAP_3, &BCSCTL3

  ;; Setup output pins
  ;; P1.0 = Start button
  ;; P1.1 = RX
  ;; P1.2 = TX
  ;; P1.3 = Audio out
  ;; P1.4 = LED Left
  ;; P1.5 = Laser Input Left
  ;; P1.6 = Laser Input Right
  ;; P1.7 = LED Right
  ;; P2.0 = Tree LED RED Left
  ;; P2.1 = Tree LED Green
  ;; P2.2 = Tree LED Yellow Bottom
  ;; P2.3 = Tree LED Yellow Middle
  ;; P2.4 = Tree LED Yellow Top
  ;; P2.5 = Tree LED RED Right
  mov.b #0x98, &P1DIR
  mov.b #0x01, &P1OUT
  mov.b #0x01, &P1REN
  mov.b #0x06, &P1SEL
  mov.b #0x06, &P1SEL2
  mov.b #0x3f, &P2DIR
  mov.b #0x21, &P2OUT
  ;mov.b #0x00, &P2SEL
  ;mov.b #0x00, &P2SEL2

  ;; Setup Timer A
  mov.w #1100, &TACCR0
  mov.w #TASSEL_2|MC_1, &TACTL ; SMCLK, DIV1, COUNT to TACCR0
  ;mov.w #CCIE, &TACCTL0
  mov.w #0, &TACCTL1

  ;; Setup Timer B
  mov.w #32768, &TBCCR0  ; 32768 ticks = 1 second
  mov.w #TBSSEL_1|MC_1, &TBCTL ; ACLK, DIV1, COUNT to TBCCR0
  mov.w #CCIE, &TBCCTL0
  mov.w #0, &TBCCTL1

  ;; Setup UART (9600 @ 1 MHz)
  mov.b #UCSSEL_2|UCSWRST, &UCA0CTL1
  mov.b #0, &UCA0CTL0
  mov.b #UCBRS_2, &UCA0MCTL
  mov.b #109, &UCA0BR0
  mov.b #0, &UCA0BR1
  bic.b #UCSWRST, &UCA0CTL1

  mov.b #1, r5

  ;; Turn on interrupts
  eint

main:
  bit.b #1, &P1IN
  jnz dont_run_game
  call #run_game
dont_run_game:
  ;; Check for break in left laser.
check_left_laser:
  bit.b #0x20, &P1IN
  jz laser_break_left
  bic.b #0x10, &P1OUT
  ;bic.b #0x01, &P2OUT
  ;; Check for break in right laser.
check_right_laser:
  bit.b #0x40, &P1IN
  jz laser_break_right
  bic.b #0x80, &P1OUT
  ;bic.b #0x20, &P2OUT
  jmp main

laser_break_left:
  bis.b #0x10, &P1OUT
  ;bis.b #0x01, &P2OUT
  jmp check_right_laser

laser_break_right:
  bis.b #0x80, &P1OUT
  ;bis.b #0x20, &P2OUT
  jmp main

reset_game:
  mov.b #0, r5
  ret

run_game:
  ;; Clear all lights
  bic.b #0x90, &P1OUT
  bic.b #0x3f, &P2OUT

  ;call #beep
  ;call #beep_2

  call #send_reset
  call #game_delay
  call #drop_tree
  ret

beep:
  mov.w #CCIE, &TACCTL0
  mov.w #1200, &TACCR0
  mov.w #10000, r8
beep_wait:
  call #check_fault
  dec.w r8
  jnz beep_wait
  mov.w #0, &TACCTL0
  bic.b #0x08, &P1OUT
  ret

beep_2:
  mov.w #CCIE, &TACCTL0
  mov.w #600, &TACCR0
  mov.w #3, r9
beep_2_wait_outer:
  mov.w #65000, r8
beep_2_wait:
  dec.w r8
  jnz beep_2_wait
  dec.w r9
  jnz beep_2_wait_outer
  mov.w #0, &TACCTL0
  bic.b #0x08, &P1OUT
  bic.b #0x08, &P1OUT
  ret

game_delay:
  ;; Delay for 3 seconds
  mov.b #3, r15
game_delay_loop:
  mov.w #0, r6
game_delay_1_sec:
  cmp.w #1, r6
  jne game_delay_1_sec
  dec.b r15
  jnz game_delay_loop
  ret

drop_tree:
  ;; Set tree to top
  mov.b #0x20, r5
  call #check_fault
drop_tree_loop:
  cmp.b #1, r6
  jnz dont_change_tree
  call #change_tree
dont_change_tree:
  cmp.w #0x02, r5
  jnz drop_tree_loop
  ret

change_tree:
  cmp.b #0x20, r5
  jeq change_tree_no_clear
  bic.b r5, &P2OUT
change_tree_no_clear:
  rra.b r5
  bis.b r5, &P2OUT
  mov.b #0, r6
  ;; Make beeping.
  cmp.b #0x02, r5
  jne drop_tree_not_go
  call #send_start
  call #beep_2
  ret
drop_tree_not_go:
  call #beep
change_tree_exit:
  ret

check_fault:
  ;; Check for break in left laser.
  bit.b #0x01, &P2OUT
  jnz check_fault_left_okay
  bit.b #0x20, &P1IN
  jnz check_fault_left_okay
  bis.b #0x01, &P2OUT
  call #send_fault_left
check_fault_left_okay:
  ;; Check for break in right laser.
  bit.b #0x20, &P2OUT
  jnz check_fault_right_okay
  bit.b #0x40, &P1IN
  jnz check_fault_right_okay
  bis.b #0x20, &P2OUT
  call #send_fault_right
check_fault_right_okay:
  ret

; send_reset()
send_reset:
  mov.b #'R', r15
  call #uart_send_char
  ret

; send_start()
send_start:
  mov.b #'S', r15
  call #uart_send_char
  ret

; send_fault_left()
send_fault_left:
  mov.b #'l', r15
  call #uart_send_char
  ret

; send_fault_right()
send_fault_right:
  mov.b #'r', r15
  call #uart_send_char
  ret

; uart_send_char(r15)
uart_send_char:
  bit.b #UCA0TXIFG, &IFG2
  jz uart_send_char
  mov.b r15, &UCA0TXBUF
  ret

timer_interrupt_a:
  xor #0x08, &P1OUT
  reti

timer_interrupt_b:
  mov.b #1, r6
  reti

vectors:
.org 0xfff2
  dw timer_interrupt_a     ; Timer0_A3 TACCR0, CCIFG
.org 0xfffa
  dw timer_interrupt_b     ; Timer1_A3 TACCR0, CCIFG
.org 0xfffe
  dw start                 ; Reset

