/* Automatically generated nanopb constant definitions */
/* Generated by nanopb-0.3.9-dev at Mon Apr 30 14:47:22 2018. */

#include "fk-radio.pb.h"

/* @@protoc_insertion_point(includes) */
#if PB_PROTO_HEADER_VERSION != 30
#error Regenerate this file with the current version of nanopb generator.
#endif



const pb_field_t fk_radio_RadioPacket_fields[6] = {
    PB_FIELD(  1, UENUM   , SINGULAR, STATIC  , FIRST, fk_radio_RadioPacket, kind, kind, 0),
    PB_FIELD(  2, BYTES   , SINGULAR, CALLBACK, OTHER, fk_radio_RadioPacket, nodeId, kind, 0),
    PB_FIELD(  3, INT32   , SINGULAR, STATIC  , OTHER, fk_radio_RadioPacket, address, nodeId, 0),
    PB_FIELD(  4, INT32   , SINGULAR, STATIC  , OTHER, fk_radio_RadioPacket, size, address, 0),
    PB_FIELD(  5, BYTES   , SINGULAR, CALLBACK, OTHER, fk_radio_RadioPacket, data, size, 0),
    PB_LAST_FIELD
};



/* @@protoc_insertion_point(eof) */
