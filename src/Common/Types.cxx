//
//  Types.cxx
//  GravityWaves
//
//  Created by Andrew Wright on 29/11/17.
//

#include <Common/Types.h>

using namespace ci;

JsonTree Serializable::WriteVec2 ( const std::string& name, const vec2& v )
{
    auto t = JsonTree::makeObject ( name );
    t.pushBack ( JsonTree ( "X", v.x ) );
    t.pushBack ( JsonTree ( "Y", v.y ) );
    
    return t;
}

JsonTree Serializable::WriteVec3 ( const std::string& name, const vec3& v )
{
    auto t = WriteVec2 ( name, vec2(v) );
    t.pushBack ( JsonTree ( "Z", v.z ) );
    
    return t;
}

vec2 Serializable::ReadVec2 ( const JsonTree& tree, const std::string& name )
{
    vec2 v;
    v.x = tree[name]["X"].getValue<float>();
    v.y = tree[name]["Y"].getValue<float>();
    
    return v;
}

vec3 Serializable::ReadVec3 ( const JsonTree& tree, const std::string& name )
{
    vec2 v = ReadVec2 ( tree, name );
    vec3 r ( v, 0 );
    r.z = tree[name]["Z"].getValue<float>();
    
    return r;
}
