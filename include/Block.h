#pragma once

#include <cstdint>

enum class BlockType : uint8_t {
    AIR = 0,
    BLOCK = 1
};

class Block {
public:
    Block();
    Block(BlockType type);
    
    BlockType GetType() const;
    void SetType(BlockType type);
    
    bool IsAir() const;
    bool IsSolid() const;

private:
    BlockType m_type;
}; 