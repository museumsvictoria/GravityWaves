//
//  Types.h
//  GravityWaves
//
//  Created by Andrew Wright on 27/11/17.
//

#ifndef Types_h
#define Types_h

#include "cinder/Signals.h"
#include "cinder/Json.h"

using u8            = std::uint8_t;
using u16           = std::uint16_t;
using u32           = std::uint32_t;
using u64           = std::uint64_t;

using s8            = std::int8_t;
using s16           = std::int16_t;
using s32           = std::int32_t;
using s64           = std::int64_t;

using f32           = float;
using f64           = double;

using DefaultSignal = ci::signals::Signal<void()>;

class Serializable
{
public:
    Serializable        ( const std::string& name ) : _serializationName(name){ };
    
    virtual void        Write ( ci::JsonTree& tree ) = 0;
    virtual void        Read  ( const ci::JsonTree& tree ) = 0;
    
    ci::JsonTree        WriteVec2 ( const std::string& name, const ci::vec2& v );
    ci::JsonTree        WriteVec3 ( const std::string& name, const ci::vec3& v );
    
    ci::vec2            ReadVec2  ( const ci::JsonTree& tree, const std::string& name );
    ci::vec3            ReadVec3  ( const ci::JsonTree& tree, const std::string& name );
    
protected:
    
    std::string         _serializationName;
};

#endif /* Types_h */
