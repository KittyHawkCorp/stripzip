/**
 * @file
 * StripZIP
 * Sanitize a ZIP file from all horrible timestamps, UID, and GID nonsense.
 *
 * ZIP specification at https://pkware.cachefly.net/webdocs/casestudies/APPNOTE.TXT
 * Additional extended header information available from
 *   ftp://ftp.info-zip.org/pub/infozip/src/zip30.zip ./proginfo/extrafld.txt
 *
 * Copyright (c) 2016, Zee.Aero
 * All rights reserved.
 */

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>

#include "err.h"

const uint32_t FILE_HEADER_SIGNATURE = 0x04034b50;
const uint32_t CENDIR_HEADER_SIGNATURE = 0x02014b50;
const uint32_t EO_CENDIR_HEADER_SIGNATURE = 0x06054b50;

#define GPB_ENCRYPTION_MASK        (0x1 <<  0)
#define GPB_METHOD_6_DETAIL        (0x3 <<  1)
#define GPB_NOT_SEEKABLE           (0x1 <<  3)
#define GPB_METHOD_8_ENH_DEFLATE   (0x1 <<  4)
#define GPB_PATCH_DATA             (0x1 <<  5)
#define GPB_STRONG_ENCRYPTION_MASK (0x1 <<  6)
#define GPB_UT8_ENCODING           (0x1 << 11)
#define GPB_CD_ENCRYPTED_MASK      (0x1 << 13)
const uint16_t GP_BIT_ENC_MARKERS       = GPB_ENCRYPTION_MASK | GPB_STRONG_ENCRYPTION_MASK | GPB_CD_ENCRYPTED_MASK;
const uint16_t GP_BIT_UNKNOWN_FLAG_MASK = ~(GPB_ENCRYPTION_MASK | GPB_METHOD_6_DETAIL | GPB_NOT_SEEKABLE | GPB_METHOD_8_ENH_DEFLATE |
                                            GPB_PATCH_DATA | GPB_STRONG_ENCRYPTION_MASK | GPB_UT8_ENCODING | GPB_CD_ENCRYPTED_MASK);

/** Header ID stripzip will use to replace undesired data.
 *  XXX: I hope nothing else uses this header for anything
 */
#define STRIPZIP_OPTION_HEADER 0xFFFF

typedef struct __attribute__ ((__packed__))
{
  uint32_t signature;
  uint16_t version_needed;
  uint16_t gp_bits;
  uint16_t compression_method;
  uint16_t last_mod_time;
  uint16_t last_mod_date;
  uint32_t crc32;
  uint32_t compressed_size;
  uint32_t uncompressed_size;
  uint16_t name_length;
  uint16_t extra_field_length;
} local_file_header_t;

typedef struct __attribute__ ((__packed__))
{
  uint32_t signature;
  uint16_t version_made_by;
  uint16_t version_needed;
  uint16_t gp_bits;
  uint16_t compression_method;
  uint16_t last_mod_time;
  uint16_t last_mod_date;
  uint32_t crc32;
  uint32_t compressed_size;
  uint32_t uncompressed_size;
  uint16_t file_name_length;
  uint16_t extra_field_length;
  uint16_t file_comment_length;
  uint16_t disk_number_start;
  uint16_t internal_attr;
  uint32_t external_attr;
  uint32_t rel_offset_local_header;
} central_directory_header_t;

typedef struct __attribute__ ((__packed__))
{
  uint32_t signature;
  uint16_t disk_number;
  uint16_t disk_num_start_of_cd;
  uint16_t num_dir_entries_this_disk;
  uint16_t total_num_entries_cd;
  uint32_t size_of_cd;
  uint32_t cd_offset_in_first_disk;
  uint16_t zip_file_comment_length;
} end_of_central_directory_header_t;

typedef struct __attribute__ ((__packed__))
{
  uint16_t id;
  uint16_t length;
} extra_header_t;


/**
 * Convenience function to overwrite a section of data with the file seek at
 * the end of the segment, and then set the seek back to the segment end.
 */
void overwrite_field(void* data, size_t len, FILE *fd)
{
  fseek(fd, -1 * len, SEEK_CUR);
  ERR_IF_NEQ(fwrite(data, len, 1, fd), 1u);
}


/**
 * Take either a central directory or local file extra data field and for the
 * things we know are horrible; purify it!
 *
 * TODO: It would be better if stripzip removed the headers completely so that the
 * ZIP was invariant regardless of what crazy program created it. But that's hard.
 */
bool purify_extra_data(size_t len, void* extra_data)
{
  size_t offset = 0;
  while (offset < len)
  {
    extra_header_t *hdr = extra_data + offset;
    offset += sizeof(extra_header_t);

    switch (hdr->id)
    {
      case 0x5455:
        /* Some sort of extended time data, see
         * ftp://ftp.info-zip.org/pub/infozip/src/zip30.zip ./proginfo/extrafld.txt
        .. fallthrough */
      case 0x7875:
        /* Unix extra data; UID / GID stuff, see
         * ftp://ftp.info-zip.org/pub/infozip/src/zip30.zip ./proginfo/extrafld.txt
         */
        hdr->id = STRIPZIP_OPTION_HEADER;
        memset(extra_data + offset, 0xFF, hdr->length);
        break;

      case STRIPZIP_OPTION_HEADER:
        break;

      default:
        printf("\tUnknown extra header: 0x%x %u\n", hdr->id, hdr->length);
        return false;
        break;
    }
    offset += hdr->length;
  }

  return true;
}


