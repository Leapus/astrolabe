#pragma once

#include <cstdint>
#include <cstddef>

namespace leapus::astrolib{

    //PBF files can most certainly be (far) over 2GB, so 32-bits will not cut it
    using file_offs_t=::size_t;

    //Blobs are never supposed to exceed 32MiB
    using blob_offs_t=int;

    //Info needed to address an OSM PBF object
    //First, you have to locate the oft-compressed blob,
    //then you need the offset into its uncompressed data
    struct osm_address_t{
        file_offs_t blob_pos;
        blob_offs_t item_pos;
    };

    //Maximum OSM precision is in nano-degrees or billionths of a degree
    //So, these are effectively 38-bit numbers.
    using ordinate_t=int64_t;

    struct coordinate_t{
        ordinate_t lat, lon;
    };

    struct box_t{
        coordinate_t sw, ne;
    };
}
