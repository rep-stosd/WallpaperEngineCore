
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

constexpr uint32_t std_format_vertex_size_herald_value2 = 0x0181000E;

constexpr uint32_t singile_bone_frame = 4 * 9;

void ss_readstr(std::ifstream& ss, std::string& str)
{
    char c = 0;
    do
    {
        ss.read(&c, 1);
        if (c)
            str.push_back(c);
    }
    while(c);
}

std::string ss_readstr(std::ifstream& ss)
{
    std::string str;
    char c = 0;
    do
    {
        ss.read(&c, 1);
        if (c)
            str.push_back(c);
    }
    while(c);
    return str;
}

template<typename T>
T ss_read(std::ifstream& ss)
{
    T ret;
    ss.read((char*)&ret, sizeof(T));
    return ret;
}

#define ss_read(ss, ret) ss.read((char*)&ret, sizeof(ret));

PAKModel PAKModel_Alloc(const std::string& path)
{
    std::ifstream ss(path.data());

    if (!ss.is_open()) {
        return {};
    }
    
    PAKModelHeader hdr{};
    PAKModel mdl{};
    
    ss.read(hdr.magic, sizeof(hdr.magic));
    ss.ignore(1);
    
    ss.read((char*)&hdr.flags, sizeof(hdr.flags));
    
    ss.ignore(sizeof(int)); // 2
    ss.ignore(sizeof(int)); // 3
    
    ss_readstr(ss, hdr.json);

    
    ss.ignore(sizeof(int)); // 4
    
    bool alt_mdl_format = false;
    uint32_t curr = 0;
    ss.read((char*)&curr, sizeof(curr)); // 5

    // if the uint at the normal vertex size position is 0, then this file
    // uses the alternative MDL format, therefore the actual vertex size is
    // located after the herald value, and we'll need to account for other differences later on.
    if(curr == 0)
    {
        alt_mdl_format = true;
        while (curr != alt_format_vertex_size_herald_value && curr != std_format_vertex_size_herald_value2)
        {
            ss.read((char*)&curr, sizeof(curr));
        }
        ss.read((char*)&curr, sizeof(curr));
    }
    else if(curr == std_format_vertex_size_herald_value)
    {
        ss.read((char*)&curr, sizeof(curr));
    }
    
    uint32_t vertex_size = curr;
    if (vertex_size % (alt_mdl_format? alt_singile_vertex : singile_vertex) != 0) {
        printf("unsupport mdl vertex size %d", vertex_size);
        return {};
    }

    // if using the alternative MDL format, vertexes contain 7 extra 32-bit chunks between
    // position and blend indices
    uint32_t vertex_num = vertex_size / (alt_mdl_format ? alt_singile_vertex : singile_vertex);
    mdl.vertices.resize(vertex_num);
    for (auto& vert : mdl.vertices) {
        ss.read((char*)&vert.position, sizeof(vert.position));
        
        if(alt_mdl_format) {
            for (int i = 0; i < 7; i++)
                ss.ignore(sizeof(int));
        }
        
        ss.read((char*)&vert.blend_indices, sizeof(vert.blend_indices));
        ss.read((char*)&vert.weight, sizeof(vert.weight));
        ss.read((char*)&vert.texcoord, sizeof(vert.texcoord));
        
    }
    
    uint32_t index_size = 0;
    ss.read((char*)&index_size, sizeof(index_size));
    if (index_size % singile_indices != 0) {
        printf("unsupport mdl indices size %d", index_size);
        return {};
    }
    
    uint32_t index_num = index_size / singile_indices;
    mdl.indices.resize(index_num * 3); // per tri
    for (auto& id : mdl.indices) {
        ss.read((char*)&id, sizeof(id));
    }
    
    
    ss.read(hdr.skinmagic, sizeof(hdr.skinmagic));
    ss.ignore(1);
    
    size_t bones_file_end;
    uint16_t bones_num;
    
    ss.read((char*)&bones_file_end, sizeof(bones_file_end));
    ss.read((char*)&bones_num, sizeof(bones_num));
    
    ss.ignore(sizeof(uint16_t));
    
    mdl.bones.resize(bones_num);
    
    for (uint i = 0; i < bones_num; i++)
    {
        auto&       bone = mdl.bones[i];
        
        std::string name = ss_readstr(ss);
        
        ss.ignore(sizeof(uint32_t));
        
        ss_read(ss, bone.parent);
        
        if (bone.parent >= i && bone.parent != -1)
        {
            printf("mdl wrong bone parent index %d", bone.parent);
            return {};
        }
        
        auto size = ss_read<uint32_t>(ss);
        
        if (size != 64) {
            printf("mdl unsupport bones size: %d", size);
            return {};
        }
        
        for (int i = 0; i < bone.transform.length(); ++i) {
            for (int j = 0; j < bone.transform[i].length(); ++j) {
                ss_read(ss, bone.transform[i][j]);
            }
        }
        

        std::string bone_simulation_json = ss_readstr(ss);
    }
    
    if (atoi(&hdr.skinmagic[3]) > 1)
    {
        auto unk = ss_read<uint16_t>(ss);
        
        auto has_trans = ss_read<uint8_t>(ss);
        
        if (has_trans) {
            for (uint i = 0; i < bones_num; i++)
                for (uint j = 0; j < 16; j++)
                    ss_read<float>(ss);
        }
        
        uint32_t size_unk = ss_read<uint32_t>(ss);
        for (uint i = 0; i < size_unk; i++)
            for (int j = 0; j < 3; j++)
                ss_read<uint32_t>(ss);
        
        ss_read<uint32_t>(ss);
        
        
        uint8_t has_offset_trans = ss_read<uint8_t>(ss);
        if (has_offset_trans) {
            for (uint i = 0; i < bones_num; i++) {
                for (uint j = 0; j < 3; j++) ss_read<float>(ss);  // like pos
                for (uint j = 0; j < 16; j++) ss_read<float>(ss); // mat
            }
        }

        uint8_t has_index = ss_read<uint8_t>(ss);
        if (has_index) {
            for (uint i = 0; i < bones_num; i++) {
                ss_read<uint32_t>(ss);
            }
        }
        
    }
    
    
    // sometimes there can be one or more zero bytes and/or MDAT sections containing
    // attachments before the MDLA section, so we need to skip them
    std::string mdType = "";
    std::string mdVersion;
    
    do {
        std::string mdPrefix = ss_readstr(ss);

        // sometimes there can be other garbage in this gap, so we need to
        // skip over that as well
        if(mdPrefix.length() == 8){
            mdType = mdPrefix.substr(0, 4);
            mdVersion = mdPrefix.substr(4, 4);

            if(mdType == "MDAT"){
                ss_read<uint32_t>(ss);// skip 4 bytes
                uint32_t num_attachments = ss_read<uint16_t>(ss); // number of attachments in the MDAT section

                for(int i = 0; i < num_attachments; i++){
                    ss_read<uint16_t>(ss); // skip 2 bytes
                    std::string attachment_name = ss_readstr(ss); // attachment name
                    int bytesToRead = mdat_attachment_data_byte_length;
                    for(int j = 0; j < bytesToRead; j++){
                        ss_read<uint8_t>(ss);
                    }

                }
            }
        }
    } while (mdType != "MDLA");
    
    

       if(mdType == "MDLA" && mdVersion.length() > 0){
           mdl.animVer = std::stoi(mdVersion);
           if (mdl.animVer != 0) {
               uint end_size = ss_read<uint32_t>(ss);

               uint anim_num = ss_read<uint32_t>(ss);
               mdl.animations.resize(anim_num);
               for (auto& anim : mdl.animations) {
                   // there can be a variable number of 32-bit 0s between animations
                   anim.id = 0;
                   while(anim.id == 0){
                       anim.id = ss_read<int32_t>(ss);
                   }
       
                   if (anim.id <= 0) {
                       printf("wrong anime id %d", anim.id);
                       return {};
                   }
                   ss_read<int32_t>(ss);
                   
                   anim.name   = ss_readstr(ss);
                   if(anim.name.empty()){
                       anim.name = ss_readstr(ss);
                   }
                   auto mode = ss_readstr(ss);
                   
                   if (mode == "loop")
                       anim.mode = 0;
                   else if (mode == "mirror")
                       anim.mode = 1;
                   else if (mode == "single")
                       anim.mode = 2;
                   
                   anim.fps    = ss_read<float>(ss);
                   anim.length = ss_read<int32_t>(ss);
                   ss_read<int32_t>(ss);

                   uint32_t b_num = ss_read<uint32_t>(ss);
                   anim.bframes_array.resize(b_num);
                   for (auto& bframes : anim.bframes_array) {
                       ss_read<int32_t>(ss);
                       uint32_t byte_size = ss_read<uint32_t>(ss);
                       uint32_t num       = byte_size / singile_bone_frame;
                       if (byte_size % singile_bone_frame != 0) {
                           printf("wrong bone frame size %d", byte_size);
                           return {};
                       }
                       bframes.resize(num);
                       for (auto& frame : bframes) {
                           frame.position = ss_read<glm::vec3>(ss);
                           frame.angle = ss_read<glm::vec3>(ss);
                           frame.scale = ss_read<glm::vec3>(ss);
                       }
                   }
                   
                   // in the alternative MDL format there are 2 empty bytes followed
                   // by a variable number of 32-bit 0s between animations. We'll read
                   // the two bytes now so that the cursor is aligned to read through the
                   // 32-bit 0s in the next iteration
                   if(alt_mdl_format)
                   {
                       ss_read<uint8_t>(ss);
                       ss_read<uint8_t>(ss);
                   }
                   else if(mdl.animVer == 3){
                       // In MDLA version 3 there is an extra 8-bit zero between animations.
                       // This will cause the parser to be misaligned moving forward if we don't handle it here.
                       ss_read<uint8_t>(ss);
                   }
                   else{
                       uint32_t unk_extra_uint = ss_read<uint32_t>(ss);
                       for (uint i = 0; i < unk_extra_uint; i++) {
                           ss_read<float>(ss);
                           // data is like: {"$$hashKey":"object:2110","frame":1,"name":"random_anim"}
                           std::string unk_extra = ss_readstr(ss);
                       }
                   }
               }
           }
       }
    
    
    
    ss.close();
    
    return mdl;
}
