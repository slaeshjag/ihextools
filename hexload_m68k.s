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
	move.l #-1, %a0
_loadhex_line:
	bsr get_byte
	cmp.b #58, %d0
	jne _skip
## Load byte count
	bsr decode_pair
	move.w %d0, %d2
## Load low address
	bsr decode_word
	move.w %d0, %a0
## Load record type
	bsr decode_pair
	
	cmp.b #0, %d0
	jeq _read_data
	cmp.b #1, %d0
	jeq _end_of_file
	cmp.b #4, %d0
	jeq _upper_address
	cmp.b #5, %d0
	jeq _entry_point
_skip:
	bsr get_byte
	cmp.b #10, %d0
	jne _skip
	btst #0, %d1
	jeq _loadhex_line
_done:
	eor.l %d0, %d0
	move.l #-1, %d1
	cmp %a1, %d1
	jeq _returned
	jsr (%a1)
_returned:
## Now return back to caller of hexload
	move.l %d2, (%sp)+
	move.l %d1, (%sp)+
	move.l %a1, (%sp)+
	move.l %a0, (%sp)+
	rts

_read_data:
	cmp.b #0, %d2
	jeq _skip
	bsr decode_pair
	sub.b #1, %d2
	move.b %d0, (%a0)+
	jra _read_data
_upper_address:
	bsr decode_word
	swap %d0
	move.w %a0, %d0
	move.l %d0, %a0
	jra _skip
_entry_point:
	bsr decode_word
	swap %d0
	move.l %d0, %a1
	bsr decode_word
	move.w %d0, %a1
	jra _skip
_end_of_file:
	move.b #1, %d1
	jra _skip


decode_word:
	move.l %d2, -(%sp)
	
	bsr decode_pair
	move.l %d0, %d2
	bsr decode_pair
	lsl.w #8, %d2
	or.w %d2, %d0
	
	move.l (%sp)+, %d2
	rts

decode_pair:
	move.l %d2, -(%sp)

	bsr decode_byte
	move.l %d0, %d2
	bsr decode_byte
	lsl.b #4, %d2
	or.b %d2, %d0

	move.l (%sp)+, %d2
	rts

decode_byte:
	bsr get_byte
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
	or.b #0xFF, %d0
	rts
