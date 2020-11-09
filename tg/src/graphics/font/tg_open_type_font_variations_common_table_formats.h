#ifndef TG_OPEN_TYPE_FONT_VARIATIONS_COMMON_TABLE_FORMATS
#define TG_OPEN_TYPE_FONT_VARIATIONS_COMMON_TABLE_FORMATS

#include "graphics/font/tg_open_type_types.h"

typedef struct tg_open_type__delta_set__record
{
	i16*    p_delta_data_i16; // DeltaData[shortDeltaCount + regionIndexCount]: Variation delta values.
	i8*     p_delta_data_i8;  // DeltaData[shortDeltaCount + regionIndexCount]: Variation delta values.
} tg_open_type__delta_set__record;

typedef struct tg_open_type__item_variation_data__subtable
{
	u16                                 item_count;         // itemCount: The number of delta sets for distinct items.
	u16                                 short_delta_count;  // shortDeltaCount: The number of deltas in each delta set that use a 16 - bit representation.Must be less than or equal to regionIndexCount.
	u16                                 region_index_count; // regionIndexCount: The number of variation regions referenced.
	u16*                                p_region_indexes;   // regionIndexes[regionIndexCount]: Array of indices into the variation region list for the regions referenced by this item variation data table.
	tg_open_type__delta_set__record*    p_delta_sets;       // deltaSets[itemCount]: Delta - set rows.
} tg_open_type__item_variation_data__subtable;

typedef struct tg_open_type__item_variation_store__table
{
	u16            format;                        // format: Format — set to 1
	tg_offset32    variation_region_list_offset;  // variationRegionListOffset: Offset in bytes from the start of the item variation store to the variation region list.
	u16            item_variation_data_count;     // itemVariationDataCount: The number of item variation data subtables.
	tg_offset32    p_item_variation_data_offsets; // itemVariationDataOffsets[itemVariationDataCount]: Offsets in bytes from the start of the item variation store to each item variation data subtable.
} tg_open_type__item_variation_store__table;

typedef struct tg_open_type__region_axis_coordinates__record
{
	tg_f2dot14    start_coord; // startCoord: The region start coordinate value for the current axis.
	tg_f2dot14    peak_coord;  // peakCoord: The region peak coordinate value for the current axis.
	tg_f2dot14    end_coord;   // endCoord: The region end coordinate value for the current axis.
} tg_open_type__region_axis_coordinates__record;

typedef struct tg_open_type__variation_region__record
{
	tg_open_type__region_axis_coordinates__record    p_region_axes[0]; // regionAxes[axisCount]: Array of region axis coordinates records, in the order of axes given in the 'fvar' table.
} tg_open_type__variation_region__record;

typedef struct tg_open_type__variation_region_list
{
	u16                                        axis_count;          // axisCount: The number of variation axes for this font. This must be the same number as axisCount in the 'fvar' table.
	u16                                        region_count;        // regionCount: The number of variation region tables in the variation region list.
	tg_open_type__variation_region__record*    p_variation_regions; // variationRegions[regionCount]: Array of variation regions.
} tg_open_type__variation_region_list;

typedef enum tg_open_type__packet_delta__flags
{
	TG_OPEN_TYPE__PACKET_DELTA__DELTAS_ARE_ZERO         = 0x80, // DELTAS_ARE_ZERO:  Flag indicating that this run contains no data(no explicit delta values are stored), and that all of the deltas for this run are zero.
	TG_OPEN_TYPE__PACKET_DELTA__DELTAS_ARE_WORDS        = 0x40, // DELTAS_ARE_WORDS: Flag indicating the data type for delta values in the run. If set, the run contains 16 - bit signed deltas(int16); if clear, the run contains 8 - bit signed deltas(int8).
	TG_OPEN_TYPE__PACKET_DELTA__DELTA_RUN_COUNT_MASK    = 0x3F  // DELTA_RUN_COUNT_MASK: Mask for the low 6 bits to provide the number of delta values in the run, minus one.
} tg_open_type__packet_delta__flags;

typedef enum tg_open_type__packed_point_number__flags
{
	TG_OPEN_TYPE__PACKET_POINT_NUMBER__POINTS_ARE_WORDS        = 0x80, // POINTS_ARE_WORDS: Flag indicating the data type used for point numbers in this run. If set, the point numbers are stored as unsigned 16 - bit values(uint16); if clear, the point numbers are stored as unsigned bytes(uint8).
	TG_OPEN_TYPE__PACKET_POINT_NUMBER__POINT_RUN_COUNT_MASK    = 0x7F  // POINT_RUN_COUNT_MASK: Mask for the low 7 bits of the control byte to give the number of point number elements, minus 1.
} tg_open_type__packed_point_number__flags;

