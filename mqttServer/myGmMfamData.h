/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   myGmMfamData.h
 * Author: he
 *
 * Created on September 2, 2016, 9:19 AM
 */

#ifndef MYGMMFAMDATA_H
#define MYGMMFAMDATA_H

//32bits actually.
#ifndef DWORD
typedef uint32_t DWORD;
#endif

#include <cstdint>
// =========================================================
// Data structure received from the MFAM unit, through
//  SPI bus
// =========================================================
typedef struct s_MfamSpiPacket {
    // Union of a few different data types, including the
    //    fiducial number.
    uint16_t frameid;  // The FID number as gotten by the macro
                     // GET_FID_COUNT is a SIGNED 16-bit integer,
                     // not unsigned, as might have been guessed from
                     // this declaration of the composite field.

    // Information about system status.
    uint16_t sysstat;

    // These next 4 fields are in this order in order to align the 2 mag data fields
    //   on 32-bit boundaries.  That design decision was made in the MFAM FPGA.
    uint32_t mag1data;
    uint16_t mag1stat;
    uint16_t mag2stat;
    uint32_t mag2data;

    // Auxiliary fields.
    uint16_t auxsenx;
    uint16_t auxseny;
    uint16_t auxsenz;
    uint16_t auxsent;
} MfamSpiPacket;

// -------- Structure to index a packet --------------------
typedef struct s_PacketIndex
{
   uint32_t   uiPacketIndex;
} PacketIndex;

// -------- MFAM data with indexing added.  -----------------
typedef struct s_IndexedSpiPacket {
    PacketIndex    piIndex;
    MfamSpiPacket  spMFAMSpiPacket;
 } IndexedMfamSpiPacket;

typedef struct s_IndexedMfamSpiPacketWithHeader
{
  DWORD                 dwRecordType;
  uint32_t                uRecordSize;
  IndexedMfamSpiPacket  imspData;
} IndexedMfamSpiPacketWithHeader;

#endif /* MYGMMFAMDATA_H */

