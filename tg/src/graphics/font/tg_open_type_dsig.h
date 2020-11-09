#ifndef TG_OPEN_TYPE_DSIG_H
#define TG_OPEN_TYPE_DSIG_H

#include "graphics/font/tg_open_type_types.h"

typedef struct tg_open_type__signature_block_format_1
{
	u16    reserved1;        // reserved1: Reserved for future use; set to zero.
	u16    reserved2;        // reserved2: Reserved for future use; set to zero.
	u32    signature_length; // signatureLength: Length (in bytes) of the PKCS#7 packet in the signature field.
	u8     p_signature[0];   // signature[signatureLength]: PKCS#7 packet
} tg_open_type__signature_block_format_1;

typedef struct tg_open_type__signature_record
{
	u32    format; // format: Format of the signature
	u32    length; // length: Length of signature in bytes
	u32    offset; // offset: Offset to the signature block from the beginning of the table
} tg_open_type__signature_record;

typedef struct tg_open_type__dsig_header
{
	u32                               version;                // version: Version number of the DSIG table (0x00000001)
	u16                               num_signatures;         // numSignatures: Number of signatures in the table
	u16                               flags;                  // flags: permission flags Bit 0: cannot be resigned Bits 1-7: Reserved (Set to 0)
	tg_open_type__signature_record    p_signature_records[0]; // signatureRecords[numSignatures]: Array of signature records
} tg_open_type__dsig_header;

#endif
