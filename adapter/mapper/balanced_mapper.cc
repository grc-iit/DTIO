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

#include "balanced_mapper.h"
#include "api/hermes.h"

namespace hermes::adapter {

/**
 * Convert a range defined by \a off and \a size into specific
 * blob placements.
 */
void BalancedMapper::map(size_t off, size_t size,
                         size_t page_size,
                         BlobPlacements &ps) {
  HILOG(kDebug, "Mapping File with offset {} and size {}", off, size);
  size_t kPageSize = page_size;
  size_t size_mapped = 0;
  while (size > size_mapped) {
    BlobPlacement p;
    p.bucket_off_ = off + size_mapped;
    p.page_ = p.bucket_off_ / kPageSize;
    p.page_size_ = page_size;
    p.blob_off_ = p.bucket_off_ % kPageSize;
    auto left_size_page = kPageSize - p.blob_off_;
    p.blob_size_ = left_size_page < size - size_mapped ? left_size_page
      : size - size_mapped;
    ps.emplace_back(p);
    size_mapped += p.blob_size_;
  }
}


}  // namespace hermes::adapter
