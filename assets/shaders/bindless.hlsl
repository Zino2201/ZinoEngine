ByteAddressBuffer bindless_storage_buffers[] : register(t0);
RWByteAddressBuffer bindless_rw_storage_buffers[] : register(u1);
Texture2D bindless_textures_2D[] : register(t2);
TextureCube bindless_textures_cube[] : register(t3);
SamplerState bindless_samplers[] : register(s4);
 
template<typename T>
inline T get_resource(uint) {}

template<> inline ByteAddressBuffer get_resource(uint index)
{
    return bindless_storage_buffers[NonUniformResourceIndex(index)];
}

template<> inline RWByteAddressBuffer get_resource(uint index)
{
    return bindless_rw_storage_buffers[NonUniformResourceIndex(index)];
}

template<> inline Texture2D get_resource(uint index)
{
    return bindless_textures_2D[NonUniformResourceIndex(index)];
}

template<> inline TextureCube get_resource(uint index)
{
    return bindless_textures_cube[NonUniformResourceIndex(index)];
}

template<> inline SamplerState get_resource(uint index)
{
    return bindless_samplers[NonUniformResourceIndex(index)];
}

template<typename StructType>
inline StructType get_struct(uint index)
{
    return get_resource<ByteAddressBuffer>(index).Load<StructType>(0);
}