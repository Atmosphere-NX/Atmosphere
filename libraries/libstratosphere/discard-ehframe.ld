SECTIONS
{
	/* Discard .eh_frame section */
	/DISCARD/ : { *(.group .comment .note .interp)
        EXCLUDE_FILE(*crtbegin.o) *(.eh_frame_hdr .eh_frame)
    }
}
