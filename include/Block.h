#pragma once

#include "BlockTypes.h"

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