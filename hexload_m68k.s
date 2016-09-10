.text
.align 2
.globl loadhex

## Calling conventions:
	## All registers are callee-save, with exception for the return register d0
	
# Internal register map:
# a0 = next write address
# a1 = entry point
# d1 = end of file flag

loadhex:
	move.l %a0, -(%sp)
	move.l %a1, -(%sp)
	move.l %d1, -(%sp)
	move.l %d2, -(%sp)
	eor.l %d1, %d1
	move.l %d1, %a0
	move.l #-1, %a1
_loadhex_line:
	bsr get_byte
	cmp.b #58, %d0
	jne _skip
## Load byte count
	bsr.b decode_pair
	move.b %d0, %d2
## Load low address
	bsr.b decode_word
	move.w %d0, %a0
## Load record type
	bsr.b decode_pair

	# tst compares with 0
	tst.b %d0
	jeq _read_data
	cmp.b #1, %d0
	jeq _end_of_file
	cmp.b #4, %d0
	jeq _upper_address
	cmp.b #5, %d0
	jeq _entry_point
_skip:
	bsr.b get_byte
	cmp.b #10, %d0
	jne _skip
	tst %d1
	jeq _loadhex_line
_done:
	eor.l %d0, %d0
	move.l #-1, %d1
	cmp %a1, %d1
	jeq _returned
	jsr (%a1)
_returned:
## Now return back to caller of hexload
	move.l (%sp)+, %d1
	move.l (%sp)+, %d2
	move.l (%sp)+, %a1
	move.l (%sp)+, %a0
	rts

_read_data:
	tst.b %d2
	jeq _skip
	bsr.b decode_pair
	sub.b #1, %d2
	move.b %d0, (%a0)+
	jra _read_data
_upper_address:
	bsr.b decode_word
	swap %d0
	move.w %a0, %d0
	move.l %d0, %a0
	jra _skip
_entry_point:
	bsr.b decode_word
	swap %d0
	move.l %d0, %a1
	bsr.b decode_word
	move.w %d0, %a1
	jra _skip
_end_of_file:
	not.b %d1
	jra _skip


decode_word:
	move.l %d2, -(%sp)
	
	bsr.b decode_pair
	move.l %d0, %d2
	bsr.b decode_pair
	lsl.w #8, %d2
	or.w %d2, %d0
	
	move.l (%sp)+, %d2
	rts

decode_pair:
	move.l %d2, -(%sp)

	bsr.b decode_byte
	move.l %d0, %d2
	bsr.b decode_byte
	lsl.b #4, %d2
	or.b %d2, %d0

	move.l (%sp)+, %d2
	rts

decode_byte:
	bsr.b get_byte
	cmp.b #57, %d0
	jle _decode_byte_lower
	add.b #-7, %d0
_decode_byte_lower:
	add.b #-48, %d0
	rts

get_byte:
	move.l 1048580, %d0
	btst #1, %d0
	jeq get_byte
	move.l 1048576, %d0
	or.l #0xFF, %d0
	rts