typedef enum tg_open_type__tuple_index__format
{
	TG_OPEN_TYPE__TUPLE_INDEX__EMBEDDED_PEAK_TUPLE      = 0x8000, // EMBEDDED_PEAK_TUPLE: Flag indicating that this tuple variation header includes an embedded peak tuple record, immediately after the tupleIndex field. If set, the low 12 bits of the tupleIndex value are ignored. Note that this must always be set within the 'cvar' table.
	TG_OPEN_TYPE__TUPLE_INDEX__INTERMEDIATE_REGION      = 0x4000, // INTERMEDIATE_REGION: Flag indicating that this tuple variation table applies to an intermediate region within the variation space. If set, the header includes the two intermediate-region, start and end tuple records, immediately after the peak tuple record (if present).
	TG_OPEN_TYPE__TUPLE_INDEX__PRIVATE_POINT_NUMBERS    = 0x2000, // PRIVATE_POINT_NUMBERS: Flag indicating that the serialized data for this tuple variation table includes packed “point” number data. If set, this tuple variation table uses that number data; if clear, this tuple variation table uses shared number data found at the start of the serialized data for this glyph variation data or 'cvar' table.
	TG_OPEN_TYPE__TUPLE_INDEX__RESERVED                 = 0x1000, // Reserved: Reserved for future use — set to 0.
	TG_OPEN_TYPE__TUPLE_INDEX__TUPLE_INDEX_MASK         = 0x0FFF  // TUPLE_INDEX_MASK: Mask for the low 12 bits to give the shared tuple records index.
} tg_open_type__tuple_index__format;

typedef struct tg_open_type__tuple_variation_header
{
	u16                   variation_data_size;      // variationDataSize: The size in bytes of the serialized data for this tuple variation table.
	u16                   tuple_index;              // tupleIndex: A packed field. The high 4 bits are flags(see below). The low 12 bits are an index into a shared tuple records array.
	// TODO: what type is this?
	// tg_open_type_tuple    peak_tuple;               // peakTuple: Peak tuple record for this tuple variation table — optional, determined by flags in the tupleIndex value. Note that this must always be included in the 'cvar' table.
	// tg_open_type_tuple    intermediate_start_tuple; // intermediateStartTuple: Intermediate start tuple record for this tuple variation table — optional, determined by flags in the tupleIndex value.
	// tg_open_type_tuple    intermediate_end_tuple;   // intermediateEndTuple: Intermediate end tuple record for this tuple variation table — optional, determined by flags in the tupleIndex value.
} tg_open_type__tuple_variation_header;

typedef enum tg_open_type__tuple_variation_count__flags
{
	TG_OPEN_TYPE__TUPLE_VARIATION_COUNT__SHARED_POINT_NUMBERS    = 0x8000, // SHARED_POINT_NUMBERS: Flag indicating that some or all tuple variation tables reference a shared set of “point” numbers. These shared numbers are represented as packed point number data at the start of the serialized data.
	TG_OPEN_TYPE__TUPLE_VARIATION_COUNT__RESERVED                = 0x7000, // Reserved: Reserved for future use — set to 0.
	TG_OPEN_TYPE__TUPLE_VARIATION_COUNT__COUNT_MASK              = 0x0FFF  // COUNT_MASK: Mask for the low bits to give the number of tuple variation tables.
} tg_open_type__tuple_variation_count__flags;

typedef struct tg_open_type__cvar__table__header
{
	u16                                     major_version;                // majorVersion: Major version number of the 'cvar' table — set to 1.
	u16                                     minor_version;                // minorVersion: Minor version number of the 'cvar' table — set to 0.
	u16                                     tuple_variation_count;        // tupleVariationCount: A packed field. The high 4 bits are flags(see below), and the low 12 bits are the number of tuple variation tables. The count can be any number between 1 and 4095.
	tg_offset16                             data_offset;                  // dataOffset: Offset from the start of the 'cvar' table to the serialized data.
	tg_open_type__tuple_variation_header    p_tuple_variation_headers[0]; // tupleVariationHeaders[tupleVariationCount]: Array of tuple variation headers.
} tg_open_type__cvar__table__header;

typedef struct tg_open_type__glyph_variation_data__header
{
	u16                                     tuple_variation_count;        // tupleVariationCount: A packed field. The high 4 bits are flags (see below), and the low 12 bits are the number of tuple variation tables for this glyph. The count can be any number between 1 and 4095.
	tg_offset16                             data_offset;                  // dataOffset: Offset from the start of the GlyphVariationData table to the serialized data.
	tg_open_type__tuple_variation_header    p_tuple_variation_headers[0]; //tupleVariationHeaders[tupleVariationCount]: Array of tuple variation headers.
} tg_open_type__glyph_variation_data__header;

typedef struct tg_open_type__tuple__record
{
	tg_f2dot14    p_coordinates[0]; // coordinates[axisCount]: Coordinate array specifying a position within the font’s variation space. The number of elements must match the axisCount specified in the 'fvar' table.
} tg_open_type__tuple__record;

#endif
