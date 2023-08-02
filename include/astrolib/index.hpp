#pragma once

#include "types.hpp"
#include "pointer.hpp"
#include "pbffile.hpp"

namespace leapus::astrolib::index{

enum index_entry_type{

    //A segment of a line, like a road
    //Can be a list of segments forming a path
    idx_line,

    //A list of idx_line which describes a filled polygon
    //It has to be clipped to the edge of the indexing box.
    //Polygons are a serious pain to manage in terms of clipping and
    //boolean operations like subtraction (holes). So, I think we will
    //start out just rendering edges (lines). The upshot is that it will
    //be really efficient, and it will have kind of a retro Tron look to it.
    idx_poly,

    //A textual label
    idx_label,

    //Some other widget, like a dot or an icon to denote a location, feature, or POI
    idx_widget
};

template<typename T>
using index_allocator = pbf::protobuf_file::allocator_type<T>;

struct index_entry{

    //TODO: Represent the type of element

    //For widgets, like textual labels, icons, markers,
    //these bounds are the coverage of the widget when superimposed over the map
    //if the current spatial tree square represented the entire screen.
    //That way, we are using a consistent coordinate system, and the entire idea
    //is that a square of the quadtree represents somewhere between 1/4 and 1x of
    //a relevant set at any given time.
    box_t bounds;

    //Where to find the serialized object in the OSM PBF file
    osm_address_t address;

    //If this item was generated or interpolated as part of detail reduction,
    //then this is the offset into the index file (not the OSM file) to find
    //the generated OSM object, otherwise this is 0 for null. "address" can point
    //to an associated OSM object from which the detail reduction was derived.
    file_offs_t reduction_detail;
};

//A square in a quadtree representing 1/4-1x of a relevant set for rendering.
//If it's a leaf node, it just points to stuff in the OSM file.
//If it's a branch node, then in addition to pointing to other nodes,
//it will have detail reduction data so that you can draw some notion
//of the entire dataset below it without actually drawing stuff you can't even see
//either because it's too small at that scale or because it's too bunched together
//with other items to distinguish.
struct quadtree_square{

    box_t bounds;

    //The four quadrants in the tree if we should get bisected
    pointer::relative_ptr<quadtree_square> nw,ne,sw,se;
};

struct index_config{
    osm::osm_file in_file;            //in file
    index_allocator<char> file_allocator;  //out file 

    //Maximum number of items permitted in an index node
    //before it is bisected. These are index nodes, not map nodes.
    //We will be using quadtrees, so this would be a quad, whether leaf or not. 
    int node_max_items;

};

//WR classes are wrappers around serializable data providing methods
//and temporary state needed to run them. They can be thought of like
//a pointer which adds methods, but can be reassigend a different target
class WRQuadSquare:public pbf::protobuf_file::pointer_type<quadtree_square> {
    
protected:
    using base_type=leapus::pbf::protobuf_file::pointer_type<quadtree_square>;

    //When the square gets overfull, split into four sub-quads
    void quadrasect();

public:
    WRQuadSquare( const index_config &conf, const base_type &ptr);
    void push();
};


class Index{
    quadtree_square m_root;

protected:
    const index_config &p_config;

public:
    Index(const index_config &config);

};

}