int main(int argc, char** argv)
{
  if (argc != 2)
  {
    printf("Usage: stripzip <in.zip>\n");
    return -1;
  }

  FILE *zf = NULL;
  ERR_RET_IF_NOT(zf = fopen(argv[1], "r+"), -1);

  /* Get the EO CenDir header */
  ERR_RET_ON_ERRNO(fseek(zf, -1 * sizeof(end_of_central_directory_header_t), SEEK_END), -1);
  end_of_central_directory_header_t eocd_header;
  ERR_RET_IF_NEQ(fread(&eocd_header, sizeof(eocd_header), 1, zf), 1u, -1);
  if (eocd_header.signature != EO_CENDIR_HEADER_SIGNATURE)
  {
    printf("Did not get a good end of directory header! There might be a ZIP file comment?\n");
    return -1;
  }
  if (eocd_header.disk_number != 0)
  {
    printf("Split archive! This tool doesn't deal with those!\n");
    return -1;
  }
  if (eocd_header.size_of_cd == 0xFFFFFFFF)
  {
    printf("This is a Zip64 file; and I don't know how to deal with those!\n");
    return -1;
  }

  /* For each entry in the central directory; purify it! */
  char local_filename[UINT16_MAX];
  char local_filecomment[UINT16_MAX];
  char local_extra[UINT16_MAX];
  fseek(zf, eocd_header.cd_offset_in_first_disk, SEEK_SET);
  for (size_t dir_entry = 0; dir_entry < eocd_header.total_num_entries_cd; dir_entry++)
  {
    printf("Now purifying entry %lu / %u (offset 0x08%lx) ", dir_entry + 1, eocd_header.total_num_entries_cd, ftell(zf));

    central_directory_header_t cd_header = {0};
    ERR_RET_IF_NEQ(fread(&cd_header, sizeof(cd_header), 1, zf), 1u, -1);
    {
      if (cd_header.signature != CENDIR_HEADER_SIGNATURE)
      {
        printf("File corrupted! Central directory signature bad (0x%x).\n", cd_header.signature);
        return -1;
      }

      if ((cd_header.gp_bits & GP_BIT_ENC_MARKERS) != 0x0)
      {
        printf("Entry encrypted, I don't know how to deal with that.\n");
        return -1;
      }
      if ((cd_header.gp_bits & GP_BIT_UNKNOWN_FLAG_MASK) != 0)
      {
        printf("Entry has strange general purpose bits: %u\n", cd_header.gp_bits);
        return -1;
      }

      // Purify time / date of CD header
      cd_header.last_mod_date = 0;
      cd_header.last_mod_time = 0;
      overwrite_field(&cd_header, sizeof(cd_header), zf);

      // Get the file name and comment
      ERR_RET_IF_NEQ(fread(local_filename, cd_header.file_name_length, 1, zf), 1u, -1);
      if (cd_header.file_comment_length > 0)
      {
        ERR_RET_IF_NEQ(fread(local_filecomment, cd_header.file_comment_length, 1, zf), 1u, -1);
      }
      printf("%.*s\n", cd_header.file_name_length, local_filename);
    }

    // Get and purify the extra data
    if (cd_header.extra_field_length)
    {
      ERR_RET_IF_NEQ(fread(local_extra, cd_header.extra_field_length, 1, zf), 1u, -1);
      ERR_RET_IF_NOT(purify_extra_data(cd_header.extra_field_length, local_extra), -1);
      overwrite_field(local_extra, cd_header.extra_field_length, zf);
    }

    // Now deal with the local header
    size_t current_cd_position = ftell(zf);
    fseek(zf, cd_header.rel_offset_local_header, SEEK_SET);
    {
      local_file_header_t lf_header = {0};
      ERR_RET_IF_NEQ(fread(&lf_header, sizeof(local_file_header_t), 1, zf), 1u, -1);
      ERR_RET_IF_NEQ(lf_header.signature, FILE_HEADER_SIGNATURE, -1);

      if ((lf_header.gp_bits & GP_BIT_ENC_MARKERS) != 0x0)
      {
        printf("Entry encrypted, I don't know how to deal with that.\n");
        return -1;
      }
      if ((lf_header.gp_bits & GP_BIT_UNKNOWN_FLAG_MASK) != 0)
      {
        printf("Entry has strange general purpose bits: %u\n", cd_header.gp_bits);
        return -1;
      }

      lf_header.last_mod_date = 0;
      lf_header.last_mod_time = 0;
      overwrite_field(&lf_header, sizeof(local_file_header_t), zf);

      // Seek over the filename (assuming there's nothing sensitive in here)
      fseek(zf, lf_header.name_length, SEEK_CUR);

      // Take care of local data extra
      if (lf_header.extra_field_length)
      {
        ERR_RET_IF_NEQ(fread(local_extra, lf_header.extra_field_length, 1, zf), 1u, -1);
        ERR_RET_IF_NOT(purify_extra_data(lf_header.extra_field_length, local_extra), -1);
        overwrite_field(local_extra, lf_header.extra_field_length, zf);
      }
    }
    fseek(zf, current_cd_position, SEEK_SET);
  }

  fclose(zf);
  return 0;
}
