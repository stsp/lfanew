/*
 * Copyright (c) 2023 TK Chia
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef _LFANEW_NEXGEN_PEHDR_H
#define _LFANEW_NEXGEN_PEHDR_H

#include <lfanew/_version.h>
#include <nexgen/mzendian.h>

typedef struct
  {
    uint_le16_t Machine;
    uint_le16_t NumberOfSections;
    uint_le32_t TimeDateStamp;
    uint_le32_t PointerToSymbolTable;
    uint_le32_t NumberOfSymbols;
    uint_le16_t SizeOfOptionalHeader;
    uint_le16_t Characteristics;
  } pe_img_fil_hdr_t;

#define IMAGE_FILE_MACHINE_AMD64	0x8664
#define IMAGE_FILE_MACHINE_ARM		0x01c0
#define IMAGE_FILE_MACHINE_ARM64	0xaa64
#define IMAGE_FILE_MACHINE_ARMNT	0x01c4
#define IMAGE_FILE_MACHINE_I386		0x014c
#define IMAGE_FILE_MACHINE_THUMB	0x01c2
#define IMAGE_FILE_MACHINE_UNKNOWN	0x0000

#define IMAGE_FILE_RELOCS_STRIPPED	0x0001
#define IMAGE_FILE_EXECUTABLE_IMAGE	0x0002
#define IMAGE_FILE_LARGE_ADDRESS_AWARE	0x0020
#define IMAGE_FILE_32BIT_MACHINE	0x0100
#define IMAGE_FILE_DEBUG_STRIPPED	0x0200
#define IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP 0x0400
#define IMAGE_FILE_NET_RUN_FROM_SWAP	0x0800
#define IMAGE_FILE_SYSTEM		0x1000
#define IMAGE_FILE_DLL			0x2000
#define IMAGE_FILE_UP_SYSTEM_ONLY	0x4000

typedef struct
  {
    uint_le16_t Magic;
    uint8_t MajorLinkerVersion;
    uint8_t MinorLinkerVersion;
    uint_le32_t SizeOfCode;
    uint_le32_t SizeOfInitializedData;
    uint_le32_t SizeOfUninitializedData;
    uint_le32_t AddressOfEntryPoint;
    uint_le32_t BaseOfCode;
    uint8_t __skip[8];
    uint_le32_t SectionAlignment;
    uint_le32_t FileAlignment;
    uint_le16_t MajorOperatingSystemVersion;
    uint_le16_t MinorOperatingSystemVersion;
    uint_le16_t MajorImageVersion;
    uint_le16_t MinorImageVersion;
    uint_le16_t MajorSubsystemVersion;
    uint_le16_t MinorSubsystemVersion;
    uint_le32_t Win32VersionValue;
    uint_le32_t SizeOfImage;
    uint_le32_t SizeOfHeaders;
    uint_le32_t Checksum;
    uint_le16_t Subsystem;
    uint_le16_t DllCharacteristics;
  } pe_img_opt_hdr_common_t;

#define IMAGE_NT_OPTIONAL_HDR32_MAGIC	0x010b
#define IMAGE_NT_OPTIONAL_HDR64_MAGIC	0x020b

typedef struct
  {
    uint_le32_t VirtualAddress;
    uint_le32_t Size;
  } pe_data_dir_t;

#define IMAGE_NUMBEROF_DIRECTORY_ENTRIES 16

typedef struct
  {
    uint8_t __skip1[24];
    uint_le32_t BaseOfData;
    uint_le32_t ImageBase;
    uint8_t __skip2[72 - 32];
    uint_le32_t SizeOfStackReserve;
    uint_le32_t SizeOfStackCommit;
    uint_le32_t SizeOfHeapReserve;
    uint_le32_t SizeOfHeapCommit;
    uint_le32_t LoaderFlags;
    uint_le32_t NumberOfRvasAndSizes;
    pe_data_dir_t DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
  } pe_img_opt_hdr32_t;

typedef struct
  {
    uint8_t __skip1[24];
    uint_le64_t ImageBase;
    uint8_t __skip2[72 - 32];
    uint_le64_t SizeOfStackReserve;
    uint_le64_t SizeOfStackCommit;
    uint_le64_t SizeOfHeapReserve;
    uint_le64_t SizeOfHeapCommit;
    uint_le32_t LoaderFlags;
    uint_le32_t NumberOfRvasAndSizes;
    pe_data_dir_t DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
  } pe_img_opt_hdr64_t;

#define IMAGE_DIRECTORY_ENTRY_EXPORT		 0
#define IMAGE_DIRECTORY_ENTRY_IMPORT		 1
#define IMAGE_DIRECTORY_ENTRY_RESOURCE		 2
#define IMAGE_DIRECTORY_ENTRY_EXCEPTION		 3
#define IMAGE_DIRECTORY_ENTRY_SECURITY		 4
#define IMAGE_DIRECTORY_ENTRY_BASERELOC		 5
#define IMAGE_DIRECTORY_ENTRY_DEBUG		 6
#define IMAGE_DIRECTORY_ENTRY_ARCHITECTURE	 7
#define IMAGE_DIRECTORY_ENTRY_GLOBALPTR		 8
#define IMAGE_DIRECTORY_ENTRY_TLS		 9
#define IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG	10
#define IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT	11
#define IMAGE_DIRECTORY_ENTRY_IAT		12
#define IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT	13
#define IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR	14


typedef union
  {
    pe_img_opt_hdr_common_t h;
    pe_img_opt_hdr32_t h32;
    pe_img_opt_hdr64_t h64;
  } pe_img_opt_hdr_t;

typedef struct
  {
    uint_le32_t Signature;
    pe_img_fil_hdr_t FileHeader;
    pe_img_opt_hdr_t OptionalHeader;
  } pe_hdr_t;

#define IMAGE_SIZEOF_SHORT_NAME	8

typedef struct
  {
    uint8_t name[IMAGE_SIZEOF_SHORT_NAME];
    uint_le32_t VirtualSize;
    uint_le32_t VirtualAddress;
    uint_le32_t SizeOfRawData;
    uint_le32_t PointerToRawData;
    uint_le32_t PointerToRelocations;
    uint_le32_t PointerToLinenumbers;
    uint_le16_t NumberOfRelocations;
    uint_le16_t NumberOfLinenumbers;
    uint_le32_t Characteristics;
  } pe_sect_hdr_t;

#define IMAGE_SCN_CNT_CODE			0x00000020
#define IMAGE_SCN_CNT_INITIALIZED_DATA		0x00000040
#define IMAGE_SCN_CNT_UNINITIALIZED_DATA	0x00000080

#endif
