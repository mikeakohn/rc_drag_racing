
;; Drag Racing Circuit - Copyright 2010 by Michael Kohn
;; Email: mike@mikekohn.net
;;   Web: http://www.mikekohn.net/
;;
;; Simulate the drag racing christmas tree, detect a fault, and detect who won
;; 

.avr8
.include "tn2313def.inc"

; note: CLKSEL 0100
;  cycles  sample rate   @8 MHz
;    400   10.0kHz * 2
;  A440 = 45.45 interrupts / 2
;  A880 = 22.72 interrupts / 2
;
;  Sound 0.5 Sec Duration = 10000 (0x2710)
;  Sound 1.0 Sec Duration = 20000 (0x4e20)

.equ A440 = 24
.equ A880 = 12
.equ THREE_SEC_LO = 0x60
.equ THREE_SEC_HI = 0xea
.equ ONE_SEC_LO = 0x20
.equ ONE_SEC_HI = 0x4e
.equ HALF_SEC_LO = 0x10
.equ HALF_SEC_HI = 0x27

; r0  = 0
; r1  = 1
; r2  = sound wave counter
; r3  = sound wave amplitude (0 or 1)
; r15 = 255
; r17 = temp outside of interrupt
; r20 = current sound frequency
; r21 = sound duration lo
; r22 = sound duration hi
; r23 = state (0=idle, 1=READY, 2=yellow/440, 3=yellow/440, 4=yellow/440, 5=green/880), 6=done
; r24 = state duration lo
; r25 = state duration hi
;
; PORTB = Y, Y, Y, G, Y, Y, Y, G
;   PD0 = Speaker
;   PD1 = Red Left
;   PA1 = Red Right
;   PD3 = Start buton  (Input)
;   PD4 = Fault Left   (Input)
;   PD5 = Fault Right  (Input)

.org 0x000
  rjmp start
.org 0x008
  rjmp the_great_interrupt   ; might as well rjmp, RJMP!

;; FIXME erase this thing.. it's dumb
.org 0x00a
  reti

;; FIXME - erase this padding.. it's dumb
;;.org 0x020

start:
  ;; I'm busy.  Don't interrupt me!
  cli

  ;; Set up stack ptr
  ;ldi r17, RAMEND>>8
  ;out SPH, r17
  ldi r17, RAMEND&255
  out SPL, r17

  ;; r0 = 0, r1 = 1, r15 = 255
  eor r0, r0
  eor r1, r1
  inc r1
  eor r15, r15
  dec r15

  ; init variables
  ldi r20, A440
  clr r23                   ; idle state

  ;; Set up PORTB
  ;ldi r17, 255
  out DDRB, r15             ; PORTB is output for LED's (sourcing)
  out PORTB, r0             ; turn off all of port B for fun
  ldi r17, 1|2|4
  out DDRD, r17             ; PD0-PD2 are output.  PD0 is the speaker
  ldi r17, 8
  out PORTD, r17            ; Outputs off, input has internal pullup
  ldi r17, 2
  out DDRA, r17

  ;; Set up TIMER1
  ldi r17, (399>>8)
  out OCR1AH, r17
  ldi r17, (399&0xff)            ; compare to 400 clocks
  out OCR1AL, r17

  ldi r17, (1<<OCIE1A)
  out TIMSK, r17                 ; enable interrupt comare A 
  out TCCR1C, r0
  out TCCR1A, r0                 ; normal counting (0xffff is top, count up)
  ldi r17, (1<<CS10)|(1<<WGM12)  ; CTC OCR1A  Clear Timer on Compare
  out TCCR1B, r17                ; prescale = 1 from clock source

  ; Fine, I can be interrupted now
  sei

main:
  tst r23
  breq test_button

  cpi r23, 1
  brne tree_dropping
  sbic PIND, PD4         ;; left side
  sbi PORTD, PD1
  sbis PIND, PD4
  cbi PORTD, PD1

  sbic PIND, PD5         ;; right side
  sbi PORTA, PA1
  sbis PIND, PD5
  cbi PORTA, PA1
  rjmp main

tree_dropping:
  cpi r23, 5
  breq main
  in r17, PIND
  sbrc r17, PD4
  sbi PORTD, PD1
  sbrc r17, PD5
  sbi PORTA, PA1
  rjmp main

test_button:
  sbic PIND, PD3
  rjmp main

  ;tst r23
  ;brne main                 ; D-BOUNCE

  out PORTB, r0
  ldi r24, THREE_SEC_LO
  ldi r25, THREE_SEC_HI 

  ldi r23, 1
  rjmp main

;; It's the GREAT interrupt!
the_great_interrupt:
  in r7, SREG

;;;; DEBUG
  ;inc r30
  ;sbrs r30, 0
  ;sbi PORTD, PD0
  ;sbrc r30, 0
  ;cbi PORTD, PD0
  ;rjmp exit_interrupt
;;;; DEBUG

  tst r23
  brne process
  out SREG, r7
  reti

process:
  cpi r23, 1
  brne not_state_1

  ;; Hackish.. to add the 3 sec delay since I don't feel like screwing
  ;; with this and making cleaner code

  dec r24                   ; dec state duration, change state on 0
  cpi r24, 0xff
  brne state_duration_no_overflow
  dec r25
  cpi r25, 0xff
  brne state_duration_no_overflow

  ;; Ready to go to state 2.. 
  inc r23
  ldi r26, 0x88
  ldi r24, ONE_SEC_LO
  ldi r25, ONE_SEC_HI 
  ldi r21, HALF_SEC_LO
  ldi r22, HALF_SEC_HI
  ldi r20, A440
  mov r2, r20
  cbi PORTD, PD1
  cbi PORTA, PA1
  out PORTB, r26
  out SREG, r7
  reti

not_state_1:

  tst r20
  breq no_sound
  dec r2
  brne sound_not_zero
  mov r2, r20
  inc r3
  sbrs r3, 0
  sbi PORTD, PD0
  sbrc r3, 0
  cbi PORTD, PD0
sound_not_zero:
  dec r21                   ; dec duration and stop sound when 0
  cpi r21, 0xff
  brne sound_duration_no_overflow
  dec r22
  cpi r22, 0xff
  brne sound_duration_no_overflow
  clr r20
sound_duration_no_overflow:

no_sound:

  dec r24                   ; dec state duration, change state on 0
  cpi r24, 0xff
  brne state_duration_no_overflow
  dec r25
  cpi r25, 0xff
  brne state_duration_no_overflow
  inc r23                   ; state duration is 0, inc state
 
  cpi r23, 6                ; done state.. set to 0.. sit
  brne not_state_6
  clr r23
  cbi PORTD, PD0
  rjmp exit_interrupt

not_state_6:
  lsr r26
  out PORTB, r26

  cpi r23, 5                ; "GO!" state, set to green
  brne not_state_5
  ldi r24, ONE_SEC_LO
  ldi r25, ONE_SEC_HI 
  ldi r21, ONE_SEC_LO
  ldi r22, ONE_SEC_HI
  ldi r20, A880
  mov r2, r20
  rjmp exit_interrupt

not_state_5:
  ldi r24, ONE_SEC_LO
  ldi r25, ONE_SEC_HI 
  ldi r21, HALF_SEC_LO
  ldi r22, HALF_SEC_HI
  ldi r20, A440
  mov r2, r20

state_duration_no_overflow:

exit_interrupt:
  out SREG, r7
  reti

signature:
.db "Drag Racing Track - Copyright 2010 - Michael Kohn - Version 0.02",0,0




