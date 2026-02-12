
#include "PAK.h"


// bytes * size
constexpr uint32_t singile_vertex  = 4 * (3 + 4 + 4 + 2);
constexpr uint32_t singile_indices = 2 * 3;
constexpr uint32_t std_format_vertex_size_herald_value = 0x01800009;

// number of bytes in an MDAT attachment after the attachment name
constexpr uint32_t mdat_attachment_data_byte_length = 64;

// alternative consts for alternative mdl format
constexpr uint32_t alt_singile_vertex = 4 * (3 + 4 + 4 + 2 + 7);
constexpr uint32_t alt_format_vertex_size_herald_value = 0x0180000F;

constexpr uint32_t singile_bone_frame = 4 * 9;


PAKModel PAKModel_Alloc(const std::string& path)
{
    
    std::ifstream ss(path.data());

    if (!ss.is_open()) {
        return {};
    }
    
    PAKModelHeader hdr{};
    PAKModel mdl{};
    
    ss.read(hdr.magic, sizeof(hdr.magic));
    ss.read((char*)&hdr.flags, sizeof(hdr.flags));
    
    ss.ignore(1);
    ss.ignore(1);
    
    std::getline( ss, hdr.json, '\0' );
    
    ss.ignore(1);

    
    bool alt_mdl_format = false;
    uint32_t curr = 0;
    ss.read((char*)&curr, sizeof(curr));

    // if the uint at the normal vertex size position is 0, then this file
    // uses the alternative MDL format, therefore the actual vertex size is
    // located after the herald value, and we'll need to account for other differences later on.
    if(curr == 0)
    {
        alt_mdl_format = true;
        while (curr != alt_format_vertex_size_herald_value)
        {
            ss.read((char*)&curr, sizeof(curr));
        }
        ss.read((char*)&curr, sizeof(curr));
    }
    else if(curr == std_format_vertex_size_herald_value)
    {
        ss.read((char*)&curr, sizeof(curr));
    }
    
    
    
    
    ss.close();
    
    return {};
}
