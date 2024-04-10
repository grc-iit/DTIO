/*
 * Copyright (C) 2024 Gnosis Research Center <grc@iit.edu>, 
 * Keith Bateman <kbateman@hawk.iit.edu>, Neeraj Rajesh
 * <nrajesh@hawk.iit.edu> Hariharan Devarajan
 * <hdevarajan@hawk.iit.edu>, Anthony Kougkas <akougkas@iit.edu>,
 * Xian-He Sun <sun@iit.edu>
 *
 * This file is part of DTIO
 *
 * DTIO is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef HERMES_ABSTRACT_MAPPER_H
#define HERMES_ABSTRACT_MAPPER_H

#include "hermes_types.h"

namespace hermes::adapter {

/**
 * Define different types of mappers supported by POSIX Adapter.
 * Also define its construction in the MapperFactory.
 */
enum class MapperType {
  kBalancedMapper
};

/**
 A structure to represent BLOB placement
*/
struct BlobPlacement {
  size_t page_;       /**< The index in the array placements */
  size_t page_size_;  /**< The size of a page */
  size_t bucket_off_; /**< Offset from file start (for FS) */
  size_t blob_off_;   /**< Offset from BLOB start */
  size_t blob_size_;  /**< Size after offset to read */
  int time_;          /**< The order of the blob in a list of blobs */

  /** create a BLOB name from index. */
  hshm::charbuf CreateBlobName() const {
    hshm::charbuf buf(sizeof(page_) + sizeof(blob_off_));
    size_t off = 0;
    memcpy(buf.data() + off, &page_, sizeof(page_));
    off += sizeof(page_);
    memcpy(buf.data() + off, &page_size_, sizeof(page_size_));
    return buf;
  }

  /** decode \a blob_name BLOB name to index.  */
  template<typename StringT>
  void DecodeBlobName(const StringT &blob_name) {
    size_t off = 0;
    memcpy(&page_, blob_name.data(), sizeof(page_));
    off += sizeof(page_);
    memcpy(&page_size_, blob_name.data() + off, sizeof(page_size_));
  }
};

typedef std::vector<BlobPlacement> BlobPlacements;

/**
   A class to represent abstract mapper
*/
class AbstractMapper {
 public:
  /** Virtual destructor */
  virtual ~AbstractMapper() = default;

  /**
   * This method maps the current operation to Hermes data structures.
   *
   * @param off offset
   * @param size size
   * @param page_size the page division factor
   * @param ps BLOB placement
   *
   */
  virtual void map(size_t off, size_t size, size_t page_size,
                   BlobPlacements &ps) = 0;
};
}  // namespace hermes::adapter

#endif  // HERMES_ABSTRACT_MAPPER_H
